# Driver Overview

The FatFs library stops at the driver interface and does not provide any implementation. Tiny FATFS adds the most important drivers. The drivers are written in a flexible way and do not use any predefined fixed pins or ports: e.g. on the SPI driver you can assign the pins as part of SPI, define the CS pin and assign your desired SPI object (e.g. SPI, SPI1, SPI2 etc).

The following driver implementations are available:

| Driver | Header | Storage | Platform | Notes |
|---|---|---|---|---|
| `RamIO` | [`driver/RamIO.h`](../src/driver/RamIO.h) | RAM / PSRAM | any | Volatile - reformatted on every mount |
| `FileIO` | [`driver/FileIO.h`](../src/driver/FileIO.h) | Host OS file (`.img`) | desktop/native builds only | Persists across process runs; auto-formats only when the image is first created |
| `ArduinoSpiIO` | [`driver/ArduinoSpiIO.h`](../src/driver/ArduinoSpiIO.h) | SD card via Arduino SPI | any Arduino board | CS pin, SPI object and post-init clock speed are freely assignable |
| `ArduinoSpiExtIO` | [`driver/ArduinoSpiIOExt.h`](../src/driver/ArduinoSpiIOExt.h) | SD card via Arduino SPI | any Arduino board | Like `ArduinoSpiIO`, but CS is driven through a user-supplied GPIO expander class instead of the core's `digitalWrite` |
| `Esp32SdmmcIO` | [`driver/Esp32SdmmcIO.h`](../src/driver/Esp32SdmmcIO.h) | SD card via native SDMMC/SDIO | ESP32 (SDMMC-capable) | Faster than SPI; uses ESP-IDF's SDMMC driver directly |
| `StreamIO` | [`driver/StreamIO.h`](../src/driver/StreamIO.h) | Any user-provided `Stream`-like class | any | Bring-your-own transport - needs `begin()`, `seek()`, `readBytes()`, `write()`, `flush()`, `sectorSize()`, `sectorCount()` and `eraseSector()` |
| `MultiIO` | [`driver/MultiIO.h`](../src/driver/MultiIO.h) | Aggregates other drivers | any | Mounts each added driver on its own logical drive number, e.g. `"0:"`, `"1:"` |
| `NBDClientIO` | [`driver/NBDClientIO.h`](../src/driver/NBDClientIO.h) | Remote Network Block Device (NBD) export (via arduino-nbd `NBDClient`) | any with a network `Client` | Uses a server's export as a 512 byte sector disk; needs the arduino-nbd `src` folder on the include path |
| `TinyUsbMscIO` | [`driver/TinyUsbMscIO.h`](../src/driver/TinyUsbMscIO.h) | Exposes another driver over USB | TinyUSB-capable boards | Not an `IO` implementation - answers USB host requests instead of FatFs |

It is very easy to add new drivers, so any contribution will be welcome...

See [Driver Code Examples](driver-examples.md) for usage samples.
