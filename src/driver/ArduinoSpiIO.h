/**
 ******************************************************************************
 * @file    user_diskio_spi.c
 * @brief   This file contains the implementation of the user_diskio_spi FatFs
 *          driver.
 ******************************************************************************
 * Portions copyright (C) 2014, ChaN, all rights reserved.
 * Portions copyright (C) 2017, kiwih, all rights reserved.
 *
 * This software is a free software and there is NO WARRANTY.
 * No restriction on use. You can use, modify and redistribute it for
 * personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
 * Redistributions of source code must retain the above copyright notice.
 *
 ******************************************************************************
 */

// This code was ported by kiwih from a copywrited (C) library written by ChaN
// available at http://elm-chan.org/fsw/ff/ffsample.zip
//(text at http://elm-chan.org/fsw/ff/00index_e.html)

// This file provides the FatFs driver functions and SPI code required to manage
// an SPI-connected MMC or compatible SD card with FAT

// It is designed to be wrapped by a cubemx generated user_diskio.c file.

#pragma once

#include "BaseIO.h"
#include "SPI.h"
#include "sdcommon.h"


namespace fatfs {

/**
 * @brief Accessing a SD card via the Arduino SPI API
 * @ingroup io
 */

class ArduinoSpiIO : public BaseIO {
 public:
  ArduinoSpiIO(int cs = -1, SPIClass &spi = SPI) { setSPI(cs, spi); }
  ArduinoSpiIO(SPIClass &spi) { setSPI(spi); }

  void setSPI(SPIClass &spi = SPI) {
    this->p_spi = &spi;
    this->cs = -1;
  }

  void setSPI(int cs = -1, SPIClass &spi = SPI) {
    this->p_spi = &spi;
    this->cs = cs;
    if (cs != -1) {
      pinMode(cs, OUTPUT);
    }
  }

  DSTATUS disk_initialize(BYTE drv /* Physical drive number (0) */
                          ) override {
    BYTE n, cmd, ty, ocr[4];

    if (drv != 0) return STA_NOINIT; /* Supports only drive 0 */
    // assume SPI already init init_spi();	/* Initialize SPI */

    if (stat & STA_NODISK) return stat; /* Is card existing in the soket? */

    set_spi_fast(false);
    for (n = 10; n; n--) xchg_spi(0xFF); /* Send 80 dummy clocks */

    ty = 0;
    if (send_cmd(CMD0, 0) == 1) {       /* Put the card SPI/Idle state */
      spi_timer_on(1000);               /* Initialization timeout = 1 sec */
      if (send_cmd(CMD8, 0x1AA) == 1) { /* SDv2? */
        for (n = 0; n < 4; n++)
          ocr[n] = xchg_spi(0xFF); /* Get 32 bit return value of R7 resp */
        if (ocr[2] == 0x01 &&
            ocr[3] == 0xAA) { /* Is the card supports vcc of 2.7-3.6V? */
          while (spi_timer_status() &&
                 send_cmd(ACMD41, 1UL << 30)); /* Wait for end of initialization
                                                  with ACMD41(HCS) */
          if (spi_timer_status() &&
              send_cmd(CMD58, 0) == 0) { /* Check CCS bit in the OCR */
            for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);
            ty =
                (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2; /* Card id SDv2 */
          }
        }
      } else {                          /* Not SDv2 card */
        if (send_cmd(ACMD41, 0) <= 1) { /* SDv1 or MMC? */
          ty = CT_SD1;
          cmd = ACMD41; /* SDv1 (ACMD41(0)) */
        } else {
          ty = CT_MMC;
          cmd = CMD1; /* MMCv3 (CMD1(0)) */
        }
        while (spi_timer_status() &&
               send_cmd(cmd, 0)); /* Wait for end of initialization */
        if (!spi_timer_status() ||
            send_cmd(CMD16, 512) != 0) /* Set block length: 512 */
          ty = 0;
      }
    }
    CardType = ty; /* Card type */
    despiselect();

    if (ty) {             /* OK */
      set_spi_fast(true); /* Set fast clock */
      stat = STA_CLEAR;   /* Clear STA_NOINIT flag */
    } else {              /* Failed */
      stat = STA_NOINIT;
    }

    return stat;
  }

  /*-----------------------------------------------------------------------*/
  /* Get disk status                                                       */
  /*-----------------------------------------------------------------------*/

  DSTATUS disk_status(BYTE drv /* Physical drive number (0) */
                      ) override {
    if (drv) return STA_NOINIT; /* Supports only drive 0 */
    return stat;                /* Return disk status */
  }

  /*-----------------------------------------------------------------------*/
  /* Read sector(s)                                                        */
  /*-----------------------------------------------------------------------*/

  DRESULT disk_read(
      BYTE drv,     /* Physical drive number (0) */
      BYTE *buff,   /* Pointer to the data buffer to store read data */
      DWORD sector, /* Start sector number (LBA) */
      UINT count    /* Number of sectors to read (1..128) */
      ) override {
    if (drv || !count) return RES_PARERR;     /* Check parameter */
    if (stat & STA_NOINIT) return RES_NOTRDY; /* Check if drive is ready */

    if (!(CardType & CT_BLOCK))
      sector *= 512; /* LBA ot BA conversion (byte addressing cards) */

    if (count == 1) {                    /* Single sector read */
      if ((send_cmd(CMD17, sector) == 0) /* READ_SINGLE_BLOCK */
          && rcvr_datablock(buff, 512)) {
        count = 0;
      }
    } else {                              /* Multiple sector read */
      if (send_cmd(CMD18, sector) == 0) { /* READ_MULTIPLE_BLOCK */
        do {
          if (!rcvr_datablock(buff, 512)) break;
          buff += 512;
        } while (--count);
        send_cmd(CMD12, 0); /* STOP_TRANSMISSION */
        wait_ready(500);     /* Wait for card to be ready after stop transmission */
      }
    }
    despiselect();

    return count ? RES_ERROR : RES_OK; /* Return result */
  }

  /*-----------------------------------------------------------------------*/
  /* Write sector(s)                                                       */
  /*-----------------------------------------------------------------------*/

#if FF_IO_USE_WRITE

  DRESULT disk_write(BYTE drv,     /* Physical drive number (0) */
                     const BYTE *buff,   /* Ponter to the data to write */
                     LBA_t sector, /* Start sector number (LBA) */
                     UINT count    /* Number of sectors to write (1..128) */
                     ) override {
    if (drv || !count) return RES_PARERR;     /* Check parameter */
    if (stat & STA_NOINIT) return RES_NOTRDY; /* Check drive status */
    if (stat & STA_PROTECT) return RES_WRPRT; /* Check write protect */

    if (!(CardType & CT_BLOCK))
      sector *= 512; /* LBA ==> BA conversion (byte addressing cards) */

    if (count == 1) {                    /* Single sector write */
      if ((send_cmd(CMD24, sector) == 0) /* WRITE_BLOCK */
          && xmit_datablock((BYTE*)buff, 0xFE)) {
        count = 0;
      }
    } else { /* Multiple sector write */
      if (CardType & CT_SDC)
        send_cmd(ACMD23, count);          /* Predefine number of sectors */
      if (send_cmd(CMD25, sector) == 0) { /* WRITE_MULTIPLE_BLOCK */
        do {
          if (!xmit_datablock((BYTE*)buff, 0xFC)) break;
          buff += 512;
        } while (--count);
        if (!xmit_datablock(0, 0xFD)) count = 1; /* STOP_TRAN token */
      }
    }
    despiselect();

    return count ? RES_ERROR : RES_OK; /* Return result */
  }
#endif

  /*-----------------------------------------------------------------------*/
  /* Miscellaneous drive controls other than data read/write               */
  /*-----------------------------------------------------------------------*/

#if FF_IO_USE_IOCTL
  DRESULT disk_ioctl(BYTE drv,  /* Physical drive number (0) */
                     ioctl_cmd_t cmd,  /* Control command code */
                     void *buff /* Pointer to the conrtol data */
                     ) override {
    DRESULT res;
    BYTE n, csd[16];
    DWORD *dp, st, ed, csize;

    if (drv) return RES_PARERR;               /* Check parameter */
    if (stat & STA_NOINIT) return RES_NOTRDY; /* Check if drive is ready */

    res = RES_ERROR;

    switch (cmd) {
      case CTRL_SYNC: /* Wait for end of internal write process of the drive */
        if (spiselect()) res = RES_OK;
        break;

      case GET_SECTOR_COUNT: /* Get drive capacity in unit of sector (DWORD) */
        if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
          if ((csd[0] >> 6) == 1) { /* SDC ver 2.00 */
            csize =
                csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
            *(DWORD *)buff = csize << 10;
          } else { /* SDC ver 1.XX or MMC ver 3 */
            n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) +
                2;
            csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) +
                    ((WORD)(csd[6] & 3) << 10) + 1;
            *(DWORD *)buff = csize << (n - 9);
          }
          res = RES_OK;
        }
        break;

      case GET_BLOCK_SIZE: /* Get erase block size in unit of sector (DWORD) */
        if (CardType & CT_SD2) {          /* SDC ver 2.00 */
          if (send_cmd(ACMD13, 0) == 0) { /* Read SD status */
            xchg_spi(0xFF);
            if (rcvr_datablock(csd, 16)) { /* Read partial block */
              for (n = 64 - 16; n; n--)
                xchg_spi(0xFF); /* Purge trailing data */
              *(DWORD *)buff = 16UL << (csd[10] >> 4);
              res = RES_OK;
            }
          }
        } else { /* SDC ver 1.XX or MMC */
          if ((send_cmd(CMD9, 0) == 0) &&
              rcvr_datablock(csd, 16)) { /* Read CSD */
            if (CardType & CT_SD1) {     /* SDC ver 1.XX */
              *(DWORD *)buff =
                  (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1)
                  << ((csd[13] >> 6) - 1);
            } else { /* MMC */
              *(DWORD *)buff =
                  ((WORD)((csd[10] & 124) >> 2) + 1) *
                  (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
            }
            res = RES_OK;
          }
        }
        break;

      case CTRL_TRIM: /* Erase a block of sectors (used when _USE_ERASE ==
      1) */
        if (!(CardType & CT_SDC)) break; /* Check if the card is SDC */
        if (disk_ioctl(drv, MMC_GET_CSD, csd)) break; /* Get CSD */
        if (!(csd[0] >> 6) && !(csd[10] & 0x40))
          break; /* Check if sector erase can be applied to the card */
        dp = (DWORD *)buff;
        st = dp[0];
        ed = dp[1]; /* Load sector block */
        if (!(CardType & CT_BLOCK)) {
          st *= 512;
          ed *= 512;
        }
        if (send_cmd(CMD32, st) == 0 && send_cmd(CMD33, ed) == 0 &&
            send_cmd(CMD38, 0) == 0 &&
            wait_ready(30000)) { /* Erase sector block */
          res = RES_OK; /* FatFs does not check result of this command */
        }
        break;

      default:
        res = RES_PARERR;
    }

    despiselect();

    return res;
  }

#endif

 protected:
  volatile DSTATUS stat = STA_NOINIT; /* Physical drive status */
  BYTE CardType;                      /* Card type flags */
  SPIClass *p_spi = &SPI;
  SPISettings spi_slow{280000, MSBFIRST, SPI_MODE0};
  SPISettings spi_fast{FF_SPI_SPEED_FAST, MSBFIRST, SPI_MODE0};
  SPISettings spi_settings;
  uint32_t spi_timeout;
  int cs = -1;

  void spi_timer_on(uint32_t waitTicks) { spi_timeout = millis() + waitTicks; }

  bool spi_timer_status() { return (millis() < spi_timeout); }

  /// set fast/slow SPI speed
  void set_spi_fast(bool fast) { spi_settings = fast ? spi_fast : spi_slow; }

  /// update the CS pin
  inline void set_cs(bool high) {
    if (cs != -1) digitalWrite(cs, high);
  }

  /* Exchange a byte */
  inline BYTE xchg_spi(BYTE dat) { return p_spi->transfer(dat); }

  /* Receive multiple byte */
  void rcvr_spi_multi(BYTE *buff, UINT btr) { 
    // For receiving from SD card, send 0xFF dummy bytes while reading
    for (UINT i = 0; i < btr; i++) {
      buff[i] = xchg_spi(0xFF);
    }
  }
  
  /* Send multiple bytes */
  void xmit_spi_multi(BYTE *buff, UINT btr) {
    p_spi->transfer(buff, btr);
  }

  /* Wait for card ready                                                   */
  int wait_ready(        /* 1:Ready, 0:Timeout */
                 UINT wt /* Timeout [ms] */
  ) {
    BYTE d;
    // wait_ready needs its own timer, unfortunately, so it can't use the
    // spi_timer functions
    uint32_t timeout = millis() + wt;
    do {
      d = xchg_spi(0xFF);
      /* This loop takes a time. Insert rot_rdq() here for multitask
       * envilonment. */
      /* Wait for card goes ready or timeout */
    } while (d != 0xFF && ((millis() < timeout)));

    return (d == 0xFF) ? 1 : 0;
  }

  /* Despiselect card and release SPI                                         */

  void despiselect(void) {
    p_spi->endTransaction();
    set_cs(true);   /* Set CS# high */
    xchg_spi(0xFF); /* Dummy clock (force DO hi-z for multiple slave SPI) */
  }

  /* Select card and wait for ready                                        */

  int spiselect(void) /* 1:OK, 0:Timeout */
  {
    p_spi->beginTransaction(spi_settings);
    set_cs(false);                 /* Set CS# low */
    xchg_spi(0xFF);                /* Dummy clock (force DO enabled) */
    if (wait_ready(500)) return 1; /* Wait for card ready */

    despiselect();
    return 0; /* Timeout */
  }

  /* Receive a data packet from the MMC                                    */

  int rcvr_datablock(            /* 1:OK, 0:Error */
                     BYTE *buff, /* Data buffer */
                     UINT btr    /* Data block length (byte) */
  ) {
    BYTE token;

    uint64_t end = millis() + 200;
    do { /* Wait for DataStart token in timeout of 200ms */
      token = xchg_spi(0xFF);
      /* This loop will take a time. Insert rot_rdq() here for multitask
       * envilonment. */
    } while ((token == 0xFF) && millis() < end);
    if (token != 0xFE)
      return 0; /* Function fails if invalid DataStart token or timeout */

    rcvr_spi_multi(buff, btr); /* Receive data from card */
    xchg_spi(0xFF);
    xchg_spi(0xFF); /* Discard CRC */

    return 1; /* Function succeeded */
  }

  /* Send a data packet to the MMC                                         */

#if FF_IO_USE_WRITE
  int xmit_datablock(            /* 1:OK, 0:Failed */
                     BYTE *buff, /* Ponter to 512 byte data to be sent */
                     BYTE token  /* Token */
  ) {
    BYTE resp;

    if (!wait_ready(500)) return 0; /* Wait for card ready */

    xchg_spi(token);             /* Send token */
    if (token != 0xFD) {         /* Send data if token is other than StopTran */
      xmit_spi_multi(buff, 512); /* Data */
      xchg_spi(0xFF);
      xchg_spi(0xFF); /* Dummy CRC */

      resp = xchg_spi(0xFF); /* Receive data resp */
      if ((resp & 0x1F) != 0x05)
        return 0; /* Function fails if the data packet was not accepted */
    }
    return 1;
  }
#endif

  /* Send a command packet to the MMC                                      */
  BYTE send_cmd(          /* Return value: R1 resp (bit7==1:Failed to send) */
                BYTE cmd, /* Command index */
                DWORD arg /* Argument */
  ) {
    BYTE n, res;

    if (cmd & 0x80) { /* Send a CMD55 prior to ACMD<n> */
      cmd &= 0x7F;
      res = send_cmd(CMD55, 0);
      if (res > 1) return res;
    }

    /* Select the card and wait for ready except to stop multiple block read */
    if (cmd != CMD12) {
      despiselect();
      if (!spiselect()) return 0xFF;
    }

    /* Send command packet */
    xchg_spi(0x40 | cmd);        /* Start + command index */
    xchg_spi((BYTE)(arg >> 24)); /* Argument[31..24] */
    xchg_spi((BYTE)(arg >> 16)); /* Argument[23..16] */
    xchg_spi((BYTE)(arg >> 8));  /* Argument[15..8] */
    xchg_spi((BYTE)arg);         /* Argument[7..0] */
    n = 0x01;                    /* Dummy CRC + Stop */
    if (cmd == CMD0) n = 0x95;   /* Valid CRC for CMD0(0) */
    if (cmd == CMD8) n = 0x87;   /* Valid CRC for CMD8(0x1AA) */
    xchg_spi(n);

    /* Receive command resp */
    if (cmd == CMD12)
      xchg_spi(0xFF); /* Diacard following one byte when CMD12 */
    n = 10;           /* Wait for response (10 bytes max) */
    do {
      res = xchg_spi(0xFF);
    } while ((res & 0x80) && --n);

    return res; /* Return received response */
  }
};

}  // namespace fatfs