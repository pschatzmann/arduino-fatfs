// SPDX-License-Identifier: MIT
/**
 * @file MBDClientIO.h
 * @brief FatFs driver that uses a remote NBD export (arduino-mbd NBDClient)
 *        as the storage device.
 *
 * The driver needs the arduino-mbd sources on the include path
 * (the directory that contains `nbd-client/NBDClient.h`).
 */
#pragma once
#include <stdint.h>
#include <string.h>

#include "BaseIO.h"
#include "nbd-client/NBDClient.h"

namespace fatfs {

/**
 * @brief Uses an NBD export as a FatFs disk.
 * @ingroup io
 *
 * The NBD protocol works with byte offsets, so the driver exposes the export
 * as 512 byte sectors: sector N starts at byte offset N * 512.
 *
 * Example:
 * @code
 * #include "fatfs.h"
 * #include "driver/MBDClientIO.h"
 *
 * WiFiClient wifi;
 * nbd::NBDClient nbd_client(wifi);
 * MBDClientIO drv{nbd_client, "192.168.1.10", nbd::NBD_DEFAULT_PORT, "ram"};
 *
 * void setup() {
 *   SD.begin(drv);
 * }
 * @endcode
 */
class MBDClientIO : public BaseIO {
 public:
  /**
   * @brief Uses a client that is already connected (or connects it later
   *        through the client itself).
   * @param client NBD client; must outlive this object
   */
  explicit MBDClientIO(nbd::NBDClient& client) : client(client) {}

  /**
   * @brief Uses a client and connects it to host/port/export on disk_initialize()
   * @param client NBD client; must outlive this object
   * @param host Server host name or address
   * @param port Server port (nbd::NBD_DEFAULT_PORT)
   * @param export_name Export name, "" selects the first export
   */
  MBDClientIO(nbd::NBDClient& client, const char* host,
              uint16_t port = nbd::NBD_DEFAULT_PORT,
              const char* export_name = "")
      : client(client), host(host), port(port), export_name(export_name) {}

  /// Connects to the server if needed and returns the disk status
  DSTATUS disk_initialize(BYTE drv) override {
    if (drv != 0) return STA_NOINIT;
    if (!client.isConnected() && host != nullptr) {
      if (!client.connect(host, port, export_name)) {
        stat = STA_NOINIT;
        return stat;
      }
    }
    stat = client.isConnected() ? STA_CLEAR : STA_NOINIT;
    if (stat == STA_CLEAR && client.isReadOnly()) stat |= STA_PROTECT;
    return stat;
  }

  /// Returns the current disk status
  DSTATUS disk_status(BYTE drv) override {
    if (drv != 0) return STA_NOINIT;
    if (!client.isConnected()) return STA_NOINIT;
    return stat;
  }

  /// Reads count sectors starting at sector
  DRESULT disk_read(BYTE drv, BYTE* buff, LBA_t sector, UINT count) override {
    if (drv != 0 || !count) return RES_PARERR;
    if (!client.isConnected()) return RES_NOTRDY;
    uint64_t offset = (uint64_t)sector * 512;
    uint64_t len = (uint64_t)count * 512;
    if (offset + len > client.size()) return RES_PARERR;
    return transfer(offset, buff, (size_t)len, false) ? RES_OK : RES_ERROR;
  }

#if FF_IO_USE_WRITE
  /// Writes count sectors starting at sector
  DRESULT disk_write(BYTE drv, const BYTE* buff, LBA_t sector,
                     UINT count) override {
    if (drv != 0 || !count) return RES_PARERR;
    if (!client.isConnected()) return RES_NOTRDY;
    if (client.isReadOnly()) return RES_WRPRT;
    uint64_t offset = (uint64_t)sector * 512;
    uint64_t len = (uint64_t)count * 512;
    if (offset + len > client.size()) return RES_PARERR;
    return transfer(offset, const_cast<BYTE*>(buff), (size_t)len, true) ? RES_OK : RES_ERROR;
  }
#endif

#if FF_IO_USE_IOCTL
  /// Miscellaneous drive controls
  DRESULT disk_ioctl(BYTE drv, ioctl_cmd_t cmd, void* buff) override {
    if (drv != 0) return RES_PARERR;
    if (!client.isConnected()) return RES_NOTRDY;

    switch (cmd) {
      case CTRL_SYNC:
        if (client.canFlush() && !client.flush()) return RES_ERROR;
        return RES_OK;

      case GET_SECTOR_COUNT:
        if (!buff) return RES_PARERR;
        *(LBA_t*)buff = (LBA_t)(client.size() / 512);
        return RES_OK;
      case GET_SECTOR_SIZE:
        if (!buff) return RES_PARERR;
        *(WORD*)buff = 512;
        return RES_OK;

      case GET_BLOCK_SIZE:
        if (!buff) return RES_PARERR;
        *(DWORD*)buff = client.preferredBlockSize() / 512;
        if (*(DWORD*)buff == 0) *(DWORD*)buff = 1;
        return RES_OK;

      case CTRL_TRIM:
        if (!buff) return RES_PARERR;
        if (!client.canTrim()) return RES_OK;  // nothing to do
        {
          LBA_t* range = (LBA_t*)buff;  // [first sector, last sector]
          uint64_t offset = (uint64_t)range[0] * 512;
          uint64_t len = ((uint64_t)range[1] - range[0] + 1) * 512;
          return client.trim(offset, (uint32_t)len) ? RES_OK : RES_ERROR;
        }

      default:
        return RES_PARERR;
    }
  }
#endif

  /// Client used by this driver
  nbd::NBDClient& getClient() { return client; }

 protected:
  nbd::NBDClient& client;
  const char* host = nullptr;
  uint16_t port = nbd::NBD_DEFAULT_PORT;
  const char* export_name = "";
  volatile DSTATUS stat = STA_NOINIT;

  /// Reads or writes len bytes at offset, split into the server's max payload
  bool transfer(uint64_t offset, BYTE* data, size_t len, bool write) {
    size_t max = client.maxPayload() > 0 ? (size_t)client.maxPayload()
                                         : (size_t)0xFFFFFFFFUL;
    while (len > 0) {
      size_t n = len < max ? len : max;
      bool ok = write ? client.write(offset, data, n) : client.read(offset, data, n);
      if (!ok) return false;
      offset += n;
      data += n;
      len -= n;
    }
    return true;
  }
};

}  // namespace fatfs
