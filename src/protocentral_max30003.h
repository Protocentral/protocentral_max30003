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

// Programmable gain amplifier (PGA) settings (maps to CNFG_ECG.GAIN codes)
enum class PgaGain : uint8_t { GAIN_20 = 0, GAIN_40 = 1, GAIN_80 = 2, GAIN_160 = 3 };

// Power modes
enum class PowerMode : uint8_t { NORMAL = 0, LOW_POWER = 1, SHUTDOWN = 2 };

// RTOR configuration preset
enum class RtorPreset : uint8_t { DEFAULT = 0, AGGRESSIVE = 1, CONSERVATIVE = 2 };

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

  // Set PGA gain
  bool setPgaGain(PgaGain gain);

  // Set power mode
  bool setPowerMode(PowerMode mode);

  // Configure RTOR detection sensitivity/preset
  bool configureRtor(RtorPreset preset);

  // Write raw config to a device register (3-byte value)
  bool writeRawRegister(uint8_t reg, uint32_t value);

  // General configuration controls (CNFG_GEN)
  bool enableUltraLowPowerLeadOn(bool enable);
  bool enableEcgChannel(bool enable);
  bool enableResistiveBias(bool enable, uint8_t rbiasv_selector, bool rbiasp, bool rbiasn);

  // Calibration configuration (CNFG_CAL)
  bool configureCalibration(bool enableVcal, uint8_t vmode, uint8_t vmag, uint8_t fcal, bool fifty, uint8_t thigh);

  // Input multiplexer (CNFG_EMUX)
  bool configureEmux(bool polarityInvert, bool openp, bool openn, uint8_t calp_sel, uint8_t caln_sel);

  // ECG channel configuration (CNFG_ECG)
  bool configureEcgChannel(uint8_t rate_code, uint8_t gain_code, bool dhpf_enable, uint8_t dlpf_code);

  // R-to-R configuration (CNFG_RTOR1/2)
  bool configureRtorFull(uint8_t wndw, uint8_t gain, bool enableRtor, uint8_t pavg, uint8_t ptsf, uint8_t hoff, uint8_t ravg, uint8_t rhsf);

  // Interrupts and dynamic manager
  bool configureInterrupts(uint8_t efit, bool clr_fast, bool clr_rrint, bool clr_samp, uint8_t samp_it);
  bool configureDynamicManager(bool fast_auto, uint8_t fast_th);

  // FIFO and commands
  bool setFilter(bool enableHighPassHalfHz, uint8_t lowPassHz);
  bool configureFifo(bool enable, uint8_t watermark);
  bool fifoReset();
  bool syncCommand();

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

  // Modify a single byte of a 3-byte register (byteIndex: 0=MSB,1=mid,2=LSB)
  bool modifyRegisterByte(uint8_t reg, uint8_t byteIndex, uint8_t mask, uint8_t value);
  // Modify a bitfield within a 3-byte register using overall bit positions (msb..lsb)
  bool modifyRegisterField(uint8_t reg, uint8_t msb, uint8_t lsb, uint32_t value);
};

#endif // PROTOCENTRAL_MAX30003_H
