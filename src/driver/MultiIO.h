#pragma once

#include "IO.h"
#include "fatfs.h"

namespace fatfs {

/**
 * @brief File system driver which supports multiple drives:
 * Add the drivers by calling add()
 * then call mount() to mount the drives.
 * @ingroup io
 */

class MultiIO : public IO {
 public:
  MultiIO() = default;

  void add(IO& io) { io_vector.push_back(&io); }

  /// mount all the added drivers
  bool mount(FatFs& fs) override {
    bool rc = true;
    for (int j = 0; j < io_vector.size(); j++) {
      rc = rc && io_vector[j]->mount(fs);
    }
    return rc;
  }

  /// unmount all drivers
  bool un_mount(FatFs& fs) override {
    bool rc = true;
    for (int j = 0; j < io_vector.size(); j++) {
      rc = rc && io_vector[j]->un_mount(fs);
    }
    return rc;
  }

  DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv >= io_vector.size()) return STA_NODISK;
    return io_vector[pdrv]->disk_initialize(0);
  };
  DSTATUS disk_status(BYTE pdrv) {
    if (pdrv >= io_vector.size()) return STA_NODISK;
    return io_vector[pdrv]->disk_status(0);
  }
  DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv >= io_vector.size()) return RES_NOTRDY;
    return io_vector[pdrv]->disk_read(0, buff, sector, count);
  }
  DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv >= io_vector.size()) return RES_NOTRDY;
    return io_vector[pdrv]->disk_write(0, buff, sector, count);
  }
  DRESULT disk_ioctl(BYTE pdrv, ioctl_cmd_t cmd, void* buff) {
    if (pdrv >= io_vector.size()) return RES_NOTRDY;
    return io_vector[pdrv]->disk_ioctl(0, cmd, buff);
  }

 protected:
  std::vector<IO*> io_vector;
};

}