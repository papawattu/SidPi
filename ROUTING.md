# SidPi Rev d - Routing Strategy Guide

## Overview
Two-layer PCB (65×56mm) with HAT-compliant dimensions. Route in this order for best results.

---

## Routing Priority Order

### 1. **Power Nets First** (Critical)
**VDD (12V)** - Boost converter output path:
- MT3608 U4 pin 1 (SW) → Inductor L1 pin 2 → Diode D1 anode
- **Keep this trace SHORT and WIDE** (≥0.5mm) - high switching current
- D1 cathode → VDD net → VDD bypass caps (C5, C7) → SID pin 28
- Add 22µF bulk cap (C7) close to SID VDD pin

**VCC (5V)** - Main supply:
- GPIO pin 2/4 → VCC net → SID pin 25
- Split VCC from GPIO to boost converter input (U4 pins 4/5)
- Place 100nF (C4) and 22µF (C6) close to U4

**+3V3**:
- GPIO pins 1/17 → U2 pin 1 → U3 pin 1
- Keep separate from VCC/VDD

**GND**:
- Full pours on both layers
- Connect all ground points with vias
- Audio sleeve ground separate from digital ground until single point

---

### 2. **Boost Converter Feedback** (Sensitive)
**N$29 (FB)**: MT3608 pin 3 → R2 (150K) → R3 (7.87K) → GND
- Keep feedback trace SHORT and AWAY from inductor
- Tap feedback from VDD side of diode, not inductor side
- This sets 12.04V output

---

### 3. **CLOCK Signal** (Critical Timing)
**N$9**: GPIO4 (pin 7) → U3 → SID pin 6 (Ø2)
- **Direct, shortest path possible**
- No vias if possible
- Keep away from boost converter (L1, D1)
- Add 100Ω series resistor near GPIO if needed for impedance matching
- Ø2 acts as both clock AND chip-select enable per datasheet

---

### 4. **Control Signals** (Low Speed, but Important)
**N$1 (/CS)**: GPIO3 (pin 5) → U3 → SID pin 8
- Active-low chip select
- Keep away from noisy power traces

**N$26 (RESET)**: GPIO2 (pin 3) → U3 → SID pin 5 (/RES)
- Must be pulled low for ≥10 Ø2 cycles on power-up
- Consider adding 10K pull-up + 0.1µF cap to GND for power-on reset

---

### 5. **Data Bus** (8-bit, Parallel)
**N$5, N$7, N$10, N$11, N$12, N$13, N$14, N$19**: D0-D7
- GPIO pins 10,11,13,15,18,19,21,23 → U2 → SID pins 15-22
- **Route as matched pair or ribbon** if possible
- Keep all 8 lines equal length (±5mm tolerance)
- 74HCT245 direction pin (pin 1) must be tied to GND for RPi→SID direction
- Add 33Ω series resistors near RPi side for signal integrity

---

### 6. **Address Bus** (5-bit)
**N$2, N$4, N$20, N$24, N$25**: A0-A4
- GPIO pins 8,12,16,22,24 → U3 → SID pins 9-13
- Lower priority than data bus
- Can route after data bus

---

### 7. **Audio Output** (Analog, Sensitive)
**N$23, N$8**: SID pin 27 → R1 (1K) → C3 (1µF) → Audio jack TIP/RING
- **Keep away from digital traces** (data/address/CLOCK)
- Keep away from boost converter (L1, D1)
- Use ground shield on bottom layer if possible
- C3 should be close to SID output
- Audio sleeve connected to GND (separate analog ground)

---

### 8. **Filter Capacitors** (Analog)
**N$3, N$17**: CAP1A/CAP1B (pins 1,2) → C1 (2.2nF)
**N$6, N$15**: CAP2A/CAP2B (pins 3,4) → C2 (2.2nF)
- Place caps as close to SID pins as possible
- Keep trace lengths short
- These set the analog filter cutoff frequencies

---

## Layer Assignment

### Top Layer (1):
- All signal routing
- Power traces (VDD, VCC)
- AUDIO_OUT analog path
- CLOCK signal

### Bottom Layer (16):
- Ground plane (solid pour)
- VCC return paths
- Secondary power routing if needed

---

## Via Strategy

**Maximum vias**: 2 per signal
**Minimum vias**: 0 (prefer direct routing)
**Via size**: 0.3mm drill / 0.6mm pad (standard)
**Power vias**: Use multiple vias for VDD/VCC (≥3 per net)

**Avoid vias on**:
- CLOCK signal (Ø2)
- Audio output path
- Boost converter high-current path

---

## Clearance Rules

| Net Class | Width | Clearance | Notes |
|-----------|-------|-----------|-------|
| Default   | 0.25mm | 0.25mm   | Digital signals |
| VCC       | 0.5mm  | 0.25mm   | 5V power |
| VDD       | 0.5mm  | 0.25mm   | 12V power |
| AUDIO_OUT | 0.25mm | 0.5mm    | Analog, keep away from digital |
| VDD→GND   | -      | 0.5mm    | High voltage isolation |

---

## HAT Compliance

**GPIO Header Keepout (pins 1-10)**:
- No components within 2.54mm of header pins 1-10
- No traces crossing top layer near pins 1-10
- Reserved for overlay HAT EEPROM and ID signals

**Mounting Holes**:
- 4× M3 holes at corners (3.5mm from edge)
- Keep copper 1mm away from hole edges

---

## Recommended Eagle autorouter Settings

```
Layer Setup: Top + Bottom (1*16)
Routing Grid: 0.25mm (10mil)
Via Size: 0.3mm drill / 0.6mm pad
PrefDir.1: Horizontal (for top layer)
PrefDir.16: Vertical (for bottom layer)
```

**Autorouter Passes**:
1. Route VDD/VCC power first (use "Busses" pass)
2. Route CLOCK separately (manual preferred)
3. Run "Route" pass for remaining signals
4. Manually route AUDIO_OUT after autorouting

---

## Manual Routing Tips

### Boost Converter (U4, L1, D1):
```
Top layer only for high-current path:
U4 pin 1 → L1 pin 1 → L1 pin 2 → D1 anode
Keep this loop area minimal (EMC)
```

### SID Chip (U$1):
- Pin 1 (CAP1A) is at bottom-left (check orientation)
- Place C1/C2 immediately adjacent
- Keep analog traces short

### Level Shifters (U2, U3):
- Direction pins (pin 1) to +3V3 for enable
- Pin 2 (DIR) to GND for RPi→SID direction
- Pin 9 (OE) to +3V3 for always-enabled

---

## DRC Checks After Routing

1. **Clearance violations**: Especially VDD→GND
2. **Unrouted pins**: All 15 GPIO signals + power
3. **Short circuits**: Power nets to GND
4. **HAT keepout**: No components in GPIO 1-10 zone
5. **Silkscreen overlap**: Component labels not on pads

---

## Testing Points (Recommended)

Add test pads for:
- **VDD** (12V) - verify boost output
- **VCC** (5V) - verify RPi supply
- **CLOCK** - verify 1MHz from GPIO4
- **AUDIO_OUT** - verify before coupling cap
- **/CS** - verify active-low

---

## Component Orientation Guide

- **SID (U$1)**: Pin 1 dot/indentation facing left
- **74HCT245 (U2, U3)**: Notch facing up
- **MT3608 (U4)**: Marked side facing up
- **Diode (D1)**: Cathode stripe away from SID
- **Audio Jack (J2)**: Solder pins toward board edge
- **GPIO Header (SK1)**: Gold fingers toward bottom

---

## Post-Routing Checklist

- [ ] All nets routed
- [ ] DRC clean (zero errors)
- [ ] Ground planes connected everywhere
- [ ] VDD/VCC traces wide enough (≥0.5mm)
- [ ] CLOCK trace shortest path
- [ ] Audio path isolated from digital
- [ ] Boost converter loop minimal
- [ ] HAT keepout zone clear
- [ ] Silkscreen labels added
- [ ] Drill sizes correct (0.8mm for DIP, 1.0mm for header)

---

## Manufacturing Notes

- **Board thickness**: 1.6mm standard
- **Copper weight**: 1oz (35µm)
- **Solder mask**: Green (or preferred color)
- **Silkscreen**: White
- **Surface finish**: HASL or ENIG
- **Min trace width**: 0.2mm (use 0.25mm for safety)
- **Min via diameter**: 0.3mm
- **Min pad diameter**: 0.6mm

---

## Software Driver Notes (for reference)

When writing RPi driver:
1. Initialize GPIO4 as GPCLK (1MHz)
2. Set data pins (GPIO 10,9,10,24,22,27,17,15) as outputs
3. Set addr pins (GPIO 8,25,23,18,14) as outputs
4. Set /CS (GPIO3), RESET (GPIO2) as outputs
5. Pull RESET low for 10+ clock cycles on init
6. Assert /CS low, then write address + data
7. Deassert /CS after each transaction

Register map (SID 6581):
- $1D (29): A4 only = write to all 24 registers
- D0-D7: Data byte
- R/W tied to GND = write-only mode
