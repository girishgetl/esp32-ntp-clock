# ESP32 NTP Clock

A WiFi-synced NTP clock built around an ESP32 and a 2.3" 7-segment Common Anode LED display (SA23-11SRWA). Features a web UI for configuration, day/night brightness scheduling, smooth digit scroll animation, and OTA firmware updates.

---

## Hardware

| Component | Part |
|-----------|------|
| Microcontroller | ESP32-WROOM-32 |
| Display | SA23-11SRWA 2.3" 7-segment (4 digit, Common Anode, Vf >10V) |
| Segment drivers | AO3400A (N-channel MOSFET) |
| Digit drivers | AO3401A (P-channel MOSFET) + AO3400A level shifter |
| Power | 12V in → Mini360 buck → 3.3V for ESP32 |
| Colon | 2x LEDs in series, 470Ω, hardware PWM |
| Protection | BAT54C dual Schottky (FTDI / Mini360 power OR-ing) |

### Pin Assignment

| Signal | GPIO |
|--------|------|
| SEG_A | 23 |
| SEG_B | 22 |
| SEG_C | 21 |
| SEG_D | 5 |
| SEG_E | 4 |
| SEG_F | 27 |
| SEG_G | 26 |
| DIGIT_1 | 16 |
| DIGIT_2 | 17 |
| DIGIT_3 | 18 |
| DIGIT_4 | 19 |
| COLON | 25 |

---

## Features

- **NTP time sync** — syncs on boot, resyncs every hour
- **Web UI** at `http://largeclock.local` — timezone, 12/24h, colon mode, brightness schedule
- **Day/Night brightness scheduler** — set separate brightness % and start hour for day and night
- **Scroll-up digit animation** — smooth 7-frame roll when digits change
- **OTA firmware updates** — flash over WiFi after first USB flash
- **WiFi reset** — wipe credentials and relaunch setup portal from the web UI

---

## First-Time Flash (USB)

The PCB does not have auto-reset circuitry (no DTR/RTS transistors on v1). You need to manually trigger the ESP32 bootloader before each USB flash.

### Requirements
- FTDI adapter (3.3V)
- [PlatformIO](https://platformio.org/) installed

### Steps

1. **Enter bootloader mode:**
   - Short **IO0 to GND** (use a jumper or tweezers)
   - While holding IO0 low, pulse **EN** (momentarily connect EN to GND then release)
   - You should see the ESP32 waiting — release IO0

2. **Flash:**
   ```bash
   pio run -e esp32doit-devkit-v1 -t upload
   ```

3. After flashing, press **EN** once to reboot into the new firmware.

> **Note:** This manual bootloader step is only needed for USB flashing. Once the OTA firmware is running, all future updates can be done wirelessly.

---

## WiFi Setup

On first boot (or after a WiFi reset), the ESP32 launches a captive portal:

- Connect to WiFi network: **`ESP32-Clock`**
- Password: **`clock1234`**
- A setup page opens automatically — select your WiFi network and enter the password
- The clock reboots and connects

Access the settings page at **`http://largeclock.local`** once connected.

---

## OTA Updates (after first flash)

All subsequent firmware updates can be done over WiFi — no USB or bootloader needed.

```bash
pio run -e ota -t upload
```

- Hostname: `largeclock`
- OTA password: `clock1234`

The clock will reboot automatically when the upload completes.

---

## WiFi Reset

To wipe WiFi credentials and re-run the setup portal (e.g. before shipping or changing networks):

1. Open `http://largeclock.local`
2. Scroll to **Factory Reset**
3. Click **Reset WiFi & Reboot**
4. The clock reboots into the captive portal

---

## Building

```bash
# Clone
git clone https://github.com/girishgetl/esp32-ntp-clock.git
cd esp32-ntp-clock

# Install dependencies and build
pio run
```

Dependencies are managed automatically by PlatformIO:
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- ArduinoOTA (bundled with ESP32 Arduino core)
