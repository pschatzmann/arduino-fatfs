/* StreamIO test backed by an in-memory "stream". This driver had no test
 * coverage anywhere in the repo before this file.
 *
 * Regression coverage for:
 *  - disk_read/disk_write computing the transfer length as
 *    sectorCount * sector_size (was sectorCount + sector_size, which
 *    truncated/misaligned nearly every real transfer)
 *  - disk_ioctl's GET_SECTOR_COUNT/GET_BLOCK_SIZE copying through
 *    memcpy(buff, &result, ...) (was passing the DWORD value itself as the
 *    source pointer)
 */
#include <cstring>
#include <vector>

#include "fatfs.h"
#include "driver/StreamIO.h"
#include "test_common.h"

using namespace fatfs;

// Minimal in-memory stand-in for the Stream-like type StreamIO<T> expects:
// begin(), seek(), sectorCount(), eraseSector(from, to), plus the
// Stream-style readBytes()/write()/flush() used for the actual transfers.
class MemStream {
 public:
  MemStream(size_t sectors, size_t sectorSize)
      : sector_size(sectorSize), data(sectors * sectorSize, 0) {}

  int sectorSize() { return (int)sector_size; }
  bool begin() { return true; }
  void seek(size_t pos) { position = pos; }

  size_t readBytes(uint8_t* buf, size_t len) {
    size_t avail = position < data.size() ? data.size() - position : 0;
    size_t n = len < avail ? len : avail;
    memcpy(buf, data.data() + position, n);
    position += n;
    return n;
  }

  size_t write(const uint8_t* buf, size_t len) {
    size_t avail = position < data.size() ? data.size() - position : 0;
    size_t n = len < avail ? len : avail;
    memcpy(data.data() + position, buf, n);
    position += n;
    return n;
  }

  void flush() {}
  uint32_t sectorCount() { return (uint32_t)(data.size() / sector_size); }

  void eraseSector(uint32_t from, uint32_t to) {
    memset(data.data() + from * sector_size, 0, (to - from + 1) * sector_size);
  }

 private:
  size_t sector_size;
  std::vector<uint8_t> data;
  size_t position = 0;
};

void setup() {
  // --- low level: exercise the disk_* contract directly ---
  MemStream mem(64, 512);
  StreamIO<MemStream> io(mem);

  CHECK(io.disk_initialize(0) == STA_CLEAR, "disk_initialize failed");

  DWORD sector_count = 0;
  CHECK(io.disk_ioctl(0, GET_SECTOR_COUNT, &sector_count) == RES_OK,
        "GET_SECTOR_COUNT ioctl failed");
  CHECK(sector_count == 64, "GET_SECTOR_COUNT returned wrong value");

  DWORD block_size = 0;
  CHECK(io.disk_ioctl(0, GET_BLOCK_SIZE, &block_size) == RES_OK,
        "GET_BLOCK_SIZE ioctl failed");
  CHECK(block_size == 1, "GET_BLOCK_SIZE returned wrong value");

  // multi-sector write/read: with the old `sectorCount + sector_size` bug
  // this would transfer only ~520 bytes instead of 8*512=4096
  const UINT n_sectors = 8;
  std::vector<uint8_t> pattern(n_sectors * 512);
  for (size_t i = 0; i < pattern.size(); i++) pattern[i] = (uint8_t)(i * 7 + 3);

  CHECK(io.disk_write(0, pattern.data(), 2, n_sectors) == RES_OK,
        "multi-sector disk_write failed");
  CHECK(io.disk_ioctl(0, CTRL_SYNC, nullptr) == RES_OK, "CTRL_SYNC failed");

  std::vector<uint8_t> readback(n_sectors * 512, 0);
  CHECK(io.disk_read(0, readback.data(), 2, n_sectors) == RES_OK,
        "multi-sector disk_read failed");
  CHECK(memcmp(pattern.data(), readback.data(), pattern.size()) == 0,
        "multi-sector round-trip data mismatch");

  printf("PASS: StreamIO low-level diskio contract\n");

  // --- high level: full FatFs stack on top of StreamIO ---
  // unlike RamIO, StreamIO does not auto-format on mount (a real Stream-
  // backed disk is expected to already carry a filesystem); this in-memory
  // stream starts blank, so format it first, same as a user would with a
  // fresh disk image.
  MemStream fsmem(200, 512);
  StreamIO<MemStream> fsio(fsmem);
  SDClass sd(fsio);
  CHECK(sd.mkfs(), "mkfs() over StreamIO failed");
  CHECK(sd.begin(), "SD.begin() over StreamIO failed");

  File f = sd.open("stream.txt", FILE_WRITE);
  CHECK((bool)f, "could not create file on StreamIO-backed volume");
  const char* content = "streamio round trip";
  f.write((const uint8_t*)content, strlen(content));
  f.close();

  File r = sd.open("stream.txt", FILE_READ);
  CHECK((bool)r, "could not reopen file on StreamIO-backed volume");
  char buf[64] = {0};
  size_t n = r.readBytes((uint8_t*)buf, sizeof(buf));
  r.close();
  CHECK(n == strlen(content), "wrong byte count reading back through StreamIO");
  CHECK(memcmp(buf, content, strlen(content)) == 0,
        "content mismatch reading back through StreamIO");

  printf("PASS: StreamIO SDClass/File round-trip\n");
  TEST_EXIT_OK();
}

void loop() {}
