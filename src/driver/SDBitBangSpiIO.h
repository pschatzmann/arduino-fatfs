/*------------------------------------------------------------------------/
/  Foolproof MMCv3/SDv1/SDv2 (in SPI mode) control module
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2019, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------/
  Features and Limitations:

  * Easy to Port Bit-banging SPI
    It uses only four GPIO pins. No complex peripheral needs to be used.

  * Platform Independent
    You need to modify only a few macros to control the GPIO port.

  * false Speed
    The data transfer rate will be several times sfalseer than hardware SPI.

  * No Media Change Detection
    Application program needs to perform a f_mount() after media change.

/-------------------------------------------------------------------------*/

#pragma once
#include "../ff/ff.h" /* Obtains integer types for FatFs */
#include "BaseIO.h"
#include "sdcommon.h"

namespace fatfs {

/**
 * @brief Accessing a SD card via SPI using bit banging
 * @ingroup io
 */

class SDBitBangSpiIO : public BaseIO {
 public:
  SDBitBangSpiIO(int miso, int mosi, int clk, int cs = -1) {
    setPins(miso, mosi, clk, cs);
  }

  void setPins(int miso, int mosi, int clk, int cs = -1) {
    this->miso = miso;
    this->mosi = mosi;
    this->clk = clk;
    this->cs = cs;
  }

  void setMISO(int miso) { this->miso = miso; }

  void setMOSI(int mosi) { this->mosi = mosi; }

  void setCLK(int clk) { this->clk = clk; }

  void setCS(int cs) { this->cs = cs; }

  DSTATUS disk_status(BYTE drv) override {
    if (drv) return STA_NOINIT;
    return Stat;
  }

  DSTATUS disk_initialize(BYTE drv) override {
    BYTE n, ty, cmd, buf[4];
    UINT tmr;
    DSTATUS s;

    if (drv != 0) return STA_NODISK;
    if (miso == -1 || mosi == -1 || clk == -1) return STA_NODISK;

    delay(10); /* 10ms */
    setup_pins();

    for (n = 10; n; n--)
      rcvr_mmc(buf, 1); /* Apply 80 dummy clocks and the card gets ready to
                           receive command */

    ty = 0;
    if (send_cmd(CMD0, 0) == 1) {       /* Enter Idle state */
      if (send_cmd(CMD8, 0x1AA) == 1) { /* SDv2? */
        rcvr_mmc(buf, 4); /* Get trailing return value of R7 resp */
        if (buf[2] == 0x01 &&
            buf[3] == 0xAA) { /* The card can work at vdd range of 2.7-3.6V */
          for (tmr = 1000; tmr;
               tmr--) { /* Wait for leaving idle state (ACMD41 with HCS bit) */
            if (send_cmd(ACMD41, 1UL << 30) == 0) break;
            delay(1);
          }
          if (tmr && send_cmd(CMD58, 0) == 0) { /* Check CCS bit in the OCR */
            rcvr_mmc(buf, 4);
            ty = (buf[0] & 0x40) ? CT_SDC2 | CT_BLOCK : CT_SDC2; /* SDv2+ */
          }
        }
      } else { /* SDv1 or MMCv3 */
        if (send_cmd(ACMD41, 0) <= 1) {
          ty = CT_SDC2;
          cmd = ACMD41; /* SDv1 */
        } else {
          ty = CT_MMC3;
          cmd = CMD1; /* MMCv3 */
        }
        for (tmr = 1000; tmr; tmr--) { /* Wait for leaving idle state */
          if (send_cmd(cmd, 0) == 0) break;
          delay(1);
        }
        if (!tmr || send_cmd(CMD16, 512) != 0) /* Set R/W block length to 512 */
          ty = 0;
      }
    }
    CardType = ty;
    s = ty ? STA_CLEAR : STA_NOINIT;
    Stat = s;

    deselect();

    return s;
  }

  DRESULT disk_read(
      BYTE drv,     /* Physical drive nmuber (0) */
      BYTE *buff,   /* Pointer to the data buffer to store read data */
      LBA_t sector, /* Start sector number (LBA) */
      UINT count    /* Sector count (1..128) */
      ) override {
    BYTE cmd;
    DWORD sect = (DWORD)sector;

    if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
    if (!(CardType & CT_BLOCK))
      sect *= 512; /* Convert LBA to byte address if needed */

    cmd = count > 1 ? CMD18
                    : CMD17; /*  READ_MULTIPLE_BLOCK : READ_SINGLE_BLOCK */
    if (send_cmd(cmd, sect) == 0) {
      do {
        if (!rcvr_datablock(buff, 512)) break;
        buff += 512;
      } while (--count);
      if (cmd == CMD18) send_cmd(CMD12, 0); /* STOP_TRANSMISSION */
    }
    deselect();

    return count ? RES_ERROR : RES_OK;
  }

  DRESULT disk_write(BYTE drv,         /* Physical drive nmuber (0) */
                     const BYTE *buff, /* Pointer to the data to be written */
                     LBA_t sector,     /* Start sector number (LBA) */
                     UINT count        /* Sector count (1..128) */
                     ) override {
    DWORD sect = (DWORD)sector;

    if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
    if (!(CardType & CT_BLOCK))
      sect *= 512; /* Convert LBA to byte address if needed */

    if (count == 1) {                  /* Single block write */
      if ((send_cmd(CMD24, sect) == 0) /* WRITE_BLOCK */
          && xmit_datablock(buff, 0xFE))
        count = 0;
    } else { /* Multiple block write */
      if (CardType & CT_SDC) send_cmd(ACMD23, count);
      if (send_cmd(CMD25, sect) == 0) { /* WRITE_MULTIPLE_BLOCK */
        do {
          if (!xmit_datablock(buff, 0xFC)) break;
          buff += 512;
        } while (--count);
        if (!xmit_datablock(0, 0xFD)) /* STOP_TRAN token */
          count = 1;
      }
    }
    deselect();

    return count ? RES_ERROR : RES_OK;
  }

  DRESULT disk_ioctl(BYTE drv,  /* Physical drive nmuber (0) */
                     ioctl_cmd_t ctrl, /* Control code */
                     void *buff /* Buffer to send/receive control data */
                     ) override {
    DRESULT res;
    BYTE n, csd[16];
    DWORD cs;

    if (disk_status(drv) & STA_NOINIT)
      return RES_NOTRDY; /* Check if card is in the socket */

    res = RES_ERROR;
    switch (ctrl) {
      case CTRL_SYNC: /* Make sure that no pending write process */
        if (select()) res = RES_OK;
        break;

      case GET_SECTOR_COUNT: /* Get number of sectors on the disk (DWORD) */
        if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
          if ((csd[0] >> 6) == 1) { /* SDC ver 2.00 */
            cs =
                csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
            *(LBA_t *)buff = cs << 10;
          } else { /* SDC ver 1.XX or MMC */
            n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) +
                2;
            cs = (csd[8] >> 6) + ((WORD)csd[7] << 2) +
                 ((WORD)(csd[6] & 3) << 10) + 1;
            *(LBA_t *)buff = cs << (n - 9);
          }
          res = RES_OK;
        }
        break;

      case GET_BLOCK_SIZE: /* Get erase block size in unit of sector (DWORD) */
        *(DWORD *)buff = 128;
        res = RES_OK;
        break;

      default:
        res = RES_PARERR;
    }

    deselect();

    return res;
  }

 protected:
  DSTATUS Stat = STA_NOINIT; /* Disk status */
  BYTE CardType;             /* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */
  int miso = -1, mosi = -1, clk = -1, cs = -1;

  void setup_pins() {
    if (cs != -1) pinMode(cs, OUTPUT);
    set_pin_active(cs, true); /* Initialize port pin tied to CS */
    pinMode(clk, OUTPUT);
    set_pin_active(clk, false); /* Initialize port pin tied to SCLK */
    pinMode(miso, INPUT);
    pinMode(mosi, OUTPUT);
  }

  inline void set_pin_active(int pin, bool active) {
    if (pin != -1) digitalWrite(pin, active);
  }

  inline bool read_data() { return digitalRead(miso); }

  /// Transmit bytes to the card (bitbanging)                              */
  void xmit_mmc(const BYTE *buff, /* Data to be sent */
                UINT bc           /* Number of bytes to send */
  ) {
    BYTE d;

    do {
      d = *buff++; /* Get a byte to be sent */
      if (d & 0x80)
        set_pin_active(mosi, true);
      else
        set_pin_active(mosi, false); /* bit7 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      if (d & 0x40)
        set_pin_active(mosi, true);
      else
        set_pin_active(mosi, false); /* bit6 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      if (d & 0x20)
        set_pin_active(mosi, true);
      else
        set_pin_active(mosi, false); /* bit5 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      if (d & 0x10)
        set_pin_active(mosi, true);
      else
        set_pin_active(mosi, false); /* bit4 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      if (d & 0x08)
        set_pin_active(mosi, true);
      else
        set_pin_active(mosi, false); /* bit3 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      if (d & 0x04)
        set_pin_active(mosi, true);
      else
        set_pin_active(mosi, false); /* bit2 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      if (d & 0x02)
        set_pin_active(mosi, true);
      else
        set_pin_active(mosi, false); /* bit1 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      if (d & 0x01)
        set_pin_active(mosi, true);
      else
        set_pin_active(mosi, false); /* bit0 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
    } while (--bc);
  }

  /// Receive bytes from the card (bitbanging)                              */
  void rcvr_mmc(BYTE *buff, /* Pointer to read buffer */
                UINT bc     /* Number of bytes to receive */
  ) {
    BYTE r;

    set_pin_active(mosi, true); /* Send 0xFF */

    do {
      r = 0;
      if (read_data()) r++; /* bit7 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      r <<= 1;
      if (read_data()) r++; /* bit6 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      r <<= 1;
      if (read_data()) r++; /* bit5 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      r <<= 1;
      if (read_data()) r++; /* bit4 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      r <<= 1;
      if (read_data()) r++; /* bit3 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      r <<= 1;
      if (read_data()) r++; /* bit2 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      r <<= 1;
      if (read_data()) r++; /* bit1 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      r <<= 1;
      if (read_data()) r++; /* bit0 */
      set_pin_active(clk, true);
      set_pin_active(clk, false);
      *buff++ = r; /* Store a received byte */
    } while (--bc);
  }

  /// Wait for card ready                                                   */
  int wait_ready(void) /* 1:OK, 0:Timeout */
  {
    BYTE d;
    UINT tmr;

    for (tmr = 5000; tmr; tmr--) { /* Wait for ready in timeout of 500ms */
      rcvr_mmc(&d, 1);
      if (d == 0xFF) break;
      delayMicroseconds(100);
    }

    return tmr ? 1 : 0;
  }

  /// Deselect the card and release SPI bus                                 */
  void deselect(void) {
    BYTE d;

    set_pin_active(cs, true); /* Set CS# high */
    rcvr_mmc(&d, 1); /* Dummy clock (force DO hi-z for multiple slave SPI) */
  }

  /// Select the card and wait for ready                                    */
  int select(void) /* 1:OK, 0:Timeout */
  {
    BYTE d;

    set_pin_active(cs, false);  /* Set CS# false */
    rcvr_mmc(&d, 1);            /* Dummy clock (force DO enabled) */
    if (wait_ready()) return 1; /* Wait for card ready */

    deselect();
    return 0; /* Failed */
  }

  /// Receive a data packet from the card                                   */
  int rcvr_datablock(            /* 1:OK, 0:Failed */
                     BYTE *buff, /* Data buffer to store received data */
                     UINT btr    /* Byte count */
  ) {
    BYTE d[2];
    UINT tmr;

    for (tmr = 1000; tmr;
         tmr--) { /* Wait for data packet in timeout of 100ms */
      rcvr_mmc(d, 1);
      if (d[0] != 0xFF) break;
      delayMicroseconds(100);
    }
    if (d[0] != 0xFE) return 0; /* If not valid data token, return with error */

    rcvr_mmc(buff, btr); /* Receive the data block into buffer */
    rcvr_mmc(d, 2);      /* Discard CRC */

    return 1; /* Return with success */
  }

  /// Send a data packet to the card                                        */
  int xmit_datablock(/* 1:OK, 0:Failed */
                     const BYTE
                         *buff, /* 512 byte data block to be transmitted */
                     BYTE token /* Data/Stop token */
  ) {
    BYTE d[2];

    if (!wait_ready()) return 0;

    d[0] = token;
    xmit_mmc(d, 1);              /* Xmit a token */
    if (token != 0xFD) {         /* Is it data token? */
      xmit_mmc(buff, 512);       /* Xmit the 512 byte data block to MMC */
      rcvr_mmc(d, 2);            /* Xmit dummy CRC (0xFF,0xFF) */
      rcvr_mmc(d, 1);            /* Receive data response */
      if ((d[0] & 0x1F) != 0x05) /* If not accepted, return with error */
        return 0;
    }

    return 1;
  }

  /// Send a command packet to the card                                     */
  BYTE send_cmd(          /* Returns command response (bit7==1:Send failed)*/
                BYTE cmd, /* Command byte */
                DWORD arg /* Argument */
  ) {
    BYTE n, d, buf[6];

    if (cmd & 0x80) { /* ACMD<n> is the command sequense of CMD55-CMD<n> */
      cmd &= 0x7F;
      n = send_cmd(CMD55, 0);
      if (n > 1) return n;
    }

    /* Select the card and wait for ready except to stop multiple block read */
    if (cmd != CMD12) {
      deselect();
      if (!select()) return 0xFF;
    }

    /* Send a command packet */
    buf[0] = 0x40 | cmd;        /* Start + Command index */
    buf[1] = (BYTE)(arg >> 24); /* Argument[31..24] */
    buf[2] = (BYTE)(arg >> 16); /* Argument[23..16] */
    buf[3] = (BYTE)(arg >> 8);  /* Argument[15..8] */
    buf[4] = (BYTE)arg;         /* Argument[7..0] */
    n = 0x01;                   /* Dummy CRC + Stop */
    if (cmd == CMD0) n = 0x95;  /* (valid CRC for CMD0(0)) */
    if (cmd == CMD8) n = 0x87;  /* (valid CRC for CMD8(0x1AA)) */
    buf[5] = n;
    xmit_mmc(buf, 6);

    /* Receive command response */
    if (cmd == CMD12) rcvr_mmc(&d, 1); /* Skip a stuff byte when stop reading */
    n = 10; /* Wait for a valid response in timeout of 10 attempts */
    do rcvr_mmc(&d, 1);
    while ((d & 0x80) && --n);

    return d; /* Return with the response value */
  }
};

}  // namespace fatfs