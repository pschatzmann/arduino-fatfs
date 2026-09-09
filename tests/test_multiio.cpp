/* MultiIO dual-drive test: mounts two independent RamIO drives and checks
 * that data written to one drive never leaks into the other.
 *
 * Regression coverage for the mount() path: previously every driver added
 * to MultiIO was mounted at the same logical drive 0 (IO::mount() always
 * used path ""), so only the most-recently-mounted driver's filesystem was
 * ever reachable and all disk I/O silently routed to io_vector[0]
 * regardless of which driver was actually registered.
 */
#include <cstring>

#include "fatfs.h"
#include "driver/RamIO.h"
#include "driver/MultiIO.h"
#include "test_common.h"

using namespace fatfs;

RamIO drvA{200, 512};
RamIO drvB{200, 512};
MultiIO multi;

void setup() {
  multi.add(drvA);
  multi.add(drvB);

  SDClass sd(multi);
  CHECK(sd.begin(), "MultiIO mount failed");

  File f0 = sd.open("0:/a.txt", FILE_WRITE);
  CHECK((bool)f0, "could not create file on drive 0");
  f0.write((const uint8_t*)"hello-drive0", 12);
  f0.close();

  File f1 = sd.open("1:/b.txt", FILE_WRITE);
  CHECK((bool)f1, "could not create file on drive 1");
  f1.write((const uint8_t*)"world-drive1", 12);
  f1.close();

  // drive 0 must not see drive 1's file, and vice versa
  CHECK(!sd.exists("0:/b.txt"), "drive 0 unexpectedly sees drive 1's file");
  CHECK(!sd.exists("1:/a.txt"), "drive 1 unexpectedly sees drive 0's file");

  File r0 = sd.open("0:/a.txt", FILE_READ);
  CHECK((bool)r0, "could not reopen file on drive 0");
  char buf0[13] = {0};
  CHECK(r0.readBytes((uint8_t*)buf0, 12) == 12, "short read on drive 0");
  r0.close();
  CHECK(strcmp(buf0, "hello-drive0") == 0, "drive 0 content mismatch");

  File r1 = sd.open("1:/b.txt", FILE_READ);
  CHECK((bool)r1, "could not reopen file on drive 1");
  char buf1[13] = {0};
  CHECK(r1.readBytes((uint8_t*)buf1, 12) == 12, "short read on drive 1");
  r1.close();
  CHECK(strcmp(buf1, "world-drive1") == 0, "drive 1 content mismatch");

  printf("PASS: MultiIO dual-drive isolation\n");
  TEST_EXIT_OK();
}

void loop() {}
