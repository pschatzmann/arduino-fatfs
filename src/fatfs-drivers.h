// SPDX-License-Identifier: MIT
#pragma once
#include "driver/RamIO.h"
#include "driver/MultiIO.h"
#ifndef ARDUINO
#include "driver/FileIO.h"
#endif
#ifdef ARDUINO
#include "driver/StreamIO.h"
#include "driver/ArduinoSpiIO.h"
#if defined(ESP32) || defined(ESP_PLATFORM)
#  include "driver/Esp32SdmmcIO.h"
#endif
#if defined(USE_TINYUSB)
#  include "driver/TinyUsbMscIO.h"
#endif
#endif