#pragma once
#include "IO.h"

namespace fatfs {

/**
 * @brief Empty IO implementation that we can use to test the compilation
 * @ingroup io
 */
class BaseIO : public IO {
 public:
   DSTATUS disk_initialize(BYTE pdrv) override { return STA_NOINIT; }
   DSTATUS disk_status(BYTE pdrv) override { return STA_NOINIT; }
   DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector,
                            UINT count) override {
    return RES_ERROR;
  }
   DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector,
                             UINT count) override {
    return RES_ERROR;
  }
   DRESULT disk_ioctl(BYTE pdrv, ioctl_cmd_t cmd, void* buff) override {
    return RES_ERROR;
  }
};

}  // namespace fatfs