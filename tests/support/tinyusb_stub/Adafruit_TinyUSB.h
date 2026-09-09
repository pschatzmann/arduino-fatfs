// Test-only stand-in for the Adafruit TinyUSB Arduino library's
// Adafruit_USBD_MSC, matching just enough of its interface to let
// TinyUsbMscIO be exercised on the desktop. Not a real USB/TinyUSB
// implementation - the real library and hardware are required for that.
#pragma once
#include <cstdint>
#include <cstddef>

class Adafruit_USBD_MSC {
 public:
  void setID(const char*, const char*, const char*) {}

  void setReadWriteCallback(int32_t (*r)(uint32_t, void*, uint32_t),
                             int32_t (*w)(uint32_t, uint8_t*, uint32_t),
                             void (*f)()) {
    read_cb = r;
    write_cb = w;
    flush_cb = f;
  }

  void setCapacity(uint32_t blocks, uint16_t block_size) {
    last_block_count = blocks;
    last_block_size = block_size;
  }

  void setUnitReady(bool ready) { unit_ready = ready; }
  bool begin() { began = true; return true; }

  // exposed so tests can drive the callbacks the same way TinyUSB would,
  // and assert on the values begin() configured
  int32_t (*read_cb)(uint32_t, void*, uint32_t) = nullptr;
  int32_t (*write_cb)(uint32_t, uint8_t*, uint32_t) = nullptr;
  void (*flush_cb)() = nullptr;
  uint32_t last_block_count = 0;
  uint16_t last_block_size = 0;
  bool unit_ready = false;
  bool began = false;
};
