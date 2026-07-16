/* FileIO test: backs a FatFs volume with a plain host OS file (.img) instead
 * of RAM. The point of this driver is persistence across process lifetimes,
 * so the core thing to verify is exactly that: write a file through one
 * FileIO instance, then read it back through a second, independent
 * instance pointing at the same image path - simulating a fresh process
 * reopening a disk image that already has a filesystem on it. Also checks
 * that a brand new image gets auto-formatted on first mount, and that a
 * pre-existing image does NOT get reformatted (which would destroy data).
 */
#include <cstdio>
#include <cstring>

#include "fatfs.h"
#include "driver/FileIO.h"
#include "test_common.h"

using namespace fatfs;

static const char* IMG_PATH = "fatfs_test_fileio.img";

void setup() {
  remove(IMG_PATH);  // defensive cleanup from a previous failed run

  const char* content = "persisted across FileIO instances";
  size_t content_len = strlen(content);

  // first "process": image doesn't exist yet, must be auto-formatted
  {
    FileIO drv(IMG_PATH, 200, 512);
    SDClass sd(drv);
    CHECK(sd.begin(), "first FileIO mount (auto-format) failed");

    File f = sd.open("persist.txt", FILE_WRITE);
    CHECK((bool)f, "could not create file on fresh image");
    CHECK(f.write((const uint8_t*)content, content_len) == content_len,
          "write() returned wrong byte count");
    f.flush();
    f.close();
    sd.end();
  }

  // second "process": same path, fresh FileIO/SDClass instance. Must NOT
  // reformat, and must see exactly what the first instance wrote.
  {
    FileIO drv(IMG_PATH, 200, 512);
    SDClass sd(drv);
    CHECK(sd.begin(), "second FileIO mount (reopen) failed");
    CHECK(sd.exists("persist.txt"), "file missing after reopening the image - "
                                     "it was likely reformatted instead of reopened");

    File f = sd.open("persist.txt", FILE_READ);
    CHECK((bool)f, "could not reopen file written by the first instance");
    CHECK(f.size() == content_len, "file size mismatch after reopening");
    char buf[64] = {0};
    CHECK(f.readBytes((uint8_t*)buf, content_len) == content_len,
          "short read after reopening");
    f.close();
    sd.end();

    CHECK(memcmp(buf, content, content_len) == 0,
          "content mismatch after reopening the image");
  }

  remove(IMG_PATH);

  printf("PASS: FileIO persists data across independent instances\n");
  TEST_EXIT_OK();
}

void loop() {}
