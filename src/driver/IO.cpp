#include "IO.h"
#include "../ff/ff.h"

namespace fatfs {

bool IO::mount(FatFs& fs) { return fs.f_mount(&fatfs, "", 0) == FR_OK; }
/// unmount the file system
bool IO::un_mount(FatFs& fs) { return fs.f_unmount("") == FR_OK; }

}