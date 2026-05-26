# ESP32 NTP Clock - Combined BOM
# Both Display PCB + Controller PCB

## ACTIVE COMPONENTS

| Qty | Reference | Value | Package | Description |
|-----|-----------|-------|---------|-------------|
| 1 | U1 | ESP32-WROOM-32 | Module | WiFi+BT MCU Module |
| 1 | U2 | Mini360 | Module | DC-DC Buck 12V→3.3V |
| 12 | Q1-Q7, Q8-Q11, Q16 | AO3400A | SOT-23 | N-Channel MOSFET 30V 5.7A |
| 4 | Q12-Q15 | AO3401A | SOT-23 | P-Channel MOSFET -30V -4A |
| 1 | D3 | BAT54C | SOT-23 | Dual Schottky (common cathode) - Power OR-ing |

## PASSIVE COMPONENTS (all 1206 package)

| Qty | Reference | Value | Package | Description |
|-----|-----------|-------|---------|-------------|
| 4 | R1-R4 | 100K | 1206 | P-ch gate pull-up to 12V |
| 2 | R5, R6 | 10K | 1206 | EN & IO0 pull-up to 3.3V |
| 1 | R7 | 270R | 1206 | Colon LED current limiting |
| 1 | C1 | 100uF/16V | 1206 | 3.3V bulk decoupling |
| 5 | C2-C6 | 100nF | 1206 | C2: 3.3V decoupling, C3: EN-GND filter, C4: DTR-IO0, C5: RTS-EN, C6: IO0-GND filter |

## DISPLAY COMPONENTS

| Qty | Reference | Value | Package | Description |
|-----|-----------|-------|---------|-------------|
| 4 | DSP1-DSP4 | SA23-11SRWA | THT | 2.3" 7-Segment Common Anode |
| 2 | D1, D2 | Blue/White 5mm | THT | Colon LEDs (Vf=3.2V) |

## CONNECTORS

| Qty | Reference | Value | Package | Description |
|-----|-----------|-------|---------|-------------|
| 2 | J1 (x2) | 1x14 Pin Header | 2.54mm THT | Inter-board connector (1 male + 1 female) |
| 1 | J2 | Screw Terminal 2P | 5.08mm THT | 12V DC Power Input |
| 1 | J3 | 1x6 Pin Header | 2.54mm THT | FTDI Programming Header (GND, RTS, VCC, TXD, RXD, DTR) |

## ORDER SUMMARY (by part number)

| Part | Qty | Typical Part Number / Search Term |
|------|-----|-----------------------------------|
| BAT54C (Dual Schottky SOT-23) | 1 | BAT54C / LCSC: C37704 |
| AO3400A (N-ch SOT-23) | 12 | AO3400A / LCSC: C20917 |
| AO3401A (P-ch SOT-23) | 4 | AO3401A / LCSC: C15127 |
| ESP32-WROOM-32 | 1 | ESP32-WROOM-32 |
| Mini360 Buck Module | 1 | Mini360 / MP2307 module |
| 100K 1206 | 4 | - |
| 10K 1206 | 2 | - |
| 270R 1206 | 1 | - |
| 100uF 1206 16V | 1 | Ceramic or use electrolytic if unavailable in 1206 |
| 100nF 1206 | 5 | - |
| SA23-11SRWA | 4 | Kingbright SA23-11SRWA |
| 5mm Blue/White LED | 2 | - |
| 1x14 Male Pin Header 2.54mm | 1 | - |
| 1x14 Female Pin Header 2.54mm | 1 | - |
| 2P Screw Terminal 5.08mm | 1 | KF301-2P or similar |
| 1x6 Male Pin Header 2.54mm | 1 | FTDI programming header |

## NOTES

1. **100uF 1206**: Standard ceramic 100uF in 1206 exists but may be expensive/hard to find.
   Alternative: Use a 6.3x5.4mm SMD electrolytic, or 47uF ceramic 1206 + 10uF ceramic 1206 in parallel.

2. **AO3400A qty=12**: 7 segment drivers + 4 level shifters + 1 colon driver

3. **AO3401A qty=4**: High-side P-channel digit switches

4. **Order extras**: Get 5-10 extra SOT-23 MOSFETs (cheap insurance for soldering mistakes)

5. **Mini360**: Pre-adjust to 3.3V output BEFORE soldering to PCB

6. **FTDI Auto-Program Circuit**:
   ```
   J3 Pinout (standard FTDI 6-pin):
   Pin 1: GND
   Pin 2: RTS → C5 (100nF) → ESP32 EN
   Pin 3: VCC → +3.3V
   Pin 4: TXD → ESP32 RXD0 (GPIO3)
   Pin 5: RXD → ESP32 TXD0 (GPIO1)
   Pin 6: DTR → C4 (100nF) → ESP32 IO0 (GPIO0)

   Auto-reset circuit:
   DTR ── C4 (100nF) ── IO0 ──┬── R6 (10K) ── 3.3V
                               │
   RTS ── C5 (100nF) ── EN  ──┬── R5 (10K) ── 3.3V
   ```
   This allows esptool to auto-reset and enter bootloader.
   Same circuit used on ESP32-DevKitC boards.
