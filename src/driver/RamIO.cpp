// separate cpp is needed because of recursive includes

#include "RamIO.h"
#include "../ff/ff.h"

namespace fatfs {

FRESULT RamIO::mount(FatFs& fs) {
  // the file system is empty so we need to format it
  if (work_buffer == nullptr) work_buffer = new uint8_t[FF_MAX_SS];
  fs.f_mkfs("", nullptr, work_buffer, FF_MAX_SS);

  // standard mount logic
  return IO::mount(fs);
}

}  // namespace fatfs