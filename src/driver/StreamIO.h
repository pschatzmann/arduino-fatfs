#pragma once
#include "IO.h"

namespace fatfs {

/**
 * @brief template class which expects a Stream class which provides the
 * following additional methods
    - begin()
    - seek()
    - sectorCount()
    - eraseSector(from, to)
 * @ingroup io
 */

template <class T>
class StreamIO : public IO {
 public:
  StreamIO(T& ref) {
    p_stream = &ref;
    sector_size = ref.sectorSize();
  }

  DSTATUS disk_initialize(BYTE pdrv) {
    // we support only 1 disk
    if (pdrv != 0) return STA_NODISK;
    ok = p_stream->begin();
    if (!ok) return STA_NODISK;
    status = STA_CLEAR;
    return status;
  }

  DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) return STA_NODISK;
    return status;
  }

  DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT sectorCount) {
    if (pdrv != 0) return RES_PARERR;
    if (status == STA_NOINIT) return RES_NOTRDY;
    p_stream->seek(sector * sector_size);
    size_t len = sectorCount + sector_size;
    size_t res = p_stream->readBytes(buff, len);
    return res == len ? RES_OK : RES_ERROR;
  }

  DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector,
                     UINT sectorCount) {
    if (pdrv != 0) return RES_PARERR;
    if (status == STA_NOINIT) return RES_NOTRDY;
    p_stream->seek(sector * sector_size);
    size_t len = sectorCount + sector_size;
    size_t res = p_stream->write(buff, len);
    return res == len ? RES_OK : RES_ERROR;
  }

  DRESULT disk_ioctl(BYTE pdrv, ioctl_cmd_t cmd, void* buff) {
    DRESULT res;
    if (pdrv) return RES_PARERR; /* Check parameter */
    switch (cmd) {
      case CTRL_SYNC:  // Wait for end of internal write process of the drive
        res = RES_OK;
        p_stream->flush();
        break;

      case GET_SECTOR_COUNT: {  // Get drive capacity in unit of sector (DWORD)
        DWORD result = p_stream->sectorCount();
        memcpy(buff, result, (sizeof(result)));
        res = RES_OK;
      } break;

      case GET_BLOCK_SIZE: {  // Get erase block size in unit of sector (DWORD)
        DWORD result = 1;
        memcpy(buff, result, (sizeof(result)));
        res = RES_OK;
      } break;

      case CTRL_TRIM: {  // Erase a block of sectors (used when _USE_ERASE == 1)
        DWORD range[2];
        // determine range
        memcpy(range, buff, sizeof(range));
        // clear memory
        p_stream->eraseSector(range[0], range[1]);
        res = RES_OK; /* FatFs does not check result of this command */
      } break;

      default:
        res = RES_PARERR;
        break;
    }
    return res;
  }

 protected:
  T* p_stream = nullptr;
  bool ok = false;
  int sector_size = 512;
  DSTATUS status = STA_NOINIT;
};

}