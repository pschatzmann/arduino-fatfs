/*----------------------------------------------------------------------/
/ Low level disk I/O module function checker (adapted from ChaN's        /
/ FatFs diskio conformance checker, examples/driver-test-ram)            /
/-----------------------------------------------------------------------/
/ Drives RamIO::disk_initialize/disk_read/disk_write/disk_ioctl directly,
/ bypassing FatFs mounting. Exercises the sector-count/sector-size math
/ and bounds checks fixed in RamIO.
*/

#include <stdio.h>
#include <string.h>

#include "fatfs.h"
#include "test_common.h"

using namespace fatfs;

RamIO drv{200, 512};  // 200 sectors with 512 bytes each

static DWORD pn(          /* Pseudo random number generator */
                DWORD pns /* 0:Initialize, !0:Read */
) {
  static DWORD lfsr;
  UINT n;

  if (pns) {
    lfsr = pns;
    for (n = 0; n < 32; n++) pn(0);
  }
  if (lfsr & 1) {
    lfsr >>= 1;
    lfsr ^= 0x80200003;
  } else {
    lfsr >>= 1;
  }
  return lfsr;
}

int test_diskio(BYTE pdrv, UINT ncyc, DWORD* buff, UINT sz_buff) {
  UINT n, cc, ns;
  DWORD sz_drv, lba, lba2, sz_eblk, pns = 1;
  WORD sz_sect;
  BYTE* pbuff = (BYTE*)buff;
  DSTATUS ds;
  DRESULT dr;

  printf("test_diskio(%u, %u, %p, %u)\n", pdrv, ncyc, (void*)buff, sz_buff);

  if (sz_buff < FF_MAX_SS + 8) {
    printf("Insufficient work area to run the program.\n");
    return 1;
  }

  for (cc = 1; cc <= ncyc; cc++) {
    printf("**** Test cycle %u of %u start ****\n", cc, ncyc);

    ds = drv.disk_initialize(pdrv);
    if (ds & STA_NOINIT) return 2;

    sz_drv = 0;
    dr = drv.disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_drv);
    if (dr != RES_OK) return 3;
    if (sz_drv < 128) return 4;

#if FF_MAX_SS != FF_MIN_SS
    sz_sect = 0;
    dr = drv.disk_ioctl(pdrv, GET_SECTOR_SIZE, &sz_sect);
    if (dr != RES_OK) return 5;
#else
    sz_sect = FF_MAX_SS;
#endif

    sz_eblk = 0;
    dr = drv.disk_ioctl(pdrv, GET_BLOCK_SIZE, &sz_eblk);
    (void)dr;

    /* Single sector write test */
    lba = 0;
    for (n = 0, pn(pns); n < sz_sect; n++) pbuff[n] = (BYTE)pn(0);
    dr = drv.disk_write(pdrv, pbuff, lba, 1);
    if (dr != RES_OK) return 6;
    dr = drv.disk_ioctl(pdrv, CTRL_SYNC, 0);
    if (dr != RES_OK) return 7;
    memset(pbuff, 0, sz_sect);
    dr = drv.disk_read(pdrv, pbuff, lba, 1);
    if (dr != RES_OK) return 8;
    for (n = 0, pn(pns); n < sz_sect && pbuff[n] == (BYTE)pn(0); n++);
    if (n != sz_sect) return 10;
    pns++;

    /* Multiple sector write test - exercises sectorCount * sector_size math */
    lba = 5;
    ns = sz_buff / sz_sect;
    if (ns > 4) ns = 4;
    if (ns > 1) {
      for (n = 0, pn(pns); n < (UINT)(sz_sect * ns); n++)
        pbuff[n] = (BYTE)pn(0);
      dr = drv.disk_write(pdrv, pbuff, lba, ns);
      if (dr != RES_OK) return 11;
      dr = drv.disk_ioctl(pdrv, CTRL_SYNC, 0);
      if (dr != RES_OK) return 12;
      memset(pbuff, 0, sz_sect * ns);
      dr = drv.disk_read(pdrv, pbuff, lba, ns);
      if (dr != RES_OK) return 13;
      for (n = 0, pn(pns); n < (UINT)(sz_sect * ns) && pbuff[n] == (BYTE)pn(0);
           n++);
      if (n != (UINT)(sz_sect * ns)) return 14;
    }
    pns++;

    /* Single sector write test (unaligned buffer address) */
    lba = 5;
    for (n = 0, pn(pns); n < sz_sect; n++) pbuff[n + 3] = (BYTE)pn(0);
    dr = drv.disk_write(pdrv, pbuff + 3, lba, 1);
    if (dr != RES_OK) return 15;
    dr = drv.disk_ioctl(pdrv, CTRL_SYNC, 0);
    if (dr != RES_OK) return 16;
    memset(pbuff + 5, 0, sz_sect);
    dr = drv.disk_read(pdrv, pbuff + 5, lba, 1);
    if (dr != RES_OK) return 17;
    for (n = 0, pn(pns); n < sz_sect && pbuff[n + 5] == (BYTE)pn(0); n++);
    if (n != sz_sect) return 18;
    pns++;

    /* out-of-range access must be rejected cleanly, not crash
       (regression test for the missing `return` on the bounds check) */
    dr = drv.disk_read(pdrv, pbuff, sz_drv, 1);
    if (dr == RES_OK) return 25;
    dr = drv.disk_write(pdrv, pbuff, sz_drv, 1);
    if (dr == RES_OK) return 26;

    (void)lba2;
    printf("**** Test cycle %u of %u completed ****\n\n", cc, ncyc);
  }

  return 0;
}

void setup() {
  DWORD buff[FF_MAX_SS]; /* Working buffer (4 sector in size) */

  int rc = test_diskio(0, 3, buff, sizeof buff);
  CHECK(rc == 0, "RamIO diskio conformance test failed");

  printf("PASS: RamIO diskio conformance\n");
  TEST_EXIT_OK();
}

void loop() {}
