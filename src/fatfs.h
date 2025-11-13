/*

  SD - a slightly more friendly wrapper for FatFs

  This library aims to expose a subset of SD card functionality
  in the form of a higher level "wrapper" object.

  It uses the same API like the Arduino SD libaray.

  License: GNU General Public License V3
          (Because sdfatlib is licensed with this.)

  (C) Copyright 2022 Phil Schatzmann

*/

/**
 * @defgroup main
 * @brief Arduino fatfs library
 * @author Phil Schatzmann

 * @defgroup sd sd
 * @brief Arduino SD API
 * @ingroup main
 *
 * @defgroup ff ff
 * @brief fatfs basic API
 * @ingroup main
 * 
  *@defgroup iterator iterator
 * @brief directory iterators 
 * @ingroup main
* 
 */
#pragma once

#ifdef ARDUINO
#include <Stream.h>
#else
#include "stream/Stream.h"
#endif

#include "fatfs-drivers.h"
#include "ff/ff.h"

#define FILE_READ FA_READ
#define FILE_WRITE (FA_READ | FA_WRITE | FA_CREATE_ALWAYS | FA_OPEN_APPEND)

namespace fatfs {
using namespace fatfs;

// forward declaration
class SDClass;

/**
 * @brief File implementation for fatfs
 * @ingroup sd
 */

class File : public Stream {
  friend class SDClass;

 public:
  File() = default;
  File(FatFs *fs) { this->fs = fs; }
  ~File() {
    if (is_open) close();
  }

  virtual size_t write(uint8_t ch) override {
    if (fs == nullptr) return 0;
    int rc = fs->f_putc(ch, &file);
    return rc == EOF ? 0 : 1;
  }

  virtual size_t write(const uint8_t *buf, size_t size) override {
    if (fs == nullptr) return 0;
    UINT result;
    FRESULT rc = fs->f_write(&file, buf, size, &result);
    return rc == FR_OK ? result : 0;
  }

  /// Rather inefficient: to be avoided
  virtual int read() override {
    UINT result;
    char buf[1] = {0};
    readBytes((uint8_t *)buf, 1);
    return result == 1 ? buf[0] : -1;
  }

  /// Rather inefficient: to be avoided
  int peek() override {
    uint32_t pos = position();
    int result = read();
    seek(pos);
    return result;
  }

  int available() override { return info.fsize - position(); }

  int availableForWrite() override { return get_free_space(); }

  void flush() override {
    if (!isDirectory()) fs->f_sync(&file);
  }

  virtual size_t readBytes(uint8_t *data, size_t len)  {
    if (isDirectory()) return 0;
    UINT result;
    auto rc = fs->f_read(&file, data, len, &result);
    return rc == FR_OK ? result : 0;
  }

  int read(void *buf, size_t nbyte) { return readBytes((uint8_t *)buf, nbyte); }

  bool seek(uint32_t pos) {
    if (isDirectory()) return 0;
    return fs->f_lseek(&file, pos);
  }

  uint32_t position() {
    if (isDirectory()) return 0;
    return fs->f_tell(&file);
  }

  uint32_t size() { return fs->f_size(&file); }

  void close() {
    if (isDirectory())
      fs->f_closedir(&dir);
    else
      fs->f_close(&file);

    memset(&dir,0,sizeof(dir));
    memset(&file,0,sizeof(file));
    memset(&info, 0, sizeof(info));
    is_open = false;
  }

  char *name() { return info.fname; }

  /// Provides the name: getName() is supported by the SDFat Library
  void getName(char *name, int len) { strncpy(name, info.fname, len); }

  bool isDirectory(void) { return info.fattrib & AM_DIR; }

  File openNextFile(uint8_t mode = FA_READ) {
    File result;
    FRESULT rc = fs->f_findnext(&dir, &result.info);
    if (rc == FR_OK) {
      result.is_open = true;
      if (!result.isDirectory()) {
        fs->f_open(&result.file, result.name(), mode);
      } else {
        fs->f_opendir(&result.dir, result.name());
      }
    }
    return result;
  }
  void rewindDirectory(void) { fs->f_rewinddir(&dir); }

  operator bool() { return is_open && error() == 0; }

  bool isEOF() {
    if (isDirectory()) return false;
    return fs->f_eof(&file);
  }

  uint8_t error() { return fs->f_error(&file); }

  using Print::print;
  using Print::println;
  using Print::write;

  /// Access to low level FatFS api
  FIL *getFIL() { return isDirectory() ? nullptr : &file; }
  /// Access to low level FatFS api
  DIR *getDIR() { return isDirectory() ? &dir : nullptr; }
  /// Access to low level FatFS api to use functionality not exposed by this API
  FatFs *getFatFs() { return fs; }
  /// Access lo low level driver
  IO *getDriver() { return fs->getDriver(); }

 protected:
  FIL file = {0};
  DIR dir = {0};
  FILINFO info = {0};
  FatFs *fs = nullptr;
  bool is_open = false;

  /// update fs, info and is_open
  bool update_stat(FatFs &fat_fs, const char *filepath) {
    is_open = fs->f_stat(filepath, &info) == FR_OK;
    return is_open;
  }

  size_t get_free_space() {
#if FF_FS_MINIMIZE == 0
    DWORD fre_clust, fre_sect;
    FATFS *fatfs;

    // Get volume information and free clusters of default drive
    auto res = fs->f_getfree("", &fre_clust, &fatfs);
    if (res) return 0;

#if FF_MAX_SS != FF_MIN_SS
    size_t sector_size = fil.ssize; /* Sector size (512, 1024, 2048 or 4096) */
#else
    size_t sector_size = FF_MAX_SS;
#endif

    // free sectors * sector size
    return (fre_clust * fatfs->csize) * sector_size;
#else
    return Stream::avaiableForWrite();
#endif
  }
};

/**
 * @brief SDClass: starting driver, access to files
 * @ingroup sd
 */

class SDClass {
 public:
  SDClass() {
#ifdef ARDUINO
    setDriver(drv);
#endif
  };
  SDClass(IO &driver) { setDriver(driver); }
  ~SDClass() { end(); }

  /// Initialization of SD card: Use before other methods
  bool begin(IO &driver) {
    setDriver(driver);
    return begin();
  }

  /// @brief Initialization of SD card. We use the SPI SD driver if nothing has
  /// been defined in the constructor
  bool begin() {
    if (getDriver() == nullptr) return false;
    return handleError(getDriver()->mount(fat_fs));
  }

#ifdef ARDUINO
  /// Compatibility with SD library: we use the Arduino SPI SD driver
  bool begin(int cs, SPIClass &spi = SPI) {
    drv.setSPI(cs, spi);
    return begin();
  }
#endif
  /// call this when a card is removed. It will allow you to insert and
  /// initialise a new card.
  void end() {
    if (getDriver() != nullptr) getDriver()->un_mount(fat_fs);
    delete[] (work_buffer);
    work_buffer = nullptr;
  }

  /// Open the specified file/directory with the supplied mode (e.g. read or
  /// write, etc). Returns a File object for interacting with the file.
  /// Note that currently only one file can be open at a time.
  File open(const char *filename, uint8_t mode = FILE_READ) {
    File file{&fat_fs};
    if (mode & FA_WRITE || file.update_stat(fat_fs, filename)) {
      FRESULT result;
      if (file.isDirectory()) {
        result = fat_fs.f_opendir(&file.dir, filename);
      } else {
        result = fat_fs.f_open(&file.file, filename, mode);
      }
      file.is_open = handleError(result);
    }
    return file;
  }

  File open(const String &filename, uint8_t mode = FILE_READ) {
    return open(filename.c_str(), mode);
  }

  /// Methods to determine if the requested file path exists.
  bool exists(const char *filepath) {
    FILINFO info;
    FRESULT rc = fat_fs.f_stat(filepath, &info); /* Get file status */
    return rc == FR_OK;
  }

  bool exists(const String &filepath) { return exists(filepath.c_str()); }

  /// Create the requested directory heirarchy--if intermediate directories
  /// do not exist they will be created.
  bool mkdir(const char *filepath) { return fat_fs.f_mkdir(filepath) == FR_OK; }
  bool mkdir(const String &filepath) { return mkdir(filepath.c_str()); }

  /// Delete the file.
  bool remove(const char *filepath) {
    return fat_fs.f_unlink(filepath) == FR_OK;
  }
  bool remove(const String &filepath) { return remove(filepath.c_str()); }

  bool rmdir(const char *filepath) {
    return fat_fs.f_unlink(filepath) == FR_OK;
  }
  bool rmdir(const String &filepath) { return rmdir(filepath.c_str()); }

#if FF_ARDUINO_LEVEL == 1
  /// Change directoy: extended functionality not available in Arduino SD API
  bool chdir(const char *filepath) { return fat_fs.f_chdir(filepath) == FR_OK; }
  /// Change directoy: extended functionality not available in Arduino SD API
  bool chdir(String filepath) { return chdir(filepath.c_str()); }
  /// Get current directoy: extended functionality not available in Arduino SD
  /// API
  bool getcwd(char *buff, size_t len) {
    return fat_fs.f_getcwd(buff, len) == FR_OK;
  }

#if FF_USE_MKFS == 1
  /// format drive
  bool mkfs(int workBufferSize = FF_MAX_SS) {
    if (work_buffer == nullptr) work_buffer = new uint8_t[workBufferSize];
    return handleError(fat_fs.f_mkfs("", nullptr, work_buffer, workBufferSize));
  }
#endif

#if FF_FS_MINIMIZE == 0
  /// get free space in bytes
  size_t free() { return File(&fat_fs).availableForWrite(); }
#endif

  /// Access to low level FatFS api to use functionality not exposed by this API
  FatFs *getFatFs() { return &fat_fs; }

  /// Set the driver
  void setDriver(IO &driver) { fat_fs.setDriver(driver); }

  /// Access lo low level driver
  IO *getDriver() { return fat_fs.getDriver(); }

#endif

 protected:
#ifdef ARDUINO
  fatfs::SDArduinoSpiIO drv;
#endif
  FatFs fat_fs;
  uint8_t *work_buffer = nullptr;

  bool handleError(FRESULT rc) {
    if (rc != FR_OK) {
      Serial.print("fatfs: error no: ");
      Serial.println((int)rc);
      return false;
    }
    return true;
  }

#if FF_ARDUINO_LEVEL == 0
  /// Set the driver
  void setDriver(IO &driver) { fat_fs.setDriver(driver); }
  /// Access lo low level driver
  IO *getDriver() { return fat_fs.getDriver(); }

#endif


};

}  // namespace fatfs

// This ensure compatibility with sketches that uses only SD library
#if FF_NAMESPACE
using namespace fatfs;
#endif

static fatfs::SDClass SD;