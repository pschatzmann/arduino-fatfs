/*----------------------------------------------------------------------/
/ Low level disk I/O module function checker                            /
/-----------------------------------------------------------------------/
/ WARNING: The data on the target drive will be lost!
*/

#include <stdio.h>
#include <string.h>
#include "fatfs.h"

RamIO drv{100, 512};  // 100 sector with 512 bytes


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

int test_diskio(BYTE pdrv,   /* Physical drive number to be checked (all data on
                                the drive will be lost) */
                UINT ncyc,   /* Number of test cycles */
                DWORD* buff, /* Pointer to the working buffer */
                UINT sz_buff /* Size of the working buffer in unit of byte */
) {
  UINT n, cc, ns;
  DWORD sz_drv, lba, lba2, sz_eblk, pns = 1;
  WORD sz_sect;
  BYTE* pbuff = (BYTE*)buff;
  DSTATUS ds;
  DRESULT dr;

  printf("test_diskio(%u, %u, 0x%08X, 0x%08X)\n", pdrv, ncyc, (UINT)buff,
         sz_buff);

  if (sz_buff < FF_MAX_SS + 8) {
    printf("Insufficient work area to run the program.\n");
    return 1;
  }

  for (cc = 1; cc <= ncyc; cc++) {
    printf("**** Test cycle %u of %u start ****\n", cc, ncyc);

    printf(" drv.disk_initalize(%u)", pdrv);
    ds = drv.disk_initialize(pdrv);
    if (ds & STA_NOINIT) {
      printf(" - failed.\n");
      return 2;
    } else {
      printf(" - ok.\n");
    }

    printf("**** Get drive size ****\n");
    printf(" drv.disk_ioctl(%u, GET_SECTOR_COUNT, 0x%08X)", pdrv, (UINT)&sz_drv);
    sz_drv = 0;
    dr = drv.disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_drv);
    if (dr == RES_OK) {
      printf(" - ok.\n");
    } else {
      printf(" - failed.\n");
      return 3;
    }
    if (sz_drv < 128) {
      printf("Failed: Insufficient drive size to test.\n");
      return 4;
    }
    printf(" Number of sectors on the drive %u is %lu.\n", pdrv, sz_drv);

#if FF_MAX_SS != FF_MIN_SS
    printf("**** Get sector size ****\n");
    printf(" drv.disk_ioctl(%u, GET_SECTOR_SIZE, 0x%X)", pdrv, (UINT)&sz_sect);
    sz_sect = 0;
    dr = drv.disk_ioctl(pdrv, GET_SECTOR_SIZE, &sz_sect);
    if (dr == RES_OK) {
      printf(" - ok.\n");
    } else {
      printf(" - failed.\n");
      return 5;
    }
    printf(" Size of sector is %u bytes.\n", sz_sect);
#else
    sz_sect = FF_MAX_SS;
#endif

    printf("**** Get block size ****\n");
    printf(" drv.disk_ioctl(%u, GET_BLOCK_SIZE, 0x%X)", pdrv, (UINT)&sz_eblk);
    sz_eblk = 0;
    dr = drv.disk_ioctl(pdrv, GET_BLOCK_SIZE, &sz_eblk);
    if (dr == RES_OK) {
      printf(" - ok.\n");
    } else {
      printf(" - failed.\n");
    }
    if (dr == RES_OK || sz_eblk >= 2) {
      printf(" Size of the erase block is %lu sectors.\n", sz_eblk);
    } else {
      printf(" Size of the erase block is unknown.\n");
    }

    /* Single sector write test */
    printf("**** Single sector write test ****\n");
    lba = 0;
    for (n = 0, pn(pns); n < sz_sect; n++) pbuff[n] = (BYTE)pn(0);
    printf(" drv.disk_write(%u, 0x%X, %lu, 1)", pdrv, (UINT)pbuff, lba);
    dr = drv.disk_write(pdrv, pbuff, lba, 1);
    if (dr == RES_OK) {
      printf(" - ok.\n");
    } else {
      printf(" - failed.\n");
      return 6;
    }
    printf(" drv.disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
    dr = drv.disk_ioctl(pdrv, CTRL_SYNC, 0);
    if (dr == RES_OK) {
      printf(" - ok.\n");
    } else {
      printf(" - failed.\n");
      return 7;
    }
    memset(pbuff, 0, sz_sect);
    printf(" drv.disk_read(%u, 0x%X, %lu, 1)", pdrv, (UINT)pbuff, lba);
    dr = drv.disk_read(pdrv, pbuff, lba, 1);
    if (dr == RES_OK) {
      printf(" - ok.\n");
    } else {
      printf(" - failed.\n");
      return 8;
    }
    for (n = 0, pn(pns); n < sz_sect && pbuff[n] == (BYTE)pn(0); n++);
    if (n == sz_sect) {
      printf(" Read data matched.\n");
    } else {
      printf(" Read data differs from the data written.\n");
      return 10;
    }
    pns++;

    printf("**** Multiple sector write test ****\n");
    lba = 5;
    ns = sz_buff / sz_sect;
    if (ns > 4) ns = 4;
    if (ns > 1) {
      for (n = 0, pn(pns); n < (UINT)(sz_sect * ns); n++)
        pbuff[n] = (BYTE)pn(0);
      printf(" drv.disk_write(%u, 0x%X, %lu, %u)", pdrv, (UINT)pbuff, lba, ns);
      dr = drv.disk_write(pdrv, pbuff, lba, ns);
      if (dr == RES_OK) {
        printf(" - ok.\n");
      } else {
        printf(" - failed.\n");
        return 11;
      }
      printf(" drv.disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
      dr = drv.disk_ioctl(pdrv, CTRL_SYNC, 0);
      if (dr == RES_OK) {
        printf(" - ok.\n");
      } else {
        printf(" - failed.\n");
        return 12;
      }
      memset(pbuff, 0, sz_sect * ns);
      printf(" drv.disk_read(%u, 0x%X, %lu, %u)", pdrv, (UINT)pbuff, lba, ns);
      dr = drv.disk_read(pdrv, pbuff, lba, ns);
      if (dr == RES_OK) {
        printf(" - ok.\n");
      } else {
        printf(" - failed.\n");
        return 13;
      }
      for (n = 0, pn(pns); n < (UINT)(sz_sect * ns) && pbuff[n] == (BYTE)pn(0);
           n++);
      if (n == (UINT)(sz_sect * ns)) {
        printf(" Read data matched.\n");
      } else {
        printf(" Read data differs from the data written.\n");
        return 14;
      }
    } else {
      printf(" Test skipped.\n");
    }
    pns++;

    printf("**** Single sector write test (unaligned buffer address) ****\n");
    lba = 5;
    for (n = 0, pn(pns); n < sz_sect; n++) pbuff[n + 3] = (BYTE)pn(0);
    printf(" drv.disk_write(%u, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff + 3), lba);
    dr = drv.disk_write(pdrv, pbuff + 3, lba, 1);
    if (dr == RES_OK) {
      printf(" - ok.\n");
    } else {
      printf(" - failed.\n");
      return 15;
    }
    printf(" drv.disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
    dr = drv.disk_ioctl(pdrv, CTRL_SYNC, 0);
    if (dr == RES_OK) {
      printf(" - ok.\n");
    } else {
      printf(" - failed.\n");
      return 16;
    }
    memset(pbuff + 5, 0, sz_sect);
    printf(" drv.disk_read(%u, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff + 5), lba);
    dr = drv.disk_read(pdrv, pbuff + 5, lba, 1);
    if (dr == RES_OK) {
      printf(" - ok.\n");
    } else {
      printf(" - failed.\n");
      return 17;
    }
    for (n = 0, pn(pns); n < sz_sect && pbuff[n + 5] == (BYTE)pn(0); n++);
    if (n == sz_sect) {
      printf(" Read data matched.\n");
    } else {
      printf(" Read data differs from the data written.\n");
      return 18;
    }
    pns++;

    printf("**** 4GB barrier test ****\n");
    if (sz_drv >= 128 + 0x80000000 / (sz_sect / 2)) {
      lba = 6;
      lba2 = lba + 0x80000000 / (sz_sect / 2);
      for (n = 0, pn(pns); n < (UINT)(sz_sect * 2); n++) pbuff[n] = (BYTE)pn(0);
      printf(" drv.disk_write(%u, 0x%X, %lu, 1)", pdrv, (UINT)pbuff, lba);
      dr = drv.disk_write(pdrv, pbuff, lba, 1);
      if (dr == RES_OK) {
        printf(" - ok.\n");
      } else {
        printf(" - failed.\n");
        return 19;
      }
      printf(" drv.disk_write(%u, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff + sz_sect),
             lba2);
      dr = drv.disk_write(pdrv, pbuff + sz_sect, lba2, 1);
      if (dr == RES_OK) {
        printf(" - ok.\n");
      } else {
        printf(" - failed.\n");
        return 20;
      }
      printf(" drv.disk_ioctl(%u, CTRL_SYNC, NULL)", pdrv);
      dr = drv.disk_ioctl(pdrv, CTRL_SYNC, 0);
      if (dr == RES_OK) {
        printf(" - ok.\n");
      } else {
        printf(" - failed.\n");
        return 21;
      }
      memset(pbuff, 0, sz_sect * 2);
      printf(" drv.disk_read(%u, 0x%X, %lu, 1)", pdrv, (UINT)pbuff, lba);
      dr = drv.disk_read(pdrv, pbuff, lba, 1);
      if (dr == RES_OK) {
        printf(" - ok.\n");
      } else {
        printf(" - failed.\n");
        return 22;
      }
      printf(" drv.disk_read(%u, 0x%X, %lu, 1)", pdrv, (UINT)(pbuff + sz_sect),
             lba2);
      dr = drv.disk_read(pdrv, pbuff + sz_sect, lba2, 1);
      if (dr == RES_OK) {
        printf(" - ok.\n");
      } else {
        printf(" - failed.\n");
        return 23;
      }
      for (n = 0, pn(pns); pbuff[n] == (BYTE)pn(0) && n < (UINT)(sz_sect * 2);
           n++);
      if (n == (UINT)(sz_sect * 2)) {
        printf(" Read data matched.\n");
      } else {
        printf(" Read data differs from the data written.\n");
        return 24;
      }
    } else {
      printf(" Test skipped.\n");
    }
    pns++;

    printf("**** Test cycle %u of %u completed ****\n\n", cc, ncyc);
  }

  return 0;
}

void setup() {
  int rc;
  DWORD buff[FF_MAX_SS]; /* Working buffer (4 sector in size) */

  /* Check function/compatibility of the physical drive #0 */
  rc = test_diskio(0, 3, buff, sizeof buff);

  if (rc) {
    printf(
        "Sorry the function/compatibility test failed. (rc=%d)\nFatFs will not "
        "work with this disk driver.\n",
        rc);
  } else {
    printf("Congratulations! The disk driver works well.\n");
  }
}

void loop() {}
