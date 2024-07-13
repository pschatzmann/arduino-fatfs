
/**
 * @defgroup io IO
 * @ingroup main
 * @brief Data drivers for fatfs
 */

#pragma once
#include "../ff/ffdef.h"


namespace fatfs {

// forward declaration of FatFs
class FatFs;  // forward declaration


/// Status of Disk Functions 
enum DSTATUS {
 STA_CLEAR=0X00,
 STA_NOINIT=0x01,  /*!<  Drive not initialized */
 STA_NODISK=0x02,  /*!<  No medium in the drive */
 STA_PROTECT=0x04  /*!<  Write protected */
};

/// Results of Disk Functions 
enum DRESULT {
  RES_OK = 0, /*!< 0: Successful */
  RES_ERROR,  /*!< 1: R/W Error */
  RES_WRPRT,  /*!< 2: Write Protected */
  RES_NOTRDY, /*!< 3: Not Ready */
  RES_PARERR  /*!< 4: Invalid Parameter */
};

enum ioctl_cmd_t {
  /* Generic command (Used by FatFs) */
  CTRL_SYNC =
      0, /* Complete pending write process (needed at FF_FS_READONLY == 0) */
  GET_SECTOR_COUNT = 1, /* Get media size (needed at FF_USE_MKFS == 1) */
  GET_SECTOR_SIZE = 2,  /* Get sector size (needed at FF_MAX_SS != FF_MIN_SS) */
  GET_BLOCK_SIZE = 3,   /* Get erase block size (needed at FF_USE_MKFS == 1) \
                         */
  CTRL_TRIM = 4, /* Inform device that the data on the block of sectors is no
         longer used \ (needed at FF_USE_TRIM == 1) */

  /* Generic command (Not used by FatFs) */
  CTRL_POWER = 5,  /* Get/Set power status */
  CTRL_LOCK = 6,   /* Lock/Unlock media removal */
  CTRL_EJECT = 7,  /* Eject media */
  CTRL_FORMAT = 8, /* Create physical format on the media */

  /* MMC/SDC specific ioctl command */
  MMC_GET_TYPE = 10,   /* Get card type */
  MMC_GET_CSD = 11,    /* Get CSD */
  MMC_GET_CID = 12,    /* Get CID */
  MMC_GET_OCR = 13,    /* Get OCR */
  MMC_GET_SDSTAT = 14, /* Get SD status */
  ISDIO_READ = 55,     /* Read data form SD iSDIO register */
  ISDIO_WRITE = 56,    /* Write data to SD iSDIO register */
  ISDIO_MRITE = 57,    /* Masked write data to SD iSDIO register */

  /* ATA/CF specific ioctl command */
  ATA_GET_REV = 20,   /* Get F/W revision */
  ATA_GET_MODEL = 21, /* Get model name */
  ATA_GET_SN = 22     /* Get serial number */
};

/**
 *  @brief FatFS interface definition
 *  @ingroup io
 **/


class IO {
 public:
  /// mount the file system
  virtual FRESULT mount(FatFs& fs);
  /// unmount the file system
  virtual FRESULT un_mount(FatFs& fs);

  virtual DSTATUS disk_initialize(BYTE pdrv) = 0;
  virtual DSTATUS disk_status(BYTE pdrv) = 0;
  virtual DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector,
                            UINT count) = 0;
  virtual DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector,
                             UINT count) = 0;
  virtual DRESULT disk_ioctl(BYTE pdrv, ioctl_cmd_t cmd, void* buff) = 0;

  FATFS fatfs;
};

}  // namespace fatfs