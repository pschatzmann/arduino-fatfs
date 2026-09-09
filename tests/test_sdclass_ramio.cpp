/* SDClass/File round-trip test on a single RamIO drive.
 * Regression coverage for File::read() (single-byte reads used to compare
 * against an uninitialized variable) and File::readBytes()/write().
 */
#include <cstring>

#include "fatfs.h"
#include "test_common.h"

using namespace fatfs;

RamIO drv{200, 512};

void setup() {
  CHECK(SD.begin(drv), "SD.begin() failed");

  const char* content = "The quick brown fox jumps over the lazy dog";
  size_t content_len = strlen(content);

  File wf = SD.open("test.txt", FILE_WRITE);
  CHECK((bool)wf, "could not create file for writing");
  size_t written = wf.write((const uint8_t*)content, content_len);
  CHECK(written == content_len, "write() returned wrong byte count");
  wf.flush();
  wf.close();

  // bulk read via readBytes()
  File rf = SD.open("test.txt", FILE_READ);
  CHECK((bool)rf, "could not open file for reading");
  CHECK(rf.size() == content_len, "file size mismatch after write");
  uint8_t buf[128] = {0};
  size_t n = rf.readBytes(buf, sizeof(buf));
  CHECK(n == content_len, "readBytes() returned wrong byte count");
  CHECK(memcmp(buf, content, content_len) == 0, "readBytes() content mismatch");
  rf.close();

  // byte-by-byte read() - exercises the File::read() fix directly
  File rf2 = SD.open("test.txt", FILE_READ);
  CHECK((bool)rf2, "could not reopen file for byte-wise reading");
  char rebuilt[128] = {0};
  size_t i = 0;
  int c;
  while ((c = rf2.read()) != -1) {
    CHECK(i < sizeof(rebuilt) - 1, "byte-wise read produced more data than written");
    rebuilt[i++] = (char)c;
  }
  CHECK(i == content_len, "byte-wise read() produced wrong length");
  CHECK(memcmp(rebuilt, content, content_len) == 0, "byte-wise read() content mismatch");
  // one more read() past EOF must consistently report EOF, not stale data
  CHECK(rf2.read() == -1, "read() past EOF did not return -1");
  rf2.close();

  printf("PASS: SDClass/File round-trip on RamIO\n");
  TEST_EXIT_OK();
}

void loop() {}
