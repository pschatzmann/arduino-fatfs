# Driver Code Examples

See [Driver Overview](driver-overview.md) for the list of available drivers.

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
// ArduinoSpiIO drv{CS, SPI, 10000000}; // same, but capped at 10MHz once the card is initialized (default: FF_SPI_SPEED_FAST, 20MHz)
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

## Network Block Device (NBD)

`NBDClientIO` uses an export from a Network Block Device server as the disk. It needs the arduino-nbd `src` folder on the include path, and any Arduino `Client` with network access (e.g. `WiFiClient`). Sectors are 512 bytes: sector N maps to byte offset N * 512 of the export.

```C++
#include <WiFi.h>
#include "fatfs.h"
#include "driver/NBDClientIO.h"

WiFiClient wifi;
nbd::NBDClient nbd_client(wifi);
// host string must stay valid while the driver is in use
NBDClientIO drv{nbd_client, "192.168.1.10", nbd::NBD_DEFAULT_PORT, "ram"};
File file;

void setup() {
    // connects to the server on first mount
    SD.begin(drv);

    file = SD.open("test");
    Serial.println(file.size());
}

void loop() {}

```

If the export is read-only, the driver reports it as write protected and file writes fail.

## Multiple Drives

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

Here is an example of setting up a multi drive scenario using the MultiIO driver:

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
