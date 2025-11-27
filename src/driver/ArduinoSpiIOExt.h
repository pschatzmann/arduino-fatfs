

#pragma once

#include "ArduinoSpiIO.h"
#include "SPI.h"
#include "sdcommon.h"

namespace fatfs {

/**
 * @brief Accessing a SD card via the Arduino SPI API: The CS pin is handled via a GPIO class
 * that provides pinMode and digitalWrite methods.
 * @ingroup io
 */

template <typename GPIOClass>
class ArduinoSpiExtIO : public ArduinoSpiIO {
 public:
  ArduinoSpiExtIO(int cs = -1, SPIClass& spi = SPI) { setSPI(cs, spi); }
  ArduinoSpiExtIO(SPIClass& spi = SPI) { setSPI(spi); }

  void setSPI(int cs = -1, SPIClass& spi = SPI) {
    this->p_spi = &spi;
    this->cs = cs;
    if (cs != -1) {
      gpio.pinMode(cs, OUTPUT);
    }
  }

 protected:
  GPIOClass gpio;

  /// update the CS pin
  void set_cs(bool high) override {
    if (cs != -1) gpio.digitalWrite(cs, high);
  }
};

}  // namespace fatfs