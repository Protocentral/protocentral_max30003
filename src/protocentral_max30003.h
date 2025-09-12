// SPDX-License-Identifier: MIT
/**
 * @file protocentral_max30003.h
 * @brief Modernized header for the MAX30003 Arduino library.
 *
 * @author Ashwin Whitchurch <support@protocentral.com>
 * @copyright Copyright (c) 2025 Protocentral Electronics
 */

#ifndef PROTOCENTRAL_MAX30003_H
#define PROTOCENTRAL_MAX30003_H

#include <Arduino.h>
#include <SPI.h>

// Register access flags
static constexpr uint8_t WREG = 0x00;
static constexpr uint8_t RREG = 0x01;

// Register addresses (subset used by this library)
enum : uint8_t {
  REG_NO_OP = 0x00,
  REG_STATUS = 0x01,
  REG_EN_INT = 0x02,
  REG_EN_INT2 = 0x03,
  REG_MNGR_INT = 0x04,
  REG_MNGR_DYN = 0x05,
  REG_SW_RST = 0x08,
  REG_SYNCH = 0x09,
  REG_FIFO_RST = 0x0A,
  REG_INFO = 0x0F,
  REG_CNFG_GEN = 0x10,
  REG_CNFG_CAL = 0x12,
  REG_CNFG_EMUX = 0x14,
  REG_CNFG_ECG = 0x15,
  REG_CNFG_RTOR1 = 0x1D,
  REG_CNFG_RTOR2 = 0x1E,
  REG_ECG_FIFO_BURST = 0x20,
  REG_ECG_FIFO = 0x21,
  REG_RTOR = 0x25,
  REG_NO_OP_ALT = 0x7F
};

// Sampling rates supported
enum SamplingRate : uint16_t { SR_128 = 128, SR_256 = 256, SR_512 = 512 };

class MAX30003 {
public:
  // Construct with CS pin and optional SPI instance (defaults to SPI)
  explicit MAX30003(int cs_pin, SPIClass &spi = SPI) noexcept;

  // Initialize device (power-on config). Returns true on success.
  bool begin();

  // Software reset
  void reset();

  // Read device ID/info register
  bool readDeviceID();

  // Set sampling rate
  bool setSamplingRate(SamplingRate rate);

  // Read single ECG 24-bit signed sample. Returns true on success.
  bool readEcgSample(int32_t &sample);

  // Read a burst of `count` ECG samples into buffer (3 bytes per sample).
  // `out` must have room for count*3 bytes.
  bool readEcgBurst(uint16_t count, uint8_t *out);

  // Update and read Heart Rate and RR interval registers
  bool updateHeartRate();

  // Getters
  uint16_t heartRate() const noexcept { return _heartRate; }
  uint16_t rrInterval() const noexcept { return _rrInterval; }

  // Low-level register access
  bool writeRegister(uint8_t reg, uint32_t value);
  bool readRegister(uint8_t reg, uint8_t *buf, size_t len = 3);

private:
  SPIClass &_spi;
  SPISettings _settings;
  int _cs_pin;

  uint16_t _heartRate{0};
  uint16_t _rrInterval{0};

  void chipSelect(bool active) const noexcept;
  bool sync();
};

#endif // PROTOCENTRAL_MAX30003_H
