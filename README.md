# NERD-O-METER

> A tiny, deliberately over-engineered conference toy for the **Heltec WiFi LoRa 32 V3**.

It watches the RF noise around you, listens for **Meshtastic**, rotates Nerdearla-themed screens, knows today's talks, exposes its own source through a QR, and computes a scientifically meaningless **NERD LEVEL™**.

Built for **Nerdearla Argentina 2026 · Friday, September 25**.

## 30-minute path — Apple M1 Max + Heltec V3

The repository is designed so the boring parts happen before the board is touched.

```bash
cd /Users/alejandro/dev/nerd-o-meter
git pull

# Installs PlatformIO/esptool if needed, downloads dependencies and COMPILES once.
bash scripts/preflight.sh --install

# Now connect the Heltec V3 directly by USB-C.
bash scripts/backup.sh
bash scripts/flash.sh
```

That's the recommended path.

Optional serial diagnostics:

```bash
bash scripts/monitor.sh
```

If everything is already installed and you just want the whole sequence:

```bash
bash scripts/all.sh
```

That does **preflight → full firmware backup → flash**. It intentionally does not auto-install software; run the `--install` preflight once first.

---

## What it does

- **Wi-Fi radar:** counts visible access points with asynchronous Wi-Fi scans.
- **BLE radar:** counts nearby BLE advertisements during short passive scans.
- **Meshtastic RX:** receive-only listener for the Argentina community convention, **ANZ + LongFast @ 919.875 MHz**.
- **Mesh popup:** when a compatible packet arrives it temporarily shows node ID, RSSI, SNR, channel hash and hop info.
- **NERD LEVEL™:** combines Wi-Fi, BLE and recent Meshtastic activity into a 0–100 nonsense-but-repeatable score.
- **Buzzwords:** rotates conference-friendly terms such as MCP, RAG, agents, embeddings, multimodal, OpenTelemetry, DuckDB, unikernels...
- **Nerdearla schedule:** the Friday schedule is embedded in firmware, so it works offline.
- **GitHub QR:** points to this repository.
- **Agenda QR:** points to the official Nerdearla agenda.
- **Runtime configuration:** every carousel screen plus the LoRa popup can be turned on/off from the Heltec itself.
- **Pixel-art splash:** a custom 1-bit face + NERD-O-METER boot screen made specifically for the 128×64 OLED.

The firmware never joins discovered Wi-Fi networks, never connects to discovered BLE devices and never transmits over LoRa.

It also never enters deep sleep: if the USB power source remains on, the Nerd-O-Meter remains running.

---

## UI rotation

Default carousel:

```text
PIXEL-ART BOOT
      ↓
RADIO SCANNER
      ↓
NERD-O-METER
      ↓
BUZZWORD
      ↓
NERDEARLA NOW / NEXT
      ↓
SOURCE QR
      ↓
AGENDA QR
      ↺
```

A Meshtastic packet can temporarily interrupt the carousel with:

```text
 MESHTASTIC RX!

 FROM !A1B2C3D4
 RSSI -82  SNR +7.5
 CH 8  HOPS 2/3
 TOTAL 17
```

The popup itself is independently switchable.

---

## The one-button UX

The V3 gives us one normal user button, so the interface deliberately stays simple.

### Normal mode

- **click** → next enabled screen
- **hold ~1 second** → configuration

### Configuration

- **click** → next setting
- **double click** → toggle selected feature ON/OFF
- **hold ~1 second** → leave configuration

Configurable items:

```text
STATS
NERD LEVEL
BUZZWORD
SCHEDULE
GITHUB QR
AGENDA QR
LORA POPUP
CLOCK
```

Feature toggles are stored in ESP32 NVS, so your chosen screen selection survives a reboot.

### Setting the clock

The Heltec V3 does **not** have a battery-backed RTC. Therefore the firmware deliberately refuses to trust an old time after reboot.

After every boot:

- the schedule screen is automatically skipped until you set the clock;
- go to `CONFIG → CLOCK`;
- **double click** to enter time setup;
- click while editing hours → +1 hour;
- double click → switch to minutes;
- click while editing minutes → +5 minutes;
- hold → save.

From then on `millis()` keeps the clock moving as long as the board remains powered.

The previous manually entered hour/minute is retained only as a convenient starting point for the next setup; it is never silently treated as current time.

---

## Nerdearla schedule

The Friday Sep 25 schedule is embedded as a compact C++ table. The schedule screen:

1. shows talks that are **happening now**;
2. rotates among simultaneous rooms;
3. if nothing is active, shows the **next** start group;
4. disappears automatically when the clock has not been configured;
5. can be disabled completely in runtime config.

The snapshot was prepared from the official Nerdearla Friday schedule on Sep 25, 2026. The event website remains the source of truth for last-minute agenda changes:

https://nerdearla.com/en/argentina/schedule/?day=2026-09-25&view=list

The agenda QR uses the shorter general schedule URL so the QR remains readable on a 128×64 display.

---

## Meshtastic mode

This is **not a full second Meshtastic node**. Your LilyGO TTGO can keep doing that job.

The Heltec acts as a lightweight packet observer using the local Argentina community convention:

```text
Region/community convention : ANZ
Preset                      : LongFast
Frequency                   : 919.875 MHz
Bandwidth                   : 250 kHz
Spreading factor            : SF11
Coding rate                 : 4/5
Sync word                   : 0x2B
Preamble                    : 16
```

The receiver only reads the public 16-byte over-the-air routing header and radio metadata. It does **not** decrypt Meshtastic payloads.

That is enough for:

- packet count;
- unique sender IDs seen recently;
- sender node number;
- channel hash;
- hop limit/start;
- RSSI;
- SNR.

If your TTGO uses another modem preset or a custom frequency slot, edit the constants in `include/config.h` and rebuild.

---

## NERD LEVEL™

The formula is intentionally ridiculous but deterministic:

- Wi-Fi: **35 points max**; saturates at 120 APs.
- BLE: **35 points max**; saturates at 80 devices.
- Meshtastic: **30 points max**; saturates at 20 packets in the last 60 seconds.

So walking from the street into a technology conference should, in theory, make the nerdometer move in the correct direction.

---

## Safe backup before replacing Meshtastic

The first thing to do with the connected board is:

```bash
bash scripts/backup.sh
```

It detects the serial port, asks the ESP32-S3 about its flash, and saves a **full-flash image** under:

```text
backups/
└── heltec-v3-YYYYMMDD-HHMMSS/
    ├── flash.bin
    ├── device-info.txt
    ├── port.txt
    └── sha256.txt
```

`backups/latest` points to the newest backup.

A full-flash dump is intentional: it captures the application plus partition data and persistent Meshtastic configuration/NVS rather than only one application image.

Backups are excluded by `.gitignore`.

### Restore the previous board

```bash
bash scripts/restore.sh backups/latest
```

The restore script verifies SHA-256 and then requires you to type `RESTORE` before writing the board.

---

## Helper scripts

| Script | What it does |
|---|---|
| `preflight.sh` | Checks macOS/Apple Silicon, writable repo, free disk, Git, Homebrew, Python, PlatformIO and esptool; resolves PlatformIO packages and performs a complete test build. |
| `preflight.sh --install` | Same check, but installs missing `platformio` and `esptool` with Homebrew. |
| `detect-port.sh` | Finds the likely Heltec/CP210x serial port. |
| `backup.sh` | Creates a timestamped full-flash backup and SHA-256. |
| `build.sh` | Compiles only. |
| `flash.sh` | Builds and uploads to the detected Heltec. |
| `monitor.sh` | Opens the 115200 baud serial console. |
| `restore.sh` | Restores a full-flash backup after explicit confirmation. |
| `all.sh` | Preflight + backup + flash. |

If auto-detection ever picks the wrong serial device:

```bash
NERDOMETER_PORT=/dev/cu.usbserial-XXXX bash scripts/flash.sh
```

---

## If flashing does not start

Usually:

```bash
bash scripts/flash.sh
```

is enough.

If the ESP32-S3 does not automatically enter its bootloader:

1. hold **PRG / BOOT**;
2. tap **RST**;
3. release **PRG / BOOT**;
4. rerun `bash scripts/flash.sh`.

---

## Project layout

```text
nerd-o-meter/
├── platformio.ini
├── README.md
├── LICENSE
├── include/
│   ├── assets.h
│   ├── config.h
│   ├── radio_monitor.h
│   ├── scanners.h
│   ├── schedule.h
│   └── ui.h
├── src/
│   ├── main.cpp
│   ├── radio_monitor.cpp
│   ├── scanners.cpp
│   ├── schedule.cpp
│   └── ui.cpp
├── scripts/
│   ├── all.sh
│   ├── backup.sh
│   ├── build.sh
│   ├── detect-port.sh
│   ├── flash.sh
│   ├── lib.sh
│   ├── monitor.sh
│   ├── preflight.sh
│   └── restore.sh
├── backups/
│   └── .gitkeep
└── .github/workflows/build.yml
```

## Hardware target

- **Heltec WiFi LoRa 32 V3 / V3.1**
- ESP32-S3
- SX1262
- 0.96" 128×64 OLED
- 8 MB flash

The code uses the V3 pin mapping directly, so there is no board-specific wrapper layer to debug five minutes before leaving.

## Build manually

```bash
brew install platformio
pio run
```

## CI

GitHub Actions compiles the firmware on every push and pull request. The workflow intentionally does only one thing: prove that the checked-in firmware still builds for `heltec_wifi_lora_32_V3`.

## License

This repository keeps its original **Unlicense**.

---

**SCAN + EXPLORE + NERD**
