// SPDX-License-Identifier: MIT
#pragma once

#if defined(USE_TINYUSB)

#include "IO.h"
#include <Adafruit_TinyUSB.h>

namespace fatfs {

/**
 * @brief Exposes an existing block device driver (e.g. RamIO, ArduinoSpiIO,
 * Esp32SdmmcIO) as a USB Mass Storage Class device via TinyUSB, so a host PC
 * connected over USB can mount it directly as a removable drive.
 *
 * Unlike the other drivers in this library, TinyUsbMscIO does not implement
 * the IO interface itself: FatFs never calls into it. Instead it is a
 * consumer of an existing IO&, answering read/write requests that arrive
 * from the USB host via TinyUSB's MSC callbacks. It can be used standalone
 * (just export a driver to the USB host) or alongside a local SD.begin(),
 * as long as both sides don't write concurrently.
 *
 * Only one instance can be active at a time: TinyUSB's Adafruit_USBD_MSC
 * callback API takes plain function pointers with no per-instance context,
 * so this class keeps the active driver in static storage.
 *
 * Requires the Adafruit TinyUSB Arduino core/library (USE_TINYUSB defined).
 * Bringing up the USB stack itself (e.g. TinyUSBDevice.begin()) is left to
 * the sketch, the same way ArduinoSpiIO leaves SPI.begin() to the sketch.
 *
 * @ingroup io
 */
class TinyUsbMscIO {
 public:
  TinyUsbMscIO() = default;
  explicit TinyUsbMscIO(IO& driver) { begin(driver); }

  /// Vendor/product/revision strings shown to the USB host - call before begin()
  void setID(const char* vendor, const char* product, const char* rev) {
    msc.setID(vendor, product, rev);
  }

  /// Registers the driver and starts the USB MSC interface
  bool begin(IO& driver) {
    p_io = &driver;

    if (p_io->disk_initialize(0) & STA_NOINIT) return false;

    DWORD sector_count = 0;
    if (p_io->disk_ioctl(0, GET_SECTOR_COUNT, &sector_count) != RES_OK)
      return false;

#if FF_MAX_SS != FF_MIN_SS
    WORD sect_size = 0;
    if (p_io->disk_ioctl(0, GET_SECTOR_SIZE, &sect_size) != RES_OK)
      return false;
    sector_size = sect_size;
#endif

    msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
    msc.setCapacity(sector_count, sector_size);
    msc.setUnitReady(true);
    return msc.begin();
  }

  /// Marks the media as (not) present, e.g. to signal ejection to the host
  void setUnitReady(bool ready) { msc.setUnitReady(ready); }

  /// Access to the underlying Adafruit_USBD_MSC instance for anything not
  /// wrapped here (setMaxLun(), isEjected(), ...)
  Adafruit_USBD_MSC& getMSC() { return msc; }

 protected:
  Adafruit_USBD_MSC msc;
  static inline IO* p_io = nullptr;
  static inline uint16_t sector_size = FF_MAX_SS;

  static int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize) {
    if (p_io == nullptr) return -1;
    UINT count = bufsize / sector_size;
    DRESULT res = p_io->disk_read(0, (BYTE*)buffer, lba, count);
    return res == RES_OK ? (int32_t)bufsize : -1;
  }

  static int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
    if (p_io == nullptr) return -1;
    UINT count = bufsize / sector_size;
    DRESULT res = p_io->disk_write(0, buffer, lba, count);
    return res == RES_OK ? (int32_t)bufsize : -1;
  }

  static void msc_flush_cb() {
    if (p_io != nullptr) p_io->disk_ioctl(0, CTRL_SYNC, nullptr);
  }
};

}  // namespace fatfs

#endif  // USE_TINYUSB
