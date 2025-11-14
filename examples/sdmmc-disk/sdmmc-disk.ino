/**
 * @file sdmmc-disk.ino
 * @brief Example demonstrating ESP32 SDMMC interface with Arduino SD API
 * 
 * ⚠️ EXPERIMENTAL: This driver is currently experiencing initialization issues
 * with newer ESP-IDF versions (ESP_ERR_TIMEOUT 0x107 in sdmmc_host_reset).
 * 
 * RECOMMENDED ALTERNATIVE: Use Arduino's built-in SD_MMC library instead:
 *   #include <SD_MMC.h>
 *   SD_MMC.begin();  // Works reliably with ESP-IDF 5.x
 * 
 * Or use SPI mode with this library (see sd-spi-disk example), which works well.
 * 
 * This example shows how to use the ESP32's built-in SDMMC controller
 * to access an SD card with the familiar Arduino SD library API.
 * 
 * IMPORTANT: ESP32 SDMMC uses FIXED pins that CANNOT be changed!
 * 
 * Hardware Setup (1-bit mode):
 * - CMD  -> GPIO15 (fixed, cannot change)
 * - CLK  -> GPIO14 (fixed, cannot change)
 * - D0   -> GPIO2  (fixed, cannot change)
 * 
 * Hardware Setup (4-bit mode - adds):
 * - D1   -> GPIO4  (fixed, cannot change)
 * - D2   -> GPIO12 (fixed, cannot change, may conflict with flash on some boards!)
 * - D3   -> GPIO13 (fixed, cannot change)
 * 
 * Known Issues:
 * - ESP_ERR_TIMEOUT (0x107) during sdmmc_host_init with ESP-IDF 5.x
 * - May require older ESP-IDF versions (4.x) or specific board configurations
 * - Arduino SD_MMC library uses different initialization that works reliably
 * 
 * Troubleshooting if you want to try anyway:
 * 1. Ensure GPIO2, GPIO14, GPIO15 are not used by other peripherals
 * 2. Try 1-bit mode if GPIO12 conflicts with flash
 * 3. Check SD card is properly inserted and powered  
 * 4. Consider SPI mode (sd-spi-disk example) which has better compatibility
 */

#include "fatfs.h"

// Configuration - change these as needed
#define USE_1BIT_MODE true    // Set to false for 4-bit mode (faster but uses more pins)
#define SDMMC_FREQ_KHZ 20000  // 20 MHz

// Create SDMMC driver and SD interface
Esp32SdmmcIO sdmmc_driver;
SDClass SDMMC{sdmmc_driver};

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP32 SDMMC with Arduino SD API ===\n");

  // Print configuration
  Serial.printf("Mode: %s\n", USE_1BIT_MODE ? "1-bit" : "4-bit");
  Serial.printf("Frequency: %d kHz\n", SDMMC_FREQ_KHZ);
  Serial.println("Fixed pins: CLK=14, CMD=15, D0=2" + String(USE_1BIT_MODE ? "" : ", D1=4, D2=12, D3=13"));
  Serial.println();

  // Initialize SDMMC driver
  Serial.println("Initializing SDMMC driver...");
  if (!sdmmc_driver.begin(USE_1BIT_MODE, SDMMC_FREQ_KHZ)) {
    Serial.println("ERROR: SDMMC driver initialization failed!");
    Serial.println();
    Serial.println("Common causes:");
    Serial.println("  1. SD card not inserted or bad connection");
    Serial.println("  2. GPIO pins conflict (CLK=14, CMD=15, D0=2 must be free)");
    if (!USE_1BIT_MODE) {
      Serial.println("  3. GPIO12 conflict (try 1-bit mode if your board uses GPIO12 for flash)");
    }
    Serial.println("  4. Try SPI mode instead (see sd-spi-disk example)");
    return;
  }

  // Mount filesystem
  if (!SDMMC.begin()) {
    Serial.println("ERROR: Filesystem mount failed!");
    Serial.println("Please check:");
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
  
  File file = SDMMC.open(filename, FILE_WRITE);
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
  
  file = SDMMC.open(filename, FILE_READ);
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
  File root = SDMMC.open(path);
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
