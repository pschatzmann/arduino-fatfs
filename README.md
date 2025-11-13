# Arduino-FatFs

There are quite a few SD Arduino libraries out there: the most important is the [SD.h provided by Arduino](https://github.com/arduino-libraries/SD) which is a wrapper for [SdFat from Bill Greiman](https://github.com/greiman/SdFat) which itself is quite friendly, powerfull and fast.

Bill's library provides some alternative SPI drivers to access the SD functionality, but it does not provide the functionality to store the data anywhere else and you can't use multiple SD drives that are attached to different SPI ports.

I am providing the [FatFs library](http://elm-chan.org/fsw/ff/00index_e.html) developed by ChaN that I have converted to C++ header only so that we can flexibly support multiple data access drivers and scenarios at the same time.

The advantage of this library is, that it provides quite a few __[configuration options](http://elm-chan.org/fsw/ff/doc/config.html#use_mkfs)__ and has a flexible __driver concept__, so that we can store the data potentially on the SD, in SPI RAM, RAM, PSRAM etc. The FatFs library stops at the driver interface and does not provide any implementation. 

I have added the most important drivers to this project:  The drivers are written in a flexible way and do not use any predefed fixed pins or ports: e.g. on the SPI driver you can assign the pins as part of SPI, define the CS pin and assign your desired SPI object (e.g. SPI, SPI1, SPI2 etc). We currently provide the following __driver implementations__:

- The data is stored in __RAM (or PSRAM)__ (RamIO)
- Support for __multiple drives__ with different drivers (MultiIO)
- SD via Arduino __SPI__ (SDArduinoSPIIO)

It is very easy to add new drivers, so any contribution will be welcome...

## SPI SD

Here is an example of setting up a SD drive using the Arduino ESP32 SPI API:

```C++
#include "SPI.h"
#include "fatfs.h"

#define MISO 12
#define MOSI 13
#define SCLK 14
#define CS   15

SDArduinoSPIIO drv{CS, SPI}; // SD driver managing CS and assign SPI
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


## Mutliple Drives


You can also use your own separate SDClass instances:

```C++
#include "SPI.h"
#include "fatfs.h"

#define MISO 12
#define MOSI 13
#define SCLK 14
#define CS   15

SDArduinoSPIIO sd{CS, SPI}; // driver managing CS and assign SPI
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
#include "driver/SDArduinoSPIIO.h"
#include "driver/RamIO.h"
#include "driver/MultiIO.h"

#define MISO 12
#define MOSI 13
#define SCLK 14
#define CS   15

SDArduinoSPIIO sd{CS, SPI}; // driver managing CS and assign SPI
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

# Documentaion

- [Arduino SD API](https://www.arduino.cc/reference/en/libraries/sd/)
- [FatFS Documentation](http://elm-chan.org/fsw/ff/00index_e.html)
- Class Documentation
    - [Arduino API](https://pschatzmann.github.io/arduino-fatfs/html/group__sd.html)
    - [FatFs API](https://pschatzmann.github.io/arduino-fatfs/html/classfatfs_1_1FatFs.html)
    - [Media Access](https://pschatzmann.github.io/arduino-fatfs/html/group__io.html)

