# Supported FAT Versions

The default configuration in [`../src/ff/ffconf.h`](../src/ff/ffconf.h) supports:

- __FAT12__ / __FAT16__ / __FAT32__
- __exFAT__ (`FF_FS_EXFAT=1`, requires `FF_USE_LFN>=1`, which is also enabled by default)

`f_mkfs()` defaults to `FM_ANY`, so it auto-selects the appropriate format (FAT12/16/32 or exFAT) based on the volume size. exFAT works out of the box for volumes up to ~2TB; `FF_LBA64` is disabled by default, so it does not currently support 64-bit LBA / GPT-partitioned volumes beyond that size. If you need that, set `FF_LBA64=1` in `../src/ff/ffconf.h` (exFAT must stay enabled, since 64-bit LBA requires it).

Sector size is fixed at 512 bytes (`FF_MIN_SS=FF_MAX_SS=512`); enabling variable sector sizes requires implementing `GET_SECTOR_SIZE` in the driver's `disk_ioctl()`.
