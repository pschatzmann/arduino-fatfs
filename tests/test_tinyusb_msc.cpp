/* TinyUsbMscIO test, using a minimal test-only stand-in for
 * Adafruit_USBD_MSC (tests/support/tinyusb_stub/Adafruit_TinyUSB.h) since
 * the real TinyUSB stack needs actual USB hardware.
 *
 * Drives the read/write/flush callbacks the same way TinyUSB would when a
 * connected host issues WRITE10/READ10 SCSI commands, backed by a RamIO,
 * and checks that begin() reports the right capacity/sector size and that
 * multi-sector transfers round-trip correctly.
 */
#include <cstring>

#include "fatfs.h"
#include "driver/RamIO.h"
#include "driver/TinyUsbMscIO.h"
#include "test_common.h"

using namespace fatfs;

RamIO ram{64, 512};
TinyUsbMscIO usbMsc;

void setup() {
  CHECK(usbMsc.begin(ram), "TinyUsbMscIO::begin() failed");

  Adafruit_USBD_MSC& msc = usbMsc.getMSC();
  CHECK(msc.began, "begin() did not start the MSC interface");
  CHECK(msc.last_block_count == 64, "wrong block count reported to the host");
  CHECK(msc.last_block_size == 512, "wrong block size reported to the host");
  CHECK(msc.unit_ready, "unit not marked ready after begin()");
  CHECK(msc.read_cb != nullptr && msc.write_cb != nullptr,
        "read/write callbacks not registered");

  // multi-sector WRITE10 then READ10, as a host copying a file would issue
  uint8_t pattern[512 * 3];
  for (size_t i = 0; i < sizeof(pattern); i++)
    pattern[i] = (uint8_t)(i * 3 + 1);

  int32_t wr = msc.write_cb(5, pattern, sizeof(pattern));
  CHECK(wr == (int32_t)sizeof(pattern), "write_cb returned wrong byte count");

  uint8_t readback[512 * 3] = {0};
  int32_t rr = msc.read_cb(5, readback, sizeof(readback));
  CHECK(rr == (int32_t)sizeof(readback), "read_cb returned wrong byte count");
  CHECK(memcmp(pattern, readback, sizeof(pattern)) == 0,
        "data mismatch through simulated USB MSC callbacks");

  msc.flush_cb();  // must not crash with no pending data

  printf("PASS: TinyUsbMscIO round-trip through simulated TinyUSB callbacks\n");
  TEST_EXIT_OK();
}

void loop() {}
