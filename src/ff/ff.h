/*----------------------------------------------------------------------------/
/  FatFs - Generic FAT Filesystem module  R0.14                               /
/-----------------------------------------------------------------------------/
/
/ Copyright (C) 2019, ChaN, all right reserved.
/
/ FatFs module is an open source software. Redistribution and use of FatFs in
/ source and binary forms, with or without modification, are permitted provided
/ that the following condition is met:

/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/
/----------------------------------------------------------------------------*/

#pragma once
#include <cstdlib>
#include "ffconf.h"  // FatFs configuration options
#include "ffdef.h"   // common structures and defines
#include "driver/IO.h"

namespace fatfs {

/**
 * @brief API for FatFS See http://elm-chan.org/fsw/ff/00index_e.html
 * @ingroup ff
 */

class FatFs {
 public:
  FatFs() = default;
  /// Constructor which is providing the io driver
  FatFs(IO& io) { setDriver(io); }
  void setDriver(IO& io) { p_io = &io; }
  IO* getDriver() {return p_io;}
  /*!<--------------------------------------------------------------*/
  /*!< FatFs module application interface                           */

  FRESULT f_open(FIL* fp, const TCHAR* path,
                 BYTE mode); /*!< Open or create a file */
  FRESULT f_close(FIL* fp);  /*!< Close an open file object */
  FRESULT f_read(FIL* fp, void* buff, UINT btr,
                 UINT* br); /*!< Read data from the file */
  FRESULT f_write(FIL* fp, const void* buff, UINT btw,
                  UINT* bw); /*!< Write data to the file */
  FRESULT f_lseek(FIL* fp,
                  FSIZE_t ofs); /*!< Move file pointer of the file object */
  FRESULT f_truncate(FIL* fp);  /*!< Truncate the file */
  FRESULT f_sync(FIL* fp);      /*!< Flush cached data of the writing file */
  FRESULT f_opendir(DIR* dp, const TCHAR* path); /*!< Open a directory */
  FRESULT f_closedir(DIR* dp);                   /*!< Close an open directory */
  FRESULT f_readdir(DIR* dp, FILINFO* fno);      /*!< Read a directory item */
  FRESULT f_findfirst(DIR* dp, FILINFO* fno, const TCHAR* path,
                      const TCHAR* pattern); /*!< Find first file */
  FRESULT f_findnext(DIR* dp, FILINFO* fno); /*!< Find next file */
  FRESULT f_mkdir(const TCHAR* path);        /*!< Create a sub directory */
  FRESULT f_unlink(
      const TCHAR* path); /*!< Delete an existing file or directory */
  FRESULT f_rename(const TCHAR* path_old,
                   const TCHAR* path_new); /*!< Rename/Move a file or directory */
  FRESULT f_stat(const TCHAR* path, FILINFO* fno); /*!< Get file status */
  FRESULT f_chmod(const TCHAR* path, BYTE attr,
                  BYTE mask); /*!< Change attribute of a file/dir */
  FRESULT f_utime(const TCHAR* path,
                  const FILINFO* fno);     /*!< Change timestamp of a file/dir */
  FRESULT f_chdir(const TCHAR* path);      /*!< Change current directory */
  FRESULT f_chdrive(const TCHAR* path);    /*!< Change current drive */
  FRESULT f_getcwd(TCHAR* buff, UINT len); /*!< Get current directory */
  FRESULT f_getfree(
      const TCHAR* path, DWORD* nclst,
      FATFS** fatfs); /*!< Get number of free clusters on the drive */
  FRESULT f_getlabel(const TCHAR* path, TCHAR* label,
                     DWORD* vsn);         /*!< Get volume label */
  FRESULT f_setlabel(const TCHAR* label); /*!< Set volume label */
  FRESULT f_forward(FIL* fp, UINT (*func)(const BYTE*, UINT), UINT btf,
                    UINT* bf); /*!< Forward data to the stream */
  FRESULT f_expand(FIL* fp, FSIZE_t fsz,
                   BYTE opt); /*!< Allocate a contiguous block to the file */
  FRESULT f_mount(FATFS* fs, const TCHAR* path,
                  BYTE opt); /*!< Mount/Unmount a logical drive */
  FRESULT f_mkfs(const TCHAR* path, const MKFS_PARM* opt, void* work,
                 UINT len); /*!< Create a FAT volume */
  FRESULT f_fdisk(
      BYTE pdrv, const LBA_t ptbl[],
      void* work);          /*!< Divide a physical drive into some partitions */
  FRESULT f_setcp(WORD cp); /*!< Set current code page */
  int f_putc(TCHAR c, FIL* fp);          /*!< Put a character to the file */
  int f_puts(const TCHAR* str, FIL* cp); /*!< Put a string to the file */
  int f_printf(FIL* fp, const TCHAR* str,
               ...); /*!< Put a formatted string to the file */
  TCHAR* f_gets(TCHAR* buff, int len, FIL* fp); /*!< Get a string from the file */

  inline bool f_eof(FIL* fp) {
    return ((int)((fp)->fptr == (fp)->obj.objsize));
  }
  inline BYTE f_error(FIL* fp) { return ((fp)->err); }
  inline FSIZE_t f_tell(FIL* fp) { return ((fp)->fptr); }
  inline FSIZE_t f_size(FIL* fp) { return ((fp)->obj.objsize); }
  inline FRESULT f_rewind(FIL* fp) { return f_lseek((fp), 0); }
  inline FRESULT f_rewinddir(DIR* dp) { return f_readdir((dp), 0); }
  inline FRESULT f_rmdir(const TCHAR* path) { return f_unlink(path); }
  inline FRESULT f_unmount(const TCHAR* path) { return f_mount(0, path, 0); }

/*--------------------------------------------------------------*/
/* Additional user defined functions                            */

/* RTC function */
#if !FF_FS_READONLY && !FF_FS_NORTC
  DWORD get_fattime(void);
#endif

/*!< Sync functions */
#if FF_FS_REENTRANT
  int ff_cre_syncobj(BYTE vol, FF_SYNC_t* sobj); /*!< Create a sync object */
  int ff_req_grant(FF_SYNC_t sobj);              /*!< Lock sync object */
  void ff_rel_grant(FF_SYNC_t sobj);             /*!< Unlock sync object */
  int ff_del_syncobj(FF_SYNC_t sobj);            /*!< Delete a sync object */
#endif

 protected:
  IO* p_io = nullptr;

  /*--------------------------------------------------------------------------

     Module Private Work Area

  ---------------------------------------------------------------------------*/
  /* Remark: Variables defined here without initial value shall be guaranteed
  /  zero/null at start-up. If not, the linker option or start-up routine is
  /  not compliance with C standard. */

  /*--------------------------------*/
  /* File/Volume controls           */
  /*--------------------------------*/

#if FF_VOLUMES < 1 || FF_VOLUMES > 10
#error Wrong FF_VOLUMES setting
#endif
  FATFS* FatFsDir[FF_VOLUMES]; /*!< Pointer to the filesystem objects (logical
                                  drives) */
  WORD Fsid;                   /*!< Filesystem mount ID */

#if FF_FS_RPATH != 0
  BYTE CurrVol; /*!< Current drive */
#endif

#if FF_FS_LOCK != 0
  FILESEM Files[FF_FS_LOCK]; /*!< Open object lock semaphores */
#endif

#if FF_STR_VOLUME_ID
#ifdef FF_VOLUME_STRS
  const char* const VolumeStr[FF_VOLUMES] = {
      FF_VOLUME_STRS}; /*!< Pre-defined volume ID */
#endif
#endif

#if FF_LBA64
#if FF_MIN_GPT > 0x100000000
#error Wrong FF_MIN_GPT setting
#endif
  const BYTE GUID_MS_Basic[16] = {0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9,
                                  0x33, 0x44, 0x87, 0xC0, 0x68, 0xB6,
                                  0xB7, 0x26, 0x99, 0xC7};
#endif

  /*--------------------------------*/
  /* LFN/Directory working buffer   */
  /*--------------------------------*/

#if FF_USE_LFN == 0 /*!< Non-LFN configuration */
#if FF_FS_EXFAT
#error LFN must be enabled when enable exFAT
#endif
#define DEF_NAMBUF
#define INIT_NAMBUF(fs)
#define FREE_NAMBUF()
#define LEAVE_MKFS(res) return res

#else /*!< LFN configurations */
#if FF_MAX_LFN < 12 || FF_MAX_LFN > 255
#error Wrong setting of FF_MAX_LFN
#endif
#if FF_LFN_BUF < FF_SFN_BUF || FF_SFN_BUF < 12
#error Wrong setting of FF_LFN_BUF or FF_SFN_BUF
#endif
#if FF_LFN_UNICODE < 0 || FF_LFN_UNICODE > 3
#error Wrong setting of FF_LFN_UNICODE
#endif
  const BYTE LfnOfs[13] = {
      1,  3,  5,  7,  9,  14, 16,
      18, 20, 22, 24, 28, 30}; /*!< FAT: Offset of LFN characters in the directory
                                  entry */
#define MAXDIRB(nc)                                                         \
  ((nc + 44U) / 15 *                                                        \
   SZDIRE) /*!< exFAT: Size of directory entry block scratchpad buffer needed \
              for the name length */

#if FF_USE_LFN == 1 /*!< LFN enabled with  working buffer */
#if FF_FS_EXFAT
  BYTE
      DirBuf[MAXDIRB(FF_MAX_LFN)]; /*!< Directory entry block scratchpad buffer */
#endif
  WCHAR LfnBuf[FF_MAX_LFN + 1]; /*!< LFN working buffer */
#define DEF_NAMBUF
#define INIT_NAMBUF(fs)
#define FREE_NAMBUF()
#define LEAVE_MKFS(res) return res

#elif FF_USE_LFN == 2 /*!< LFN enabled with dynamic working buffer on the stack \
                       */
#if FF_FS_EXFAT
#define DEF_NAMBUF                                                          \
  WCHAR lbuf[FF_MAX_LFN + 1];                                               \
  BYTE dbuf[MAXDIRB(FF_MAX_LFN)]; /*!< LFN working buffer and directory entry \
                                     block scratchpad buffer */
#define INIT_NAMBUF(fs)  \
  {                      \
    (fs)->lfnbuf = lbuf; \
    (fs)->dirbuf = dbuf; \
  }
#define FREE_NAMBUF()
#else // !FF_FS_EXFAT
#define DEF_NAMBUF WCHAR lbuf[FF_MAX_LFN + 1]; /*!< LFN working buffer */
#define INIT_NAMBUF(fs) \
  { (fs)->lfnbuf = lbuf; }
#define FREE_NAMBUF()
#endif // FF_FS_EXFAT

#define LEAVE_MKFS(res) return res

#elif FF_USE_LFN == 3 /*!< LFN enabled with dynamic working buffer on the heap \
                       */
#if FF_FS_EXFAT
#define DEF_NAMBUF                                                       \
  WCHAR* lfn; /*!< Pointer to LFN working buffer and directory entry block \
                 scratchpad buffer */
#define INIT_NAMBUF(fs)                                            \
  {                                                                \
    lfn = ff_memalloc((FF_MAX_LFN + 1) * 2 + MAXDIRB(FF_MAX_LFN)); \
    if (!lfn) LEAVE_FF(fs, FR_NOT_ENOUGH_CORE);                    \
    (fs)->lfnbuf = lfn;                                            \
    (fs)->dirbuf = (BYTE*)(lfn + FF_MAX_LFN + 1);                  \
  }
#define FREE_NAMBUF() ff_memfree(lfn)
#else
#define DEF_NAMBUF WCHAR* lfn; /*!< Pointer to LFN working buffer */
#define INIT_NAMBUF(fs)                         \
  {                                             \
    lfn = ff_memalloc((FF_MAX_LFN + 1) * 2);    \
    if (!lfn) LEAVE_FF(fs, FR_NOT_ENOUGH_CORE); \
    (fs)->lfnbuf = lfn;                         \
  }
#define FREE_NAMBUF() ff_memfree(lfn)
#endif
#define LEAVE_MKFS(res)         \
  {                             \
    if (!work) ff_memfree(buf); \
    return res;                 \
  }
#define MAX_MALLOC 0x8000 /*!< Must be >=FF_MAX_SS */

#else
#error Wrong setting of FF_USE_LFN
#endif /*!< FF_USE_LFN == 1 */
#endif /*!< FF_USE_LFN == 0 */

  void flush(putbuff* pb);
  void putc_bfd(putbuff* pb, TCHAR c);
  int putc_flush(putbuff* pb);
  FRESULT validate(FFOBJID* obj, FATFS** rfs);
  FRESULT sync_window(FATFS* fs);
  FRESULT move_window(FATFS* fs, LBA_t sect);
  FRESULT sync_fs(FATFS* fs);
  DWORD get_fat(FFOBJID* obj, DWORD clst);
  FRESULT put_fat(FATFS* fs, DWORD clst, DWORD val);
  DWORD find_bitmap(FATFS* fs, DWORD clst, DWORD ncl);
  FRESULT remove_chain(FFOBJID* obj, DWORD clst, DWORD pclst);
  DWORD create_chain(FFOBJID* obj, DWORD clst);
  FRESULT dir_clear(FATFS* fs, DWORD clst);
  FRESULT dir_sdi(DIR* dp, DWORD ofs);
  FRESULT dir_next(DIR* dp, int stretch);
  FRESULT dir_alloc(DIR* dp, UINT nent);
  FRESULT dir_read(DIR* dp, int vol);
  FRESULT dir_find(DIR* dp);
  FRESULT dir_register(DIR* dp);
  FRESULT dir_remove(DIR* dp);
  FRESULT follow_path(DIR* dp, const TCHAR* path);
  UINT check_fs(FATFS* fs, LBA_t sect);
  UINT find_volume(FATFS* fs, UINT part);
  FRESULT mount_volume(const TCHAR** path, FATFS** rfs, BYTE mode);
  int cmp_lfn(const WCHAR* lfnbuf, BYTE* dir);
  int pick_lfn(WCHAR* lfnbuf, BYTE* dir);
  void put_lfn(const WCHAR* lfn, BYTE* dir, BYTE ord, BYTE sum);
  FRESULT create_partition(BYTE drv, const LBA_t plst[], UINT sys, BYTE* buf);
  FRESULT change_bitmap(FATFS* fs, DWORD clst, DWORD ncl, int bv);
  FRESULT fill_first_frag(FFOBJID* obj);
  FRESULT fill_last_frag(FFOBJID* obj, DWORD lcl, DWORD term);
  FRESULT load_xdir(DIR* dp);
  FRESULT load_obj_xdir(DIR* dp, const FFOBJID* obj);
  FRESULT store_xdir(DIR*);
  FRESULT chk_lock(DIR* dp, int acc);
  int enq_lock(void);
  UINT inc_lock(DIR* dp, int acc);
  FRESULT dec_lock(UINT i);
  void clear_lock(FATFS* fs);

#if FF_USE_LFN == 3 /*!< Dynamic memory allocation */

  /*------------------------------------------------------------------------*/
  /* Allocate a memory block */
  /*------------------------------------------------------------------------*/

  void* ff_memalloc(/*!< Returns pointer to the allocated memory block (null
                       if not enough core) */
                    UINT msize /*!< Number of bytes to allocate */
  ) {
    return malloc(msize); /*!< Allocate a new memory block with POSIX API */
  }

  /*------------------------------------------------------------------------*/
  /* Free a memory block                                                    */
  /*------------------------------------------------------------------------*/

  void ff_memfree(void* mblock /*!< Pointer to the memory block to free (nothing
                                  to do if null) */
  ) {
    free(mblock); /*!< Free the memory block with POSIX API */
  }

#endif
};

/* LFN support functions */
#if FF_USE_LFN >= 1 /*!< Code conversion (defined in unicode.c) */
WCHAR ff_oem2uni(WCHAR oem, WORD cp); /*!< OEM code to Unicode conversion */
WCHAR ff_uni2oem(DWORD uni, WORD cp); /*!< Unicode to OEM code conversion */
DWORD ff_wtoupper(DWORD uni);         /*!< Unicode upper-case conversion */
#endif

}  // namespace fatfs

// Header-only library: include implementations
#ifndef FATFS_HEADER_ONLY_IMPL
#define FATFS_HEADER_ONLY_IMPL

// Include FatFs core implementation
#include "ff-inc.h"

// Include system functions (memory allocation, locking)
#include "ffsystem-inc.h"

// Include Unicode conversion tables (if LFN support enabled)
#if FF_USE_LFN >= 1
#include "ffunicode-inc.h"
#endif

#endif  // FATFS_HEADER_ONLY_IMPL