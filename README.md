# Arduino-FatFs

[![Arduino Library](https://img.shields.io/badge/Arduino-Library-blue.svg)](https://www.arduino.cc/reference/en/libraries/)
[![Build: CMake](https://img.shields.io/badge/Build-CMake-064F8C.svg?logo=cmake)](CMakeLists.txt)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE.txt)

There are quite a few SD Arduino libraries out there: the most important is the [SD.h provided by Arduino](https://github.com/arduino-libraries/SD) which is a wrapper for [SdFat from Bill Greiman](https://github.com/greiman/SdFat) which itself is quite friendly, powerfull and fast.

Bill's library provides some alternative SPI drivers to access the SD functionality, but it does not provide the functionality to store the data anywhere else and you can't use multiple SD drives that are attached to different SPI ports.

I am providing the [FatFs library](http://elm-chan.org/fsw/ff/00index_e.html) developed by ChaN that I have converted to C++ header only so that we can flexibly support multiple data access drivers and scenarios at the same time.

The advantage of this library is, that it provides quite a few __[configuration options](http://elm-chan.org/fsw/ff/doc/config.html#use_mkfs)__ and has a flexible __driver concept__, so that we can store the data potentially on the SD, in SPI RAM, RAM, PSRAM etc. The FatFs library stops at the driver interface and does not provide any implementation. 

I have added the most important drivers to this project:  The drivers are written in a flexible way and do not use any predefed fixed pins or ports: e.g. on the SPI driver you can assign the pins as part of SPI, define the CS pin and assign your desired SPI object (e.g. SPI, SPI1, SPI2 etc). We currently provide the following __driver implementations__:

- The data is stored in __RAM (or PSRAM)__ (RamIO)
- The data is stored in a __host OS file__/disk image, desktop builds only (FileIO)
- Support for __multiple drives__ with different drivers (MultiIO)
- SD via Arduino __SPI__ (ArduinoSpiIO)
- Export any driver as a __USB Mass Storage__ device via TinyUSB (TinyUsbMscIO)

It is very easy to add new drivers, so any contribution will be welcome...

## Supported FAT Versions

The default configuration in [`src/ff/ffconf.h`](src/ff/ffconf.h) supports:

- __FAT12__ / __FAT16__ / __FAT32__
- __exFAT__ (`FF_FS_EXFAT=1`, requires `FF_USE_LFN>=1`, which is also enabled by default)

`f_mkfs()` defaults to `FM_ANY`, so it auto-selects the appropriate format (FAT12/16/32 or exFAT) based on the volume size. exFAT works out of the box for volumes up to ~2TB; `FF_LBA64` is disabled by default, so it does not currently support 64-bit LBA / GPT-partitioned volumes beyond that size. If you need that, set `FF_LBA64=1` in `src/ff/ffconf.h` (exFAT must stay enabled, since 64-bit LBA requires it).

Sector size is fixed at 512 bytes (`FF_MIN_SS=FF_MAX_SS=512`); enabling variable sector sizes requires implementing `GET_SECTOR_SIZE` in the driver's `disk_ioctl()`.

## SPI SD

Here is an example of setting up a SD drive using the Arduino ESP32 SPI API:

```C++
#include "SPI.h"
#include "fatfs.h"

#define MISO 12
#define MOSI 13
#define SCLK 14
#define CS   15

ArduinoSpiIO drv{CS, SPI}; // SD driver managing CS and assign SPI
File file;

void setup() {
    // start SPI and setup pins
    SPI.begin(SCLK, MISO, MOSI);
    // start SD 
    SD.begin(drv); 

    file = SD.open("test");
    Serial.println(file.size());
}

void loop() {}

```
This example demonstates the most generic way to set up things. Of cause it is still possible to do it the way the Arduino SD library posposes (and you don't need to define a driver for using SD SPI yourself): just call ```SD.begin(CS);```


## RAM Drive

Here is an example of setting up a SD drive in RAM:

```C++
#include "SPI.h"
#include "fatfs.h"

RamIO drv{100, 512}; // 100 sector with 512 bytes
File file;

void setup() {
    // start SD 
    SD.begin(drv); 

    file = SD.open("test");
    Serial.println(file.size());
}

void loop() {}

```

## File-backed Disk Image (desktop/native builds)

`FileIO` is like `RamIO`, but backed by a plain host OS file instead of RAM - it's for desktop/native builds only (guarded by `#ifndef ARDUINO`), mainly useful for tests and tooling that run on a PC rather than a microcontroller. Unlike `RamIO`, the data survives past the lifetime of one process: mounting an existing image reopens the filesystem already on it instead of reformatting it, and the resulting `.img` file can be inspected with ordinary OS tools (e.g. `mtools`' `mdir -i disk.img@@32256 ::`, `fsck.vfat`, a loopback mount, ...).

```C++
#include "fatfs.h"
#include "driver/FileIO.h"

FileIO drv{"disk.img", 2048, 512}; // 1MB image, created if it doesn't exist yet
File file;

void setup() {
    // start SD - auto-formats disk.img only the first time it's created
    SD.begin(drv);

    file = SD.open("test", FILE_WRITE);
    file.println("hello");
    file.close();
}

void loop() {}

```


## Mutliple Drives


You can also use your own separate SDClass instances:

```C++
#include "SPI.h"
#include "fatfs.h"

#define MISO 12
#define MOSI 13
#define SCLK 14
#define CS   15

ArduinoSpiIO sd{CS, SPI}; // driver managing CS and assign SPI
RamIO mem{100, 512}; // 100 sector with 512 bytes
SDClass sd_sd{sd}; // SD and assign driver
SDClass sd_mem{mem};
File file;

void setup() {
    // start SPI and setup pins
    SPI.begin(SCLK, MISO, MOSI);

    // start SD 
    sd_mem.begin(); 
    sd_sd.begin(); 

    auto file1 = sd_mem.open("0:test");
    Serial.println(file1.size());
    auto file2 = sd_sd.open("1:test");
    Serial.println(file2.size());
}

void loop() {}

```

Here is an example of setting up a multi drive scenario using the MultiIO diver:

```C++
#include "SPI.h"
#include "fatfs.h"
#include "driver/ArduinoSpiIO.h"
#include "driver/RamIO.h"
#include "driver/MultiIO.h"

#define MISO 12
#define MOSI 13
#define SCLK 14
#define CS   15

ArduinoSpiIO sd{CS, SPI}; // driver managing CS and assign SPI
RamIO mem{100, 512}; // 100 sector with 512 bytes
MultiIO drv;

void setup() {
    // start SPI and setup pins
    SPI.begin(SCLK, MISO, MOSI);

    // setup MultiIO
    drv.add(sd);
    drv.add(mem);

    // start all drives 
    SD.begin(drv); 

    auto file1 = SD.open("0:test");
    Serial.println(file1.size());
    auto file2 = SD.open("1:test");
    Serial.println(file2.size());
}

void loop() {}

```

## USB Mass Storage (TinyUSB)

`TinyUsbMscIO` exposes an existing driver (RamIO, ArduinoSpiIO, ...) directly to a host PC over USB as a mass storage device, using the [Adafruit TinyUSB library](https://github.com/adafruit/Adafruit_TinyUSB_Arduino). Unlike the other drivers, it doesn't implement the `IO` interface itself - FatFs never calls into it. Instead it forwards USB read/write requests coming *from* the host straight to an existing `IO&`'s `disk_read()`/`disk_write()`. It can be used standalone (just export storage to the host, no local FatFs mount needed) or together with a local `SD.begin()`, as long as both sides aren't writing at the same time.

Requires a TinyUSB-capable board/core (e.g. RP2040, SAMD21/51, nRF52, ESP32-S2/S3) with `USE_TINYUSB` defined; bringing up the USB stack itself is left to the sketch, the same way `ArduinoSpiIO` leaves `SPI.begin()` to the sketch.


# Documentaion

- [Arduino SD API](https://www.arduino.cc/reference/en/libraries/sd/)
- [FatFS Documentation](http://elm-chan.org/fsw/ff/00index_e.html)
- Class Documentation
    - [Arduino API](https://pschatzmann.github.io/arduino-fatfs/html/group__sd.html)
    - [FatFs API](https://pschatzmann.github.io/arduino-fatfs/html/classfatfs_1_1FatFs.html)
    - [Directory Iterators](https://pschatzmann.github.io/arduino-fatfs/html/group__iterator.html)
    - [Drivers](https://pschatzmann.github.io/arduino-fatfs/html/group__io.html)

# License

The wrapper code in this repository (everything outside `src/ff/`) is licensed under the [MIT License](LICENSE.txt). It bundles [ChaN's FatFs](http://elm-chan.org/fsw/ff/00index_e.html) core (`src/ff/`) under FatFs's own permissive terms, reproduced in [LICENSE.txt](LICENSE.txt). See the top of `src/driver/ArduinoSpiIO.h` and `src/driver/Esp32SdmmcIO.h` for the (compatible, permissive) terms that apply to those two files specifically.

