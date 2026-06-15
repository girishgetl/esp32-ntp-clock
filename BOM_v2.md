# ESP32 NTP Clock v2 - BOM
# Single unified PCB — display + controller combined

## ACTIVE COMPONENTS

| Qty | Reference | Value | Package | Description |
|-----|-----------|-------|---------|-------------|
| 1 | U1 | ESP32-WROOM-32 | Module | WiFi MCU |
| 1 | U2 | Mini360 | Module | DC-DC Buck 12V→3.3V |
| 9 | Q1,Q2,Q5,Q6,Q9,Q10,Q13,Q14,Q19 | AO3400A | SOT-23 | N-ch MOSFET — segment drivers (7) + colon driver (1) + spare (1) |
| 4 | Q3,Q7,Q11,Q15 | AO3400A | SOT-23 | N-ch MOSFET — digit level shifters |
| 4 | Q4,Q8,Q12,Q16 | AO3401A | SOT-23 | P-ch MOSFET — high-side digit switches |
| 2 | Q17,Q18 | SS8050 | SOT-23 | NPN BJT — auto-reset DTR/RTS circuit |
| 1 | D4 | BAT54C | SOT-23 | Dual Schottky — FTDI/Mini360 power OR-ing |
| 2 | D2,D3 | 5mm LED (Red/any) | THT | Colon LEDs (Vf ~3.2V) |
| 4 | DSP5-DSP8 | SA23-11SRWA | THT | 2.3" 7-Segment Common Anode display |

## PASSIVE COMPONENTS

| Qty | Reference | Value | Package | Description |
|-----|-----------|-------|---------|-------------|
| 4 | R3,R4,R5,R6 | 10K | 1206 | P-ch gate pull-up to 12V |
| 2 | R1,R2 | 10K | 1206 | ESP32 EN and IO0 pull-up to 3.3V |
| 2 | R7,R8 | 10K | 1206 | Auto-reset BJT base resistors |
| 1 | R_colon | 470R | 1206 | Colon LED current limit |
| 1 | C1 | 100µF/16V | Electrolytic SMD | 3.3V bulk decoupling (Mini360 output) |
| 1 | C2 | 100nF | 1206 | 3.3V HF decoupling |
| 1 | C3 | 100nF | 1206 | EN pin noise filter |

## CONNECTORS

| Qty | Reference | Value | Package | Description |
|-----|-----------|-------|---------|-------------|
| 1 | J1 | DC Barrel Jack | PCB mount | 12V power input (5.5mm/2.1mm) |
| 1 | J2 | Barrel_Jack_Switch | PCB mount | Switch contact (if using switched jack) |
| 1 | J3 | 1x6 Pin Header 2.54mm | THT | FTDI programming header |

## FTDI HEADER PINOUT (J3)
```
Pin 1: GND
Pin 2: CTS  (no connect on ESP32 side)
Pin 3: IN_3V3 (via BAT54C)
Pin 4: TXD → ESP32 RXD0 (GPIO3)
Pin 5: RXD → ESP32 TXD0 (GPIO1)
Pin 6: DTR → auto-reset Q17
       RTS → auto-reset Q18
```

## ORDER SUMMARY

| Part | Qty | Notes |
|------|-----|-------|
| ESP32-WROOM-32 | 1 | |
| Mini360 buck module | 1 | Pre-adjust to 3.3V before soldering |
| AO3400A SOT-23 | 15 | 13 needed + 2 spare |
| AO3401A SOT-23 | 6 | 4 needed + 2 spare |
| SS8050 SOT-23 | 3 | 2 needed + 1 spare |
| BAT54C SOT-23 | 2 | 1 needed + 1 spare |
| SA23-11SRWA | 4 | 2.3" 7-seg display |
| 5mm LED | 4 | 2 needed + 2 spare, colon |
| 10K 1206 resistor | 20 | 8 needed + extras |
| 470R 1206 resistor | 3 | 1 needed + 2 spare |
| 100nF 1206 capacitor | 5 | 2 needed + extras |
| 100µF SMD electrolytic | 2 | 1 needed + 1 spare |
| DC barrel jack PCB mount 5.5/2.1mm | 1 | |
| 1x6 pin header 2.54mm | 2 | 1 needed + 1 spare |

## NOTES

1. **AO3400A total = 13**: 7 segment + 4 level shifters + 1 colon + 1 spare GPIO32 (SEG_DP future use)
2. **SS8050**: NPN BJT for auto-reset — matches DevKitC V4 reference design
3. **P-ch gate pull-ups are 10K to 12V** — confirmed on v1 for clean 50µs blanking
4. **Mini360**: Adjust trimmer to exactly 3.3V output BEFORE soldering onto PCB
5. **Colon LED current**: 12V - 3.2V - 3.2V = 5.6V / 470R ≈ 12mA per LED — good brightness
6. **Order extras on all SOT-23 parts** — small and easy to lose during hand assembly
