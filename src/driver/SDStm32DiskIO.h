
/**
 ******************************************************************************
 * @file    sd_diskio.c
 * @author  MCD Application Team
 * @brief   SD Disk I/O driver based on BSP v1 api.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2017-2019 STMicroelectronics. All rights reserved.
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                       opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 **/
/* Includes ------------------------------------------------------------------*/
#pragma once

#include "BaseIO.h"
#include "stm32/Sd2Card.h"

#define _USE_WRITE 1 /* 1: Enable disk_write function */
#define _USE_IOCTL 1 /* 1: Enable disk_ioctl function */

/* use the default SD timout as defined in the platform BSP driver*/
#if defined(SDMMC_DATATIMEOUT)
#define SD_TIMEOUT SDMMC_DATATIMEOUT
#elif defined(SD_DATATIMEOUT)
#define SD_TIMEOUT SD_DATATIMEOUT
#else
#define SD_TIMEOUT 30 * 1000
#endif

#define SD_DEFAULT_BLOCK_SIZE 512

namespace fatfs {

/**
 * @brief Depending on the usecase, the SD card initialization could be done at
 * the application level, if it is the case define the flag below to disable the
 * BSP_SD_Init() call in the SD_Initialize().
 * @ingroup io
 */

class SDStm32DiskIO : public BaseIO {
 public:
#if defined(SDMMC1) || defined(SDMMC2)
  SDStm32DiskIO(uint32_t data0, uint32_t data1, uint32_t data2, uint32_t data3,
                uint32_t ck, uint32_t cmd, uint32_t ckin, uint32_t cdir,
                uint32_t d0dir, uint32_t d123dir) {
    card.init(SD_DETECT_NONE, data0, data1, data2, data3, ck, cmd, ckin, cdir,
              d0dir, d123dir);
  }
#else
  SDStm32DiskIO(uint32_t data0, uint32_t data1, uint32_t data2, uint32_t data3,
                uint32_t ck, uint32_t cmd) {
    card.init(SD_DETECT_NONE, data0, data1, data2, data3, ck, cmd);
  }
#endif

#if defined(SDMMC1) || defined(SDMMC2)
  SDStm32DiskIO(uint32_t detect, uint32_t data0, uint32_t data1, uint32_t data2,
                uint32_t data3, uint32_t ck, uint32_t cmd, uint32_t ckin,
                uint32_t cdir, uint32_t d0dir, uint32_t d123dir)
#else
  SDStm32DiskIO(uint32_t detect, uint32_t data0, uint32_t data1, uint32_t data2,
                uint32_t data3, uint32_t ck, uint32_t cmd)
#endif
  {
    card.setDx(data0, data1, data2, data3);
    card.setCK(ck);
    card.setCMD(cmd);
#if defined(SDMMC1) || defined(SDMMC2)
    card.setCKIN(ckin);
    card.setCDIR(cdir);
    card.setDxDIR(d0dir, d123dir);
#endif
    if (detect != SD_DETECT_NONE) {
      PinName p = digitalPinToPinName(detect);
      if ((p == NC) || BSP_SD_DetectPin(set_GPIO_Port_Clock(STM_PORT(p)),
                                        STM_LL_GPIO_PIN(p)) != MSD_OK) {
        return;
      }
    }
#if defined(USE_SD_TRANSCEIVER) && (USE_SD_TRANSCEIVER != 0U)
    PinName sd_en = digitalPinToPinName(SD_TRANSCEIVER_EN);
    PinName sd_sel = digitalPinToPinName(SD_TRANSCEIVER_SEL);
    if (BSP_SD_TransceiverPin(set_GPIO_Port_Clock(STM_PORT(sd_en)),
                              STM_LL_GPIO_PIN(sd_en),
                              set_GPIO_Port_Clock(STM_PORT(sd_sel)),
                              STM_LL_GPIO_PIN(sd_sel)) == MSD_ERROR) {
      return;
    }
#endif
    if (BSP_SD_Init() == MSD_OK) {
      BSP_SD_GetCardInfo(&cardInfo);
      return;
    }
    return;
  }

  /**
   * @brief  Initializes a Drive
   * @param  pdrv : not used
   * @retval DSTATUS: Operation status
   */
  DSTATUS disk_initialize(BYTE pdrv) override {
    status = STA_NOINIT;
#if !defined(DISABLE_SD_INIT)

    if (BSP_SD_Init() == MSD_OK) {
      status = disk_status(pdrv);
    }

#else
    Stat = disk_status(pdrv);
#endif
    return status;
  }

  /**
   * @brief  Gets Disk Status
   * @param  pdrv : not used
   * @retval DSTATUS: Operation status
   */
  DSTATUS disk_status(BYTE pdrv) override {
    if (pdrv != 0) return STA_NODISK;
    status = STA_NOINIT;
    if (BSP_SD_GetCardState() == MSD_OK) {
      status = STA_CLEAR;
    }
    return status;
  }

  /**
   * @brief  Reads Sector(s)
   * @param  pdrv : not used
   * @param  *buff: Data buffer to store read data
   * @param  sector: Sector address (LBA)
   * @param  count: Number of sectors to read (1..128)
   * @retval DRESULT: Operation result
   */
  DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) override {
    if (pdrv != 0) return RES_NOTRDY;
    if (status == STA_NOINIT) return RES_NOTRDY;
    DRESULT res = RES_ERROR;

    if (BSP_SD_ReadBlocks((uint32_t *)buff, (uint32_t)(sector), count,
                          SD_TIMEOUT) == MSD_OK) {
      /* wait until the read operation is finished */
      while (BSP_SD_GetCardState() != MSD_OK) {
      }
      res = RES_OK;
    }

    return res;
  }

/**
 * @brief  Writes Sector(s)
 * @param  pdrv : not used
 * @param  *buff: Data to be written
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to write (1..128)
 * @retval DRESULT: Operation result
 */
#if _USE_WRITE == 1

  DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector,
                     UINT count) override {
    if (pdrv != 0) return RES_NOTRDY;
    if (status == STA_NOINIT) return RES_NOTRDY;
    DRESULT res = RES_ERROR;

    if (BSP_SD_WriteBlocks((uint32_t *)buff, (uint32_t)(sector), count,
                           SD_TIMEOUT) == MSD_OK) {
      /* wait until the Write operation is finished */
      while (BSP_SD_GetCardState() != MSD_OK) {
      }
      res = RES_OK;
    }

    return res;
  }
#endif /* _USE_WRITE == 1 */

/**
 * @brief  I/O control operation
 * @param  pdrv : not used
 * @param  cmd: Control code
 * @param  *buff: Buffer to send/receive control data
 * @retval DRESULT: Operation result
 */
#if _USE_IOCTL == 1
  DRESULT disk_ioctl(BYTE pdrv, ioctl_cmd_t cmd, void *buff) override {
    (void)pdrv;
    DRESULT res = RES_ERROR;

    if (status & STA_NOINIT) return RES_NOTRDY;

    switch (cmd) {
      /* Make sure that no pending write process */
      case CTRL_SYNC:
        res = RES_OK;
        break;

      /* Get number of sectors on the disk (DWORD) */
      case GET_SECTOR_COUNT:
        BSP_SD_GetCardInfo(&cardInfo);
        *(DWORD *)buff = cardInfo.LogBlockNbr;
        res = RES_OK;
        break;

      /* Get R/W sector size (WORD) */
      case GET_SECTOR_SIZE:
        BSP_SD_GetCardInfo(&cardInfo);
        *(WORD *)buff = cardInfo.LogBlockSize;
        res = RES_OK;
        break;

      /* Get erase block size in unit of sector (DWORD) */
      case GET_BLOCK_SIZE:
        BSP_SD_GetCardInfo(&cardInfo);
        *(DWORD *)buff = cardInfo.LogBlockSize / SD_DEFAULT_BLOCK_SIZE;
        res = RES_OK;
        break;

      default:
        res = RES_PARERR;
    }

    return res;
  }
#endif /* _USE_IOCTL == 1 */

 protected:
  // Disk status
  volatile DSTATUS status = STA_NOINIT;
  Sd2Card card;
  BSP_SD_CardInfo cardInfo;
};

}  // namespace fatfs