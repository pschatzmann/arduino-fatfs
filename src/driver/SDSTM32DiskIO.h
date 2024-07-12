
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

class SDSTM32DiskIO : public BaseIO {
 public:
  /**
   * @brief  Initializes a Drive
   * @param  lun : not used
   * @retval DSTATUS: Operation status
   */
  DSTATUS disk_initialize(BYTE lun) override {
    stat = STA_NOINIT;
#if !defined(DISABLE_SD_INIT)

    if (BSP_SD_Init() == MSD_OK) {
      stat = SD_CheckStatus(lun);
    }

#else
    Stat = SD_CheckStatus(lun);
#endif
    return stat;
  }

  /**
   * @brief  Gets Disk Status
   * @param  lun : not used
   * @retval DSTATUS: Operation status
   */
  DSTATUS disk_status(BYTE lun) override { return SD_CheckStatus(lun); }

  /**
   * @brief  Reads Sector(s)
   * @param  lun : not used
   * @param  *buff: Data buffer to store read data
   * @param  sector: Sector address (LBA)
   * @param  count: Number of sectors to read (1..128)
   * @retval DRESULT: Operation result
   */
  DRESULT disk_read(BYTE lun, BYTE *buff, DWORD sector, UINT count) {
    (void)lun;
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
 * @param  lun : not used
 * @param  *buff: Data to be written
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to write (1..128)
 * @retval DRESULT: Operation result
 */
#if _USE_WRITE == 1
  DRESULT disk_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count) {
    (void)lun;
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
 * @param  lun : not used
 * @param  cmd: Control code
 * @param  *buff: Buffer to send/receive control data
 * @retval DRESULT: Operation result
 */
#if _USE_IOCTL == 1
  DRESULT disk_ioctl(BYTE lun, ioctl_cmd_t cmd, void *buff) {
    (void)lun;
    DRESULT res = RES_ERROR;
    BSP_SD_CardInfo CardInfo;

    if (stat & STA_NOINIT) return RES_NOTRDY;

    switch (cmd) {
      /* Make sure that no pending write process */
      case CTRL_SYNC:
        res = RES_OK;
        break;

      /* Get number of sectors on the disk (DWORD) */
      case GET_SECTOR_COUNT:
        BSP_SD_GetCardInfo(&CardInfo);
        *(DWORD *)buff = CardInfo.LogBlockNbr;
        res = RES_OK;
        break;

      /* Get R/W sector size (WORD) */
      case GET_SECTOR_SIZE:
        BSP_SD_GetCardInfo(&CardInfo);
        *(WORD *)buff = CardInfo.LogBlockSize;
        res = RES_OK;
        break;

      /* Get erase block size in unit of sector (DWORD) */
      case GET_BLOCK_SIZE:
        BSP_SD_GetCardInfo(&CardInfo);
        *(DWORD *)buff = CardInfo.LogBlockSize / SD_DEFAULT_BLOCK_SIZE;
        res = RES_OK;
        break;

      default:
        res = RES_PARERR;
    }

    return res;
  }
#endif /* _USE_IOCTL == 1 */

protected:
  /* Disk status */
  volatile DSTATUS stat = STA_NOINIT;

  DSTATUS SD_CheckStatus(BYTE lun) {
    (void)lun;
    stat = STA_NOINIT;

    if (BSP_SD_GetCardState() == MSD_OK) {
      stat &= ~STA_NOINIT;
    }

    return stat;
  }
};

}  // namespace fatfs