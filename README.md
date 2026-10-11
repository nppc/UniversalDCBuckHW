# UniversalDCBuckHW

A universal, digitally controlled DC-DC Buck converter with hardware design (KiCad) and high-performance AVR assembly firmware (ATtiny85).

Designed for remote digital control and telemetry via I²C from host controllers like ESP8266, ESP32, Arduino, or Raspberry Pi.

---

## Features

- **Microcontroller:** Microchip/Atmel ATtiny85 running at 8 MHz.
- **High-Frequency PWM:** 250 kHz Fast PWM using Timer1 driven by the internal 64 MHz high-speed PLL (`PLLCSR`).
- **Closed-Loop Regulation:** 1 ms loop updating PWM duty cycle to match target output voltage.
- **Configurable Slew Rate:** Smooth voltage ramping configured in (V/s) to protect connected loads against voltage spikes.
- **Dual Telemetry:** Real-time voltage ($V \times 10$) and current ($A \times 10$) sensing, with optional multi-sample moving average filtering.
- **EEPROM Storage:** Non-volatile parameter storage (voltage limits, slew rate) protected by a CRC-8 checksum.
- **I²C Slave Interface:** 7-bit addressing (`0x5E`), supporting telemetry readout and parameter configuration over standard 100 kHz bus.

---

## Repository Structure

```
UniversalDCBuckHW/
├── Firmware/
│   └── Assembler/
│       └── UDCBuckHW/          # ATtiny85 assembly source code
│           ├── main.asm        # Entry point, vector table, main loop
│           ├── I2C.inc         # USI-based I2C slave state machine
│           ├── PWM.inc         # Timer1 64 MHz PLL & 250 kHz PWM setup
│           ├── ADC.inc         # ADC channel sequencing & conversion
│           ├── scheduler.inc   # Timer0 1 ms rate scheduler & voltage ramps
│           ├── EEPROM.inc      # EEPROM read/write with CRC-8 validation
│           ├── MovAverage.inc  # Running moving-average filter
│           ├── math.inc        # 16-bit unsigned division routine
│           └── main.inc        # Buffer sync, PWM regulation, unit conversions
├── KiCad/                      # KiCad schematics and PCB layout
│   ├── UniDCBuckHW.sch         # Main converter schematic (IR2101S, MOSFETs, etc.)
│   ├── UniDCBuckHW.kicad_pcb   # PCB board layout
│   └── Adapter1-25to1/         # Adapter board files
├── SCAD/                       # OpenSCAD 3D models for power components
├── Refs/                       # Component datasheets & reference materials
└── bin/                        # Pre-built binaries (.hex) and flashing scripts
```

---

## Hardware Architecture

### Pinout (ATtiny85)

| Pin | Name | Direction | Description |
| :---: | :--- | :---: | :--- |
| **PB0** | `PIN_SDA` | I/O | I²C Serial Data (USI Two-Wire) |
| **PB1** | `PIN_PWM` | Output | 250 kHz PWM signal to MOSFET driver (`OC1A`) |
| **PB2** | `PIN_SCL` | Input | I²C Serial Clock (USI Two-Wire) |
| **PB3** | `PIN_Isense` | Input | ADC3 current measurement (e.g. 185 mV/A sensor) |
| **PB4** | `PIN_Vsense` | Input | ADC2 voltage divider feedback |
| **PB5** | `RESET` | Input | Active-low reset (ISP programming) |

### Power Stage
- **Gate Driver:** IR2101S high-and-low side MOSFET driver.
- **Power Switches:** IRLR8726PbF N-channel power MOSFETs (DPAK).
- **Freewheeling Diode:** PPS1045 Schottky barrier rectifier.
- **Inductor:** PCV-2-473-10L high-current power inductor.

---

## I²C Protocol & Memory Map

- **Slave Address:** `0x5E` (7-bit), `0xBC` (8-bit write), `0xBD` (8-bit read)
- **Bus Speed:** 100 kHz Standard Mode (recommended)

### Buffer Layout (11 Bytes)

| Offset | Field | Access | Format / Units | Description |
| :---: | :--- | :---: | :---: | :--- |
| `0` | `Voltage_Set` | R/W | $V \times 10$ (e.g., $120 = 12.0\text{ V}$) | Target output voltage. Set to `0` to turn off output. |
| `1` | `Voltage_Change` | R/W | $(V \times 10) / \text{s}$ (e.g., $10 = 1.0\text{ V/s}$) | Slew rate limit ($\ge 4$). |
| `2` | `Voltage_Min` | R/W | $V \times 10$ (e.g., $10 = 1.0\text{ V}$) | Minimum allowed output voltage. |
| `3` | `Voltage_Max` | R/W | $V \times 10$ (e.g., $120 = 12.0\text{ V}$) | Maximum allowed output voltage. |
| `4` | `PWM_Value` | R | $0 - 255$ | Current PWM duty cycle (`OCR1A`). |
| `5` | `Voltage_Measured` | R | $V \times 10$ | Measured output voltage. |
| `6` | `Current_Measured` | R | $A \times 10$ (e.g., $25 = 2.5\text{ A}$) | Measured output current. |
| `7` | `Voltage_RAW_L` | R | Low byte | 10-bit raw ADC reading for voltage. |
| `8` | `Voltage_RAW_H` | R | High byte | 10-bit raw ADC reading for voltage. |
| `9` | `Current_RAW_L` | R | Low byte | 10-bit raw ADC reading for current. |
| `10` | `Current_RAW_H` | R | High byte | 10-bit raw ADC reading for current. |

---

## Communication Examples

### Arduino / ESP8266 / ESP32 (C++ `Wire.h`)

#### Setting Output Voltage and Limits (Write)
```cpp
#include <Wire.h>

#define BUCK_I2C_ADDR 0x5E

void setBuckVoltage(float targetVolts, float slewRateVPerSec, float minVolts, float maxVolts) {
    uint8_t vSet   = (uint8_t)(targetVolts * 10.0f);     // e.g. 12.0V -> 120
    uint8_t vChg   = (uint8_t)(slewRateVPerSec * 10.0f); // e.g. 1.0V/s -> 10 (min 4)
    uint8_t vMin   = (uint8_t)(minVolts * 10.0f);        // e.g. 3.3V -> 33
    uint8_t vMax   = (uint8_t)(maxVolts * 10.0f);        // e.g. 15.0V -> 150

    Wire.beginTransmission(BUCK_I2C_ADDR);
    Wire.write(vSet);
    Wire.write(vChg);
    Wire.write(vMin);
    Wire.write(vMax);
    Wire.endTransmission();
}

void turnOffBuck() {
    // Setting Voltage_Set = 0 immediately disables PWM regulation
    Wire.beginTransmission(BUCK_I2C_ADDR);
    Wire.write(0);
    Wire.endTransmission();
}
```

#### Reading Telemetry (Read)
```cpp
#include <Wire.h>

#define BUCK_I2C_ADDR 0x5E

struct BuckTelemetry {
    float voltageSet;
    float voltageChange;
    float voltageMin;
    float voltageMax;
    uint8_t pwmDuty;
    float voltageMeasured;
    float currentMeasured;
    uint16_t adcRawVoltage;
    uint16_t adcRawCurrent;
};

bool readBuckTelemetry(BuckTelemetry &data) {
    uint8_t bytesReceived = Wire.requestFrom(BUCK_I2C_ADDR, 11);
    if (bytesReceived < 11) {
        return false; // Incomplete read or slave not responding
    }

    uint8_t buf[11];
    for (int i = 0; i < 11; i++) {
        buf[i] = Wire.read();
    }

    data.voltageSet       = buf[0] / 10.0f;
    data.voltageChange    = buf[1] / 10.0f;
    data.voltageMin       = buf[2] / 10.0f;
    data.voltageMax       = buf[3] / 10.0f;
    data.pwmDuty          = buf[4];
    data.voltageMeasured  = buf[5] / 10.0f;
    data.currentMeasured  = buf[6] / 10.0f;
    data.adcRawVoltage    = buf[7] | ((uint16_t)buf[8] << 8);
    data.adcRawCurrent    = buf[9] | ((uint16_t)buf[10] << 8);

    return true;
}
```

---

### Python (Raspberry Pi / `smbus2`)

```python
from smbus2 import SMBus, i2c_msg
import time

I2C_BUS = 1
BUCK_ADDR = 0x5E

def set_voltage(bus, target_v, slew_rate=1.0, min_v=3.3, max_v=15.0):
    v_set = int(target_v * 10)
    v_chg = max(4, int(slew_rate * 10))
    v_min = int(min_v * 10)
    v_max = int(max_v * 10)
    
    msg = i2c_msg.write(BUCK_ADDR, [v_set, v_chg, v_min, v_max])
    bus.i2c_rdwr(msg)

def read_telemetry(bus):
    msg = i2c_msg.read(BUCK_ADDR, 11)
    bus.i2c_rdwr(msg)
    data = list(msg)
    
    return {
        "v_set": data[0] / 10.0,
        "v_change": data[1] / 10.0,
        "v_min": data[2] / 10.0,
        "v_max": data[3] / 10.0,
        "pwm_duty": data[4],
        "v_measured": data[5] / 10.0,
        "current_a": data[6] / 10.0,
        "raw_v_adc": data[7] | (data[8] << 8),
        "raw_i_adc": data[9] | (data[10] << 8)
    }

with SMBus(I2C_BUS) as bus:
    # Set to 12.0V output
    set_voltage(bus, target_v=12.0, slew_rate=1.0, min_v=3.3, max_v=15.0)
    time.sleep(0.5)
    
    # Read telemetry
    telemetry = read_telemetry(bus)
    print(f"Output: {telemetry['v_measured']:.1f} V, Current: {telemetry['current_a']:.1f} A, PWM: {telemetry['pwm_duty']}/255")
```

---

## Building and Flashing

### Building Firmware

#### Option 1: CMake & Ninja (MPLAB / VS Code)
```powershell
cmake --build "Firmware/Assembler/UDCBuckHW/_build/HWBuckAtTiny85/default" --clean-first
```

#### Option 2: AVRASM2 Command Line
```powershell
avrasm2.exe -o bin/UDCBuckHW.hex -fI Firmware/Assembler/UDCBuckHW/main.asm
```

### Flashing with USBasp & AVRDUDE

Run `bin/flash.cmd` or invoke `avrdude` directly:
```bash
avrdude -c usbasp -p t85 -U flash:w:bin/UDCBuckHW.hex:i
```
