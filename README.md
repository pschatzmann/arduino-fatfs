# Tiny FATFS

[![Arduino Library](https://img.shields.io/badge/Arduino-Library-blue.svg)](https://www.arduino.cc/reference/en/libraries/)
[![Build: CMake](https://img.shields.io/badge/Build-CMake-064F8C.svg?logo=cmake)](CMakeLists.txt)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE.txt)

There are quite a few SD Arduino libraries out there: the most important is the [SD.h provided by Arduino](https://github.com/arduino-libraries/SD) which is a wrapper for [SdFat from Bill Greiman](https://github.com/greiman/SdFat) which itself is quite friendly, powerful and fast.

Bill's library provides some alternative SPI drivers to access the SD functionality, but it does not provide the functionality to store the data anywhere else and you can't use multiple SD drives that are attached to different SPI ports.

I am providing the [FatFs library](http://elm-chan.org/fsw/ff/00index_e.html) developed by ChaN that I have converted to C++ header only so that we can flexibly support multiple data access drivers and scenarios at the same time.

The advantage of this library is, that it provides quite a few __[configuration options](http://elm-chan.org/fsw/ff/doc/config.html#use_mkfs)__ and has a flexible __[driver concept](docs/driver-overview.md)__, so that we can store the data potentially on a SD disk, in RAM, PSRAM, on remote devices etc. 


# Installation

For Arduino, you can download the library as zip and call Include Library -> Add .ZIP Library. Or you can git clone this project into the Arduino libraries folder, e.g. with

```bash
cd ~/Documents/Arduino/libraries
git clone https://github.com/pschatzmann/arduino-fatfs.git
```

# Documentation

- [Supported FAT Versions](docs/supported-fat-versions.md)
- [Desktop Build and Tests](docs/desktop-build.md)
- [Driver Overview](docs/driver-overview.md)
- [Driver Code Examples](docs/driver-examples.md)
- [Arduino SD API](https://www.arduino.cc/reference/en/libraries/sd/)
- [Original FatFS Documentation](http://elm-chan.org/fsw/ff/00index_e.html)
- Class Documentation
    - [Arduino API](https://pschatzmann.github.io/arduino-fatfs/html/group__sd.html)
    - [FatFs API](https://pschatzmann.github.io/arduino-fatfs/html/classfatfs_1_1FatFs.html)
    - [Directory Iterators](https://pschatzmann.github.io/arduino-fatfs/html/group__iterator.html)
    - [Drivers](https://pschatzmann.github.io/arduino-fatfs/html/group__io.html)

# License

The wrapper code in this repository (everything outside `src/ff/`) is licensed under the [MIT License](LICENSE.txt). It bundles [ChaN's FatFs](http://elm-chan.org/fsw/ff/00index_e.html) core (`src/ff/`) under FatFs's own permissive terms, reproduced in [LICENSE.txt](LICENSE.txt). See the top of `src/driver/ArduinoSpiIO.h` and `src/driver/Esp32SdmmcIO.h` for the (compatible, permissive) terms that apply to those two files specifically.

