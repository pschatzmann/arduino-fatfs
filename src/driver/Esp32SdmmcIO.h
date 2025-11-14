/**
 ******************************************************************************
 * @file    Esp32SdmmcIO.h
 * @brief   FatFs driver for ESP32 SDMMC interface using low-level API
 ******************************************************************************
 * This software is free software and there is NO WARRANTY.
 * No restriction on use. You can use, modify and redistribute it for
 * personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
 ******************************************************************************
 */

#pragma once

#include "BaseIO.h"

#if defined(ESP32) || defined(ESP_PLATFORM)

#include <driver/sdmmc_defs.h>
#include <driver/sdmmc_host.h>
#include <sdmmc_cmd.h>

#include "sdcommon.h"

namespace fatfs {

/**
 * @brief Accessing an SD card via the ESP32 SDMMC interface using low-level API
 * @ingroup io
 *
 * This driver uses the ESP32's built-in SDMMC controller for high-speed
 * SD card access via the ESP-IDF low-level driver API. It supports both
 * 1-bit and 4-bit bus modes with custom pin configuration.
 *
 * Example usage:
 * @code
 * #include "fatfs.h"
 * using namespace fatfs;
 *
 * // Method 1: Default constructor + begin()
 * Esp32SdmmcIO driver1;
 * FatFs fs1;
 *
 * void setup() {
 *   if (!driver1.begin(false, SDMMC_FREQ_DEFAULT)) {
 *     Serial.println("Card Mount Failed");
 *     return;
 *   }
 *   driver1.mount(fs1);
 * }
 *
 * // Method 2: Constructor with config (auto-init on mount)
 * Esp32SdmmcIO driver2(false, SDMMC_FREQ_DEFAULT);  // 4-bit, 20MHz
 * FatFs fs2;
 *
 * void setup() {
 *   // Card initializes automatically when mounting
 *   driver2.mount(fs2);
 * }
 * @endcode
 */
class Esp32SdmmcIO : public BaseIO {
 public:
  /**
   * @brief Default constructor - card must be initialized with begin()
   */
  Esp32SdmmcIO() : card(nullptr), auto_init(false) {
    memset(&host_config, 0, sizeof(host_config));
    memset(&slot_config, 0, sizeof(slot_config));
  }

  /**
   * @brief Constructor with bus mode and frequency
   * @param mode1bit Use 1-bit mode (true) or 4-bit mode (false, default)
   * @param max_freq_khz Maximum frequency in kHz (default: SDMMC_FREQ_DEFAULT)
   *
   * Note: Card initialization is deferred until disk_initialize() is called,
   * or you can call begin() explicitly.
   */
  Esp32SdmmcIO(bool mode1bit, int max_freq_khz = SDMMC_FREQ_DEFAULT)
      : card(nullptr),
        auto_init(true),
        init_mode1bit(mode1bit),
        init_max_freq_khz(max_freq_khz) {
    memset(&host_config, 0, sizeof(host_config));
    memset(&slot_config, 0, sizeof(slot_config));
  }

  /**
   * @brief Constructor with custom host and slot configuration
   * @param mode1bit Use 1-bit mode (true) or 4-bit mode (false)
   * @param max_freq_khz Maximum frequency in kHz
   * @param host_cfg Host configuration (from SDMMC_HOST_DEFAULT())
   * @param slot_cfg Slot configuration (from SDMMC_SLOT_CONFIG_DEFAULT())
   *
   * Note: Card initialization is deferred until disk_initialize() is called,
   * or you can call begin() explicitly.
   */
  Esp32SdmmcIO(bool mode1bit, int max_freq_khz, const sdmmc_host_t& host_cfg,
               const sdmmc_slot_config_t& slot_cfg)
      : card(nullptr),
        auto_init(true),
        init_mode1bit(mode1bit),
        init_max_freq_khz(max_freq_khz),
        host_config(host_cfg),
        slot_config(slot_cfg) {}

  /**
   * @brief Destructor - cleanup resources
   */
  ~Esp32SdmmcIO() { end(); }

  /**
   * @brief Initialize the SDMMC interface and mount the SD card
   *
   * @param mode1bit Use 1-bit mode (true) or 4-bit mode (false, default)
   * @param max_freq_khz Maximum frequency in kHz (default: SDMMC_FREQ_DEFAULT)
   * @return true if initialization succeeded, false otherwise
   */
  bool begin(bool mode1bit = false, int max_freq_khz = SDMMC_FREQ_DEFAULT) {
    return begin(mode1bit, max_freq_khz, SDMMC_HOST_DEFAULT(),
                 SDMMC_SLOT_CONFIG_DEFAULT());
  }

  /**
   * @brief Initialize with custom host and slot configuration
   *
   * @param mode1bit Use 1-bit mode (true) or 4-bit mode (false)
   * @param max_freq_khz Maximum frequency in kHz
   * @param host_cfg Host configuration (from SDMMC_HOST_DEFAULT())
   * @param slot_cfg Slot configuration (from SDMMC_SLOT_CONFIG_DEFAULT())
   * @return true if initialization succeeded, false otherwise
   */
  bool begin(bool mode1bit, int max_freq_khz, sdmmc_host_t host_cfg,
             sdmmc_slot_config_t slot_cfg) {
    // Store configurations
    host_config = host_cfg;
    slot_config = slot_cfg;

    // Set bus width
    if (mode1bit) {
      slot_config.width = 1;
    } else {
      slot_config.width = 4;
    }

    // Set maximum frequency
    host_config.max_freq_khz = max_freq_khz;

    // host_config.flags &= ~SDMMC_HOST_FLAG_DDR;

    // Clean up any previous initialization state
    // (in case SDMMC was used before and not properly cleaned up)
    sdmmc_host_deinit();

    // Initialize host peripheral
    esp_err_t err = sdmmc_host_init();
    if (err != ESP_OK) {
      ESP_LOGE("Esp32SdmmcIO", "sdmmc_host_init failed: 0x%x", err);
      stat = STA_NOINIT;
      return false;
    }

    // Initialize slot (ESP32 only has SDMMC_HOST_SLOT_1)
    err = sdmmc_host_init_slot(SDMMC_HOST_SLOT_1, &slot_config);
    if (err != ESP_OK) {
      ESP_LOGE("Esp32SdmmcIO", "sdmmc_host_init_slot failed: 0x%x", err);
      sdmmc_host_deinit();
      stat = STA_NOINIT;
      return false;
    }

    // Allocate card structure
    if (card == nullptr) {
      card = (sdmmc_card_t*)malloc(sizeof(sdmmc_card_t));
      if (card == nullptr) {
        sdmmc_host_deinit();
        stat = STA_NOINIT;
        return false;
      }
    }
    memset(card, 0, sizeof(sdmmc_card_t));

    // Probe and initialize card
    card->host = host_config;
    err = sdmmc_card_init(&host_config, card);
    if (err != ESP_OK) {
      free(card);
      card = nullptr;
      sdmmc_host_deinit();
      stat = STA_NODISK;
      return false;
    }

    // Determine card type flags
    CardType = 0;
    if (card->is_mmc) {
      CardType = CT_MMC;
    } else {
      // SD card
      if (card->ocr & SD_OCR_SDHC_CAP) {
        CardType = CT_SD2 | CT_BLOCK;  // SDHC/SDXC
      } else {
        CardType = CT_SD2;  // SDSC
      }
    }

    stat = STA_CLEAR;
    return true;
  }

  /**
   * @brief End SDMMC interface and unmount SD card
   */
  void end() {
    if (card != nullptr) {
      free(card);
      card = nullptr;
    }

  #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 4, 0)
    // Deinitialize slot and host
    sdmmc_host_deinit_slot(SDMMC_HOST_SLOT_1);
  #endif
    sdmmc_host_deinit();
    stat = STA_NOINIT;
  }

  /**
   * @brief Initialize disk drive
   * @param drv Physical drive number (0)
   * @return Disk status
   */
  DSTATUS disk_initialize(BYTE drv) override {
    if (drv != 0) return STA_NOINIT;

    // If already initialized, return current status
    if (stat == STA_CLEAR) return stat;

    // Try to initialize with stored settings or defaults
    if (auto_init && host_config.flags == 0) {
      // Constructor with mode1bit and freq was used
      if (begin(init_mode1bit, init_max_freq_khz)) {
        return STA_CLEAR;
      }
    } else if (auto_init && host_config.flags != 0) {
      // Constructor with full config was used
      if (begin(init_mode1bit, init_max_freq_khz, host_config, slot_config)) {
        return STA_CLEAR;
      }
    } else {
      // Default constructor was used, try with defaults
      if (begin()) {
        return STA_CLEAR;
      }
    }

    return stat;
  }

  /**
   * @brief Get disk status
   * @param drv Physical drive number (0)
   * @return Disk status
   */
  DSTATUS disk_status(BYTE drv) override {
    if (drv != 0) return STA_NOINIT;
    return stat;
  }

  /**
   * @brief Read sector(s)
   * @param drv Physical drive number (0)
   * @param buff Pointer to the data buffer to store read data
   * @param sector Start sector number (LBA)
   * @param count Number of sectors to read (1..128)
   * @return Result code
   */
  DRESULT disk_read(BYTE drv, BYTE* buff, LBA_t sector, UINT count) override {
    if (drv != 0 || !count) return RES_PARERR;
    if (stat & STA_NOINIT) return RES_NOTRDY;
    if (card == nullptr) return RES_NOTRDY;

    // Use low-level API to read sectors
    esp_err_t err = sdmmc_read_sectors(card, buff, sector, count);

    return (err == ESP_OK) ? RES_OK : RES_ERROR;
  }

  /**
   * @brief Write sector(s)
   * @param drv Physical drive number (0)
   * @param buff Pointer to the data to write
   * @param sector Start sector number (LBA)
   * @param count Number of sectors to write (1..128)
   * @return Result code
   */
#if FF_IO_USE_WRITE
  DRESULT disk_write(BYTE drv, const BYTE* buff, LBA_t sector,
                     UINT count) override {
    if (drv != 0 || !count) return RES_PARERR;
    if (stat & STA_NOINIT) return RES_NOTRDY;
    if (stat & STA_PROTECT) return RES_WRPRT;
    if (card == nullptr) return RES_NOTRDY;

    // Use low-level API to write sectors
    esp_err_t err = sdmmc_write_sectors(card, buff, sector, count);

    return (err == ESP_OK) ? RES_OK : RES_ERROR;
  }
#endif

  /**
   * @brief Miscellaneous drive controls
   * @param drv Physical drive number (0)
   * @param cmd Control command code
   * @param buff Pointer to the control data
   * @return Result code
   */
#if FF_IO_USE_IOCTL
  DRESULT disk_ioctl(BYTE drv, ioctl_cmd_t cmd, void* buff) override {
    if (drv != 0) return RES_PARERR;
    if (stat & STA_NOINIT) return RES_NOTRDY;

    DRESULT res = RES_ERROR;

    switch (cmd) {
      case CTRL_SYNC:
        // SD_MMC handles synchronization internally
        res = RES_OK;
        break;

      case GET_SECTOR_COUNT:
        if (buff && card) {
          // Get total sectors from card capacity
          uint64_t capacity =
              ((uint64_t)card->csd.capacity) * card->csd.sector_size;
          *(DWORD*)buff = (DWORD)(capacity / 512);
          res = RES_OK;
        }
        break;

      case GET_SECTOR_SIZE:
        if (buff) {
          // SD cards use 512-byte sectors for compatibility
          *(WORD*)buff = 512;
          res = RES_OK;
        }
        break;

      case GET_BLOCK_SIZE:
        if (buff && card) {
          // Return erase block size in sectors
          // Use the card's erase sector size if available
          uint32_t erase_size = card->csd.sector_size;
          if (erase_size < 512) erase_size = 512;
          *(DWORD*)buff = erase_size / 512;
          if (*(DWORD*)buff == 0) *(DWORD*)buff = 1;
          res = RES_OK;
        }
        break;

      case CTRL_TRIM:
        // TRIM/ERASE not implemented via SD_MMC library
        res = RES_OK;  // Return OK but don't actually trim
        break;

      case MMC_GET_TYPE:
        if (buff) {
          *(BYTE*)buff = CardType;
          res = RES_OK;
        }
        break;

      case MMC_GET_CSD:
        if (buff && card) {
          // Copy CSD register data
          memcpy(buff, &card->csd, sizeof(card->csd));
          res = RES_OK;
        }
        break;

      case MMC_GET_CID:
        if (buff && card) {
          // Copy CID register data
          memcpy(buff, &card->cid, sizeof(card->cid));
          res = RES_OK;
        }
        break;

      case MMC_GET_OCR:
        if (buff && card) {
          // Copy OCR register
          *(DWORD*)buff = card->ocr;
          res = RES_OK;
        }
        break;

      case MMC_GET_SDSTAT:
        if (buff && card) {
          // Get SD status - this requires reading from the card
          // For simplicity, return not implemented
          res = RES_PARERR;
        }
        break;

      default:
        res = RES_PARERR;
        break;
    }

    return res;
  }
#endif

  /**
   * @brief Get the card size in bytes
   * @return Card size in bytes
   */
  uint64_t cardSize() const {
    if (card == nullptr) return 0;
    return ((uint64_t)card->csd.capacity) * card->csd.sector_size;
  }

  /**
   * @brief Get the card type flags
   * @return Card type flags (CT_MMC, CT_SD1, CT_SD2, CT_BLOCK)
   */
  uint8_t cardType() const { return CardType; }

  /**
   * @brief Get the total number of sectors
   * @return Total sectors (0 if not initialized)
   */
  uint64_t totalSectors() const {
    if (stat & STA_NOINIT || card == nullptr) return 0;
    return cardSize() / 512;
  }

  /**
   * @brief Get pointer to the card structure (for advanced use)
   * @return Pointer to sdmmc_card_t or nullptr if not initialized
   */
  sdmmc_card_t* getCard() { return card; }

  /**
   * @brief Check if card is MMC
   * @return true if MMC card, false if SD card
   */
  bool isMMC() const { return card ? card->is_mmc : false; }

  /**
   * @brief Get card frequency in kHz
   * @return Card operating frequency in kHz
   */
  uint32_t getFreqKHz() const { return card ? card->max_freq_khz : 0; }

 protected:
  volatile DSTATUS stat = STA_NOINIT;  ///< Physical drive status
  BYTE CardType = 0;                   ///< Card type flags
  sdmmc_card_t* card;                  ///< Pointer to card structure
  sdmmc_host_t host_config;            ///< Host configuration
  sdmmc_slot_config_t slot_config;     ///< Slot configuration

  // Auto-initialization parameters (for constructor-based config)
  bool auto_init;         ///< Whether to auto-init on first use
  bool init_mode1bit;     ///< Stored 1-bit mode setting
  int init_max_freq_khz;  ///< Stored max frequency setting
};

}  // namespace fatfs

#endif  // ESP32 || ESP_PLATFORM
