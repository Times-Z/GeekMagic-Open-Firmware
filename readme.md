# Geekmagic open firmware

> This repo documents the LCD interface inside the **HelloCubic Lite** cube (ESP8266) from [GeekMagicClock](https://github.com/GeekMagicClock/HelloCubic-Lite)

> It also comptabile with the **Smalltv-Ultra** from [GeekMagicClock](https://github.com/GeekMagicClock/smalltv-ultra)

[![Latest Release](https://img.shields.io/github/v/release/Times-Z/GeekMagic-Open-Firmware?label=Latest%20Version&color=c56a90&style=for-the-badge&logo=star)](https://github.com/Times-Z/GeekMagic-Open-Firmware/releases)
[![Build status main](https://img.shields.io/github/actions/workflow/status/Times-Z/GeekMagic-Open-Firmware/.github/workflows/ci.yml?branch=main&label=Pipeline%20Status%20main&style=for-the-badge&logo=star)](https://github.com/Times-Z/GeekMagic-Open-Firmware/actions)

[![Build status develop](https://img.shields.io/github/actions/workflow/status/Times-Z/GeekMagic-Open-Firmware/.github/workflows/ci.yml?branch=develop&label=Pipeline%20Status%20develop&style=for-the-badge&logo=star)](https://github.com/Times-Z/GeekMagic-Open-Firmware/actions)
[![License: GPLV3](https://img.shields.io/badge/License-GPLV3-yellow.svg?style=for-the-badge)](LICENSE)

## Table of Contents

- [Important information](#important-information)
- [Teardown](#teardown)
- [Screen hardware configuration](#screen-hardware-configuration)
    - [Display specifications](#display-specifications)
    - [Pin wiring](#pin-wiring)
    - [Important configuration details](#important-configuration-details)
- [How the screen works](#how-the-screen-works)
    - [Initialization sequence](#initialization-sequence)
    - [Communication protocol](#spi-communication-protocol)
    - [Drawing to the screen](#drawing-to-the-screen)
    - [Color format](#color-format)
- [What's next ?](#whats-next)
- [The firmware](#plateformio-firmware)
- [Install guide](#installation-guide)
- [License](#license)
- [Support](#support)

<div align="center">
   <img src=".github/assets/01-showcase.jpg" alt="HelloCubic Lite Showcase" width="1000" />
   <a href="https://www.youtube.com/watch?v=ga3Aeqo_rWA" target="_blank" ><img src=".github/assets/01-showcase.gif" alt="HelloCubic Lite Showcase" width="1000" /></a>
   <br>
    <em>open firmware showcase working on both hellocubic and smalltv</em>
</div>

## Important information

**Warning: I am not responsible for bricking your devices. Flash at your own risk**

**I recommend making a complete [backup](backup/readme.md) of your flash before doing anything**
**I've upload my factory backup (version 7.0.17 for cube and 9.0.40 for small tv); it might be useful. Backup tested and approved, it works**

## Teardown

> Version i've bought :
>
> - Cube : <https://www.aliexpress.com/item/1005007793281982.html>
> - SmallTV : <https://www.aliexpress.com/item/1005008671174445.html>

- **MCU**: ESP8266
- **LCD controller**: ST7789 (RGB565)
- **Case**: 3d printed

<div align="center">
   <img src=".github/assets/02-disassembly-cube.jpg" alt="Cube Disassembly" width="1000" />
   <img src=".github/assets/02-disassembly-tv.jpg" alt="Cube Disassembly" width="1000" />
   <br>
   <em>Cube & TV Disassembly</em>
</div>

## Screen hardware configuration

### Display specifications

- **Controller**: ST7789
- **Resolution**: 240x240 pixels
- **Color Format**: RGB565 (16-bit color)
- **Interface**: SPI (Serial Peripheral Interface)
- **SPI Speed**: 40 MHz (80 MHz is possible, but unstable and outside datasheet spec)
- **Rotation**: Upside-down for cube display, normal for the small tv

### Pin wiring

The wiring is the same form small TV and cube

The display is connected to the ESP8266 using the following GPIO pins:

| Function      | GPIO Pin | Description                                           |
| ------------- | -------- | ----------------------------------------------------- |
| **MOSI**      | GPIO 13  | SPI Master Out Slave In (data from ESP8266 to screen) |
| **SCK**       | GPIO 14  | SPI Clock                                             |
| **CS**        | GND      | Chip Select, tied permanently to GND                  |
| **DC**        | GPIO 0   | Data/Command select (LOW=command, HIGH=data)          |
| **RST**       | GPIO 2   | Reset pin                                             |
| **Backlight** | GPIO 5   | Backlight control (Active LOW)                        |

<div align="center">
   <img src=".github/assets/03-pinout.jpg" alt="Pinout Diagram" width="1000" />
   <br>
   <em>Pin Wiring Diagram (exact same for small tv)</em>
</div>

### Important configuration details

**Chip select (CS) polarity**: This board ties CS of the display permanently to GND.

**SPI mode**: SPI Mode 3 (CPOL=1, CPHA=1)

**Data/command pin**: LOW for commands, HIGH for data

**Backlight**: Active-low control - set GPIO 5 LOW to turn the backlight on, HIGH to turn it off

## How the screen works

### Initialization sequence

The firmware initializes the display through the `lcdEnsureInit()` function which performs the following steps:

1. **Backlight activation**: GPIO 5 is configured as output and driven based on the `LCD_BACKLIGHT_ACTIVE_LOW` configuration (typically driven LOW to turn on the backlight)

2. **SPI bus initialization**: Hardware SPI is initialized with:
    - Clock speed: Defined by `LCD_SPI_HZ` (typically 40 MHz)
    - Mode: Defined by `LCD_SPI_MODE` (Mode 3 required for this display)

3. **Hardware reset sequence**: The RST pin (GPIO 2) is toggled with timing:
    - Set HIGH → wait 120ms → Set LOW → wait 120ms → Set HIGH → wait 120ms

4. **Display controller initialization**: A vendor-specific initialization sequence is executed via `lcdRunVendorInit()` which includes:
    - Sleep out (0x11) with 120ms delay
    - Porch settings (0xB2) with parameters: HS=0x1F, VS=0x1F, Dummy=0x00, HBP=0x33, VBP=0x33
    - Tearing effect (0x35) set to OFF (0x00)
    - Memory access control/MADCTL (0x36) set to default (0x00)
    - Color mode (0x3A) set to 16-bit RGB565 (0x05)
    - Power control settings:
        - Power B7 (0xB7) = 0x00
        - Power BB (0xBB) = 0x36
        - Power C0 (0xC0) = 0x2C
        - Power C2 (0xC2) = 0x01
        - Power C3 (0xC3) = 0x13
        - Power C4 (0xC4) = 0x20
        - Power C6 (0xC6) = 0x13
        - Power D0 (0xD0) = 0xA4, 0xA1
        - Power D6 (0xD6) = 0xA1
    - Gamma correction (0xE0, 0xE1) with predefined curves (14 bytes each)
    - Gamma control (0xE4) = 0x1D, 0x00, 0x00
    - Display inversion (0x21)
    - Display ON (0x29)
    - Column address setup (0x2A): 0x00 to 0xEF
    - Row address setup (0x2B): 0x00 to 0xEF
    - RAM write command (0x2C)

5. **Post-initialization**:
    - 10ms delay for display stabilization
    - Display rotation is applied (from configuration via `getLCDRotationSafe()`)
    - Screen is filled with black and text color is set to white

### SPI Communication Protocol

The ST7789 communicates via SPI with the following signal handling:

1. **Clock**: SCK (GPIO 14) - drives the SPI clock at the configured frequency (40 MHz)
2. **Data**: MOSI (GPIO 13) - carries command bytes or pixel data from ESP8266 to display
3. **Chip Select**: CS is permanently tied to GND (always active)
4. **Data/Command mode**: DC pin (GPIO 0) indicates the type of data:
    - DC = LOW: Command byte follows
    - DC = HIGH: Pixel/parameter data follows

The display requires **SPI Mode 3** (CPOL=1, CPHA=1) which is explicitly configured in the initialization sequence.

### Drawing to the screen

The firmware uses the **Arduino_GFX library** with a custom ST7789 display driver. Drawing operations are managed through the global `g_lcd` instance:

1. **Text rendering**:
    - Implemented via `lcdDrawTextWrapped()` which provides word-wrapping support
    - Text wrapping algorithm handles spaces, tabs, and newlines
    - Text size is scaled by integer multipliers (6×8 pixels per character at size 1)
    - Supports automatic line wrapping with configurable character and line limits

2. **Graphics primitives**:
    - Direct access to Arduino_GFX API via `DisplayManager::getGfx()`
    - Rectangle filling: `fillRect(x, y, width, height, color)`
    - Full screen fills: `fillScreen(color)`
    - Direct SPI writes are batched between `beginWrite()` and `endWrite()` calls

3. **GIF playback**:
    - Managed via the `Gif` class instance `s_gif`
    - Supports full-screen GIF playback with optional duration limits
    - Can be stopped at any time via `DisplayManager::stopGif()`

4. **Performance optimizations**:
    - **Hardware SPI**: Uses ESP8266's hardware SPI peripheral (40 MHz) for efficient transfers
    - **Batch writes**: Commands and data are batched between `beginWrite()`/`endWrite()` calls
    - **Yield calls**: `yield()` is called during long operations to prevent watchdog timeout
    - **Direct streaming**: GIF frames are streamed directly without intermediate buffering

### Color format

The display uses **RGB565** (16-bit) color encoding:

- **Red channel**: 5 bits (bits 15-11)
- **Green channel**: 6 bits (bits 10-5)
- **Blue channel**: 5 bits (bits 4-0)

This format provides 65,536 distinct colors and is the standard for ST7789 displays.

**Common color constants** (defined in DisplayManager.h):

- Black: `0x0000`
- White: `0xFFFF`
- Red: `0xF800`
- Green: `0x07E0`
- Blue: `0x001F`
- Cyan: `0x07FF`
- Magenta: `0xF81F`
- Yellow: `0xFFE0`

    etc...

## What's next ?

Okay, now we have a minimal firmware that works.

I really like ESP devices and I really enjoy working with ESP-IDF, so I’m planning ~~if possible (I haven’t checked compatibility yet, I’m still new to this world)~~ to create a firmware close to the original one in terms of features, but fully open source ofc \o/

Since ESP IDF is not compatible with esp8266, i'm going to build the firmware on top of [plateformIO](https://platformio.org/)

That’s the project, at least

## PlateformIO Firmware

This is the "real" firmware I want to improve, with clean and reliable code

### Stack

| Component         | Technology / Library                                                     | Main Role                               |
| ----------------- | ------------------------------------------------------------------------ | --------------------------------------- |
| Microcontroller   | ESP8266 (esp12e)                                                         | Main hardware platform                  |
| Build environment | PlatformIO                                                               | Project management, build, upload       |
| Framework         | Arduino Framework                                                        | Software base for ESP8266               |
| Filesystem        | LittleFS                                                                 | Local storage LittleFS                  |
| Graphics display  | Arduino_GFX Library                                                      | ST7789 display management (SPI, RGB565) |
| Web UI (frontend) | [Pico.css](https://picocss.com/docs), [Alpine.js](https://alpinejs.dev/) | Minimalist web user interface           |

## Installation Guide

To use the open-source firmware on your compatible GeekMagic device, follow these steps:

### 1. Clone the repository

```bash
git clone https://github.com/Times-Z/GeekMagic-Open-Firmware.git
```

### 2. Configure the JSON file

```bash
cp data/config-{hellocubic|smalltv}.example data/config.json
```

You can edit this JSON file to configure your firmware, for example by modifying `wifi_ssid` and `wifi_password` so that your device connects to your network

Note: The Wi-Fi credentials and API token in config.json are migrated to EEPROM "secure storage" on first boot. After that, these credentials are erased from config.json

**Configuration options:**

- `wifi_ssid`: Your WiFi network name
- `wifi_password`: Your WiFi password
- `api_token`: Bearer token for API authentication
- `lcd_rotation`: Display rotation setting
- `ntp_server`: NTP server for time synchronization

Security of stored secrets:

- All sensitive values (API keys, wifi credentials, tokens, etc.) are stored in EEPROM using a device-unique obfuscation scheme
- The obfuscation key is derived from the ESP8266's MAC address, chip ID, and a salt (configurable [here](./src/main.cpp#L41) in code) using SHA-256
- The JSON payload is XORed with this derived key before being written to EEPROM, and de-obfuscated on read
- This makes it much harder to recover secrets from a raw flash dump on another device, or with only partial knowledge of the hardware

Limitations:

- This is not true encryption and does not provide hardware-backed security or protection against a determined attacker with full device access
- The public salt is not secret; if an attacker knows the salt, MAC, and chip ID, they can reconstruct the key
- There is no secure element or tamper resistance

**Do not assume confidentiality against a determined attacker**

Warning: If the salt, the chip ID or the mac address change, the eeprom secure storage is clear and reset to ensure no data leak is possible

### 3. Build the firmware and filesystem

To build the firmware you can use decontainer, docker or build it by yourself using [PlateformIO](https://docs.platformio.org/en/latest/core/installation/methods/installer-script.html)

```bash
pio run && pio run --target buildfs

# or using devcontainer aliases

build && buildfs

# or docker

./scripts/build-with-docker.sh
```

The generated files will be located in:

```
.pio/build/esp12e/
```

### 4. Flash the firmware

There are two possible flashing methods:

- **OTA (Over-The-Air)** – no prerequisites
- **USB** – requires a USB-to-TTL converter to connect to the ESP

---

### OTA Flashing

Flashing is done in two steps

#### Step 1: Flash the firmware

Go to:

```
http://{your_geekmagic_ip}/update
```

This is the update endpoint of the original firmware. Upload the `firmware.bin` file

Once this step is complete, the device will reboot. Depending on the model, the screen orientation may be correct or not — this is normal

At this point, only the firmware has been flashed. The filesystem (configuration, web app) still needs to be flashed

#### Step 2: Flash the filesystem

The firmware will create a Wi‑Fi access point with the following credentials:

- **SSID:** `GeekMagic`
- **Password:** `$str0ngPa$$w0rd`

Connect to this access point and go to:

```
http://192.168.4.1/legacyupdate
```

From there, select and flash the filesystem using the `littlefs.bin` file

Once the device reboots, the setup is complete!

## License

This project is licensed under the **GPLv3 License** - see the [LICENSE](LICENSE) file for details

---

## Support

- Found a bug or question ? [Open an issue](https://github.com/Times-Z/GeekMagic-Open-Firmware/issues)

---

<div align="center">

**Made with ❤️**

[Star us on GitHub](https://github.com/Times-Z/GeekMagic-Open-Firmware.git) if you find this project useful !

This project took me a lot of time !

</div>
