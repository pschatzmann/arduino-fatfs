/**
 * @file sdmmc-disk.ino
 * @brief Example demonstrating ESP32 SDMMC interface with Arduino SD API
 * 
 * This example shows how to use the ESP32's built-in SDMMC controller
 * to access an SD card with the familiar Arduino SD library API.
 * 
 * Hardware Setup:
 * - Connect SD card to ESP32 SDMMC pins:
 *   - CMD  -> GPIO15 (or custom)
 *   - CLK  -> GPIO14 (or custom)
 *   - D0   -> GPIO2  (or custom)
 *   - D1   -> GPIO4  (4-bit mode)
 *   - D2   -> GPIO12 (4-bit mode)
 *   - D3   -> GPIO13 (4-bit mode)
 * 
 * Note: Some ESP32 boards have SD card slots with pre-defined pins.
 */

#include <Arduino.h>
#include "fatfs.h"
#include "driver/Esp32SdmmcIO.h"
#include "driver/sdcommon.h"

using namespace fatfs;

// Default frequency for SDMMC (20 MHz)
#ifndef SDMMC_FREQ_DEFAULT
#define SDMMC_FREQ_DEFAULT 20000
#endif

// Create SDMMC driver and SD interface
Esp32SdmmcIO sdmmc_driver(false, SDMMC_FREQ_DEFAULT);  // 4-bit, 20MHz
SDClass SD{sdmmc_driver};  // SD interface using SDMMC driver

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 SDMMC with Arduino SD API ===\n");

  // Initialize SD card (driver auto-initializes)
  if (!SD.begin()) {
    Serial.println("ERROR: SD Card initialization failed!");
    Serial.println("Please check:");
    Serial.println("  - SD card is inserted");
    Serial.println("  - Wiring is correct");
    Serial.println("  - Card is formatted (FAT32 recommended)");
    return;
  }

  Serial.println("✓ SD Card initialized successfully");
  
  // Print card information
  printCardInfo();

  // Demonstrate filesystem operations
  demonstrateFileOperations();
  
  Serial.println("\n=== Example Complete ===");
}

void loop() {
  // Nothing to do here
  delay(1000);
}

void printCardInfo() {
  Serial.println("\n--- SD Card Information ---");
  
  uint8_t cardTypeFlags = sdmmc_driver.cardType();
  Serial.print("Card Type: ");
  if (sdmmc_driver.isMMC()) {
    Serial.println("MMC");
  } else if (cardTypeFlags & CT_BLOCK) {
    Serial.println("SDHC/SDXC");
  } else {
    Serial.println("SDSC");
  }

  uint64_t cardSize = sdmmc_driver.cardSize();
  Serial.print("Card Size: ");
  Serial.print(cardSize / (1024.0 * 1024.0 * 1024.0));
  Serial.println(" GB");
  
  Serial.print("Total Sectors: ");
  Serial.println((unsigned long)sdmmc_driver.totalSectors());
  
  Serial.print("Frequency: ");
  Serial.print(sdmmc_driver.getFreqKHz());
  Serial.println(" kHz");
  
  Serial.println("---------------------------\n");
}

void demonstrateFileOperations() {
  Serial.println("\n--- File Operations Demo ---");
  
  // Create and write to a file
  const char* filename = "/test.txt";
  const char* message = "Hello from ESP32 SDMMC with Arduino SD API!\n";
  
  Serial.print("Creating file: ");
  Serial.println(filename);
  
  File file = SD.open(filename, FILE_WRITE);
  if (file) {
    size_t bytesWritten = file.write((const uint8_t*)message, strlen(message));
    file.close();
    Serial.print("✓ Wrote ");
    Serial.print(bytesWritten);
    Serial.println(" bytes");
  } else {
    Serial.println("✗ Failed to create file");
    return;
  }

  // Read back the file
  Serial.print("Reading file: ");
  Serial.println(filename);
  
  file = SD.open(filename, FILE_READ);
  if (file) {
    Serial.print("✓ Read: ");
    while (file.available()) {
      Serial.write(file.read());
    }
    
    // Get file size
    file.seek(0);
    Serial.print("File size: ");
    Serial.print(file.size());
    Serial.println(" bytes");
    
    file.close();
  } else {
    Serial.println("✗ Failed to read file");
    return;
  }

  // List root directory
  Serial.println("\n--- Root Directory ---");
  listDirectory("/");
  
  Serial.println("----------------------\n");
}

void listDirectory(const char* path) {
  File root = SD.open(path);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    root.close();
    return;
  }
  
  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  [DIR]  ");
      Serial.println(file.name());
    } else {
      Serial.print("  [FILE] ");
      Serial.print(file.name());
      Serial.print("  ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    file.close();
    file = root.openNextFile();
  }
  root.close();
}
