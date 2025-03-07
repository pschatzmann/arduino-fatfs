#pragma once

#include <stdlib.h>
#include <cstring>
#include <vector>
#include "IO.h"

namespace fatfs {

/**
 * @brief The data is stored in RAM. In a ESP32 when PSRAM has been activated we
 * store it is PSRAM.
 * @ingroup io
 */
class RamIO : public IO {
 public:
  //  valid sector sizes are 512, 1024, 2048 and 4096
  RamIO(int sectorCount, int sectorSize = FF_MAX_SS) {
    sector_size = sectorSize;
    sector_count = sectorCount;
  }

  ~RamIO() {
    for (int j = 0; j <= sector_count; j++) {
      free(sectors[j]);
    }
    delete[]work_buffer;
  }

  // custom logic on mount: we need to format the drive 
  FRESULT mount(FatFs& fs) override;

  DSTATUS disk_initialize(BYTE pdrv) override {
    if (pdrv != 0) return STA_NODISK;
    // allocate sectors
    if (sectors.size() == 0) {
      for (int j = 0; j <= sector_count; j++) {
        uint8_t* ptr = nullptr;
#ifdef ESP32
        ptr = (uint8_t*)ps_malloc(sector_count * sector_size);
#endif
        if (ptr == nullptr) ptr = (uint8_t*)malloc(sector_size);
        memset(ptr, 0, sector_size);
        sectors.push_back(ptr);
      }
    }
    status = STA_CLEAR;
    return status;
  }

  DSTATUS disk_status(BYTE pdrv) override {
    if (pdrv != 0) return STA_NODISK;
    return status;
  }

  DRESULT disk_read(BYTE pdrv, BYTE* buffer, LBA_t sectorNo, UINT sectorCount) override {
    if (pdrv != 0) return RES_NOTRDY;
    if (status == STA_NOINIT) return RES_NOTRDY;
    if (sectors.size() == 0 || sectorNo + sectorCount > sector_count) RES_ERROR;
    for (int j = sectorNo; j < sectorNo + sectorCount; j++) {
      int idx = j - sectorNo;
      uint8_t* source = sectors[j];
      memcpy(buffer + (idx * sector_size), source , sector_size);
    }
    return RES_OK;
  }

  DRESULT disk_write(BYTE pdrv, const BYTE* buffer, LBA_t sectorNo,
                     UINT sectorCount) override {
    if (pdrv != 0) return RES_NOTRDY;
    if (status == STA_NOINIT) return RES_NOTRDY;
    if (sectors.size() == 0 || sectorNo + sectorCount > sector_count) RES_ERROR;
    for (int j = sectorNo; j < sectorNo + sectorCount; j++) {
      int idx = j - sectorNo;
      uint8_t* target = sectors[j];
      memcpy(target, buffer + (idx * sector_size), sector_size);
    }
    return RES_OK;
  }

  DRESULT disk_ioctl(BYTE pdrv, ioctl_cmd_t cmd, void* buffer) override {
    DRESULT res;
    if (pdrv) return RES_PARERR; /* Check parameter */
    switch (cmd) {
      case CTRL_SYNC: /* Wait for end of internal write process of the drive */
        res = RES_OK;
        break;

      case GET_SECTOR_COUNT: { /* Get drive capacity in unit of sector (DWORD)
                                */
        DWORD result = sector_count;
        memcpy(buffer, &result, (sizeof(result)));
        res = RES_OK;
      } break;

      case GET_BLOCK_SIZE: { /* Get erase block size in unit of sector (DWORD)
                              */
        DWORD result = 1;
        memcpy(buffer, &result, (sizeof(result)));
        res = RES_OK;
      } break;

      case CTRL_TRIM: { /* Erase a block of sectors (used when _USE_ERASE == 1)
                         */
        DWORD range[2];
        // determine range
        memcpy(&range, buffer, sizeof(range));
        // clear memory
        for (int j = range[0]; j <= range[1]; j++) {
          memset(sectors[j], 0, sector_size);
        }
        res = RES_OK; /* FatFs does not check result of this command */
      } break;

      default:
        res = RES_PARERR;
        break;
    }
    return res;
  }

 protected:
  std::vector<uint8_t*> sectors;
  DSTATUS status = STA_NOINIT;
  int sector_size = 512;
  size_t sector_count = 0;
  uint8_t *work_buffer = nullptr;
};

}  // namespace fatfs