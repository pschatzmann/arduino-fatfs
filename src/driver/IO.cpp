#include "IO.h"
#include "../ff/ff.h"

namespace fatfs {

FRESULT IO::mount(FatFs& fs) { return fs.f_mount(&fatfs, "", 0); }
/// unmount the file system
FRESULT IO::un_mount(FatFs& fs) { return fs.f_unmount(""); }

}