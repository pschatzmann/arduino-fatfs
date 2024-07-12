#pragma once
#include "IO.h"

namespace fatfs {

/**
 * @brief Empty IO implementation that we can use to test the compilation
 * @ingroup io
 */
class BaseIO : public IO {
 public:
  virtual DSTATUS disk_initialize(BYTE pdrv) { return STA_NOINIT; }
  virtual DSTATUS disk_status(BYTE pdrv) { return STA_NOINIT; }
  virtual DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    return RES_ERROR;
  }
  virtual DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector,
                             UINT count) {
    return RES_ERROR;
  }
  virtual DRESULT disk_ioctl(BYTE pdrv, ioctl_cmd_t cmd, void* buff) {
    return RES_ERROR;
  }
};

}