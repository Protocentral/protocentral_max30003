# ProtoCentral MAX30003 — Single‑lead ECG Breakout

Compact Arduino library and examples for the MAX30003 single‑lead ECG analog front end.

Highlights
- MAX30003 AFE: single‑lead ECG, ultra‑low power operation, integrated R‑R (RTOR) detection and QRS timing.
- Board: ProtoCentral breakout with level selection for 3.3V/5V systems, SPI interface, and INT pin for event/RTOR notifications.

Quick features
- Single‑lead ECG measurement (24‑bit ADC samples)
- On‑chip R‑R (RTOR) detection and heart‑rate calculation
- Low power: suitable for battery powered wearables
- SPI interface; interrupt output for beat/flags

Wiring (Arduino defaults used in examples)

| MAX30003 signal | Arduino pin | Notes |
|-----------------|-------------|-------|
| MISO            | D12         | SPI MISO |
| MOSI            | D11         | SPI MOSI |
| SCLK            | D13         | SPI SCK  |
| CS              | D4          | Chip select (examples default) |
| INT1            | D2          | Interrupt / RTOR flag |
| VCC / 3V3       | 3.3V / 5V   | Power (match board jumper) |
| GND             | GND         | Ground |

Quick start (example)

```cpp
#include <SPI.h>
#include "protocentral_max30003.h"

// Use chip select pin D4 (examples default)
MAX30003 max30003(4);

void setup() {
  Serial.begin(115200);
  SPI.begin();

  if (!max30003.readDeviceID()) {
    while (1) { delay(1000); }
  }

  max30003.begin();         // start sensor, default sampling
  max30003.setSamplingRate(MAX30003::SRATE_1000HZ); // optional: change sample rate
}

void loop() {
  int32_t ecg = 0;
  if (max30003.readEcgSample(ecg)) {   // returns true when a new sample is available
    Serial.println(ecg);               // 24‑bit sign‑extended ADC value
  }

  // RTOR / heart rate (library keeps latest values)
  max30003.updateHeartRate();
  Serial.print("HR:"); Serial.print(max30003.heartRate());
  Serial.print(" RR(ms):"); Serial.println(max30003.rrInterval());

  delay(8);
}
```

Examples
- `examples/01-ecg-openview` — OpenView / UART packet streaming example
- `examples/02-ecg-plotter` — Arduino Serial Plotter output
- `examples/03-hr-rtor` — Heart‑rate and RTOR demonstration

Build & upload (arduino-cli)

```bash
arduino-cli compile --fqbn arduino:renesas_uno:minima --libraries /path/to/Arduino/libraries /path/to/protocentral_max30003/examples/02-ecg-plotter
arduino-cli upload -p /dev/ttyACM0 -b arduino:renesas_uno:minima -i /tmp/build/02-ecg-plotter.ino.bin
```

Documentation & support
- Board and getting started: https://docs.protocentral.com/getting-started-with-max30003/
- Product page: https://www.protocentral.com/open-medical-devices/1149-protocentral-max30003-single-lead-ecg-breakout-board.html

Licenses
- Hardware: CERN-OHL-P v2 (see `LICENSE.md`)
- Software: MIT (see `LICENSE.md`)

If you want a shorter or longer README, or to add a wiring diagram image in this file, tell me where to place it and I will update the README accordingly.
