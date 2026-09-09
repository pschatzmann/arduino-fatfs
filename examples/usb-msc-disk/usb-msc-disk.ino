// Exposes a RAM disk directly to a connected host PC over USB as a mass
// storage device, using TinyUsbMscIO. Requires a TinyUSB-capable board/core
// (e.g. RP2040, SAMD21/51, nRF52, ESP32-S2/S3) with the Adafruit TinyUSB
// library installed and USE_TINYUSB defined.
//
// Note: this sketch only exports the RAM disk over USB - it does not also
// mount it locally via SD.begin(). If you need both, avoid writing from the
// device and the host at the same time.

#include "fatfs.h"
#include "driver/RamIO.h"
#include "driver/TinyUsbMscIO.h"

RamIO ram{2048, 512};  // 1MB RAM disk
TinyUsbMscIO usbMsc;

void setup() {
  Serial.begin(115200);
  while (!Serial);

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_SAMD) || defined(NRF52840_XXAA)
  // some cores need the USB device stack brought up explicitly before
  // configuring the MSC interface; on others (e.g. arduino-pico) this
  // already happened before setup() runs
  TinyUSBDevice.begin(0);
#endif

  usbMsc.setID("Arduino", "RAM Disk", "1.0");
  if (!usbMsc.begin(ram)) {
    Serial.println("usbMsc.begin() failed!");
    while (true);
  }

  Serial.println("RAM disk exported over USB - check your PC for a new drive");
}

void loop() {}
