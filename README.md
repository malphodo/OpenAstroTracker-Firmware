# OpenAstroTracker-Firmware
Official firmware for the OpenAstroTracker. Other important resources can be found [here](https://wiki.openastrotech.com/en/Links).

## Change log
See the [Changelog](Changelog.md) for details about what versions made what changes.

## Coding guidelines

See `.clang-format` file. A GitHub action is run on every PR to make sure that the code complies with the formatting guidelines.

### Run clang-format locally
* Install `clang-format` version 12. _Note: not all distributions default to version 12_
  * Windows: Installers available from [LLVM Website](https://llvm.org/builds/)
  * Ubuntu: `sudo apt install clang-format-12`
  * ArchLinux: `sudo pacman -S clang`
* Run the formatter: 
  * VSCode Extension: [https://marketplace.visualstudio.com/items?itemName=xaver.clang-format](https://marketplace.visualstudio.com/items?itemName=xaver.clang-format)
  * Shell: `bash -c 'shopt -s nullglob globstar;GLOBIGNORE=./src/libs/TimerInterrupt/*; for i in ./{.,src/**,unit_tests,boards/**}/*.{c,cpp,h,hpp}; do clang-format -i $i; done'`
## Contribution

This is an open source project and everyone is welcome to contribute. We will be following these rules while reviewing your pull request:
- The pull request consists **only** of the **changes related to its particular feature or bugfix**. If there are multiple unrelated changes which should be merged into this repository, you have to create a separate pull request for each of them. 
- The pull request **builds correctly**. If it doesn't, please fix the issues and push them to the source branch. You can use the matrix_build.py script to build all the important configurations locally (works similar to our CI).
- The pull request can only be merged **after** all comments were resolved BY **OAT DEVELOPERS**. Please don't resolve the comments yourself since this can lead to missed issues.
- If the pull request is not maintained by its author in a reasonably prompt manner after a review, the developers can decide to close it without merging since the accumulated merge conflicts and original code changes could lead to massive efforts. You can then still recreate your pull request after applying all the required changes on your fork branch.

## Development

Even if Arduino IDE is supported, we highly recommend using VSCode with [PlatformIO](https://platformio.org/) for development. It allows automatic dependency management, powerful IDE, debugging, automatic build flags definition and more.

### Debugging

#### ATmega2560-based

> :warning: **Debugging is only supported on mega2560 platforms at the moment!**

For this example we will be using the `ramps` environment, but you can use any derived environment as well

> You may need to set `debug_port` in your `platformio.ini`, platformio says it will auto-detect the port but it doesn't seem to be working at the moment

Start a gdb shell debugging the current firmware:
```shell
pio run -e ramps -t clean  # Clean the environment
piodebuggdb -e ramps  # Initialize a debug session.
# This will build the firmware in debug mode, and then initialize a remote gdb session
# You may have visual debug capabilities in your IDE if it has platformio integration as well
```

When using `avr-stub` as a debug interface, it requires 2 things:
1. Serial link 0 must not be used in the firmware
    - As such, all external interfaces using `Serial` are disabled in a debug build (`Serial1`, `Serial2` etc are ok)
2. Exclusive access to an interrupt vector
    - This requires hot-patching the arduino framework (specifically `WInterrupts.c`) to disable the ISR registration. The implementation of this is in `pre_script_patch_debug.py`, which should happen automagically

> Note that while avr-stub is in RAM mode, the firmware will run very slowly and timing-related functions might not work correctly

Debugging is still a bit flakey, so you may need to try multiple times in order to get a solid debugging session.

More information is available in the [avr-stub documentation](https://github.com/jdolinay/avr_debug/tree/master/doc)

### SKR 1.4 Turbo power and USB notes

For the `skr14turbo` environment, USB serial visibility and motor movement depend on correct board power configuration:

- Provide the main board supply on `VIN` at 12V (or 24V according to your setup). 9V can boot the MCU but is often not stable enough for stepper operation.
- Set the 5V source jumper according to your usage:
  - `5V USB`: recommended when connected to a PC and using OATControl over USB (CDC COM port enumeration).
  - `5V VDD`/internal 5V: only when your external power setup provides stable 5V logic power.
- If the COM port does not appear in Windows, first verify USB data cable and jumper position before debugging firmware.
- A small idle motor pulse/noise with one jumper position and no motion with another usually indicates a power-path issue, not a Meade protocol issue.

### SKR 1.4 Turbo GPS over I2C (Adafruit-style)

The `skr14turbo` profile now supports an I2C GPS transport path intended for Adafruit-style GPS devices exposing NMEA data over I2C.

#### Configuration

In `Configuration_local.hpp`:

- `USE_GPS 1`
- `USE_GPS_I2C_ADAFRUIT 1`
- `GPS_I2C_ADDRESS 0x10` (change this if your module uses another address)

When `USE_GPS_I2C_ADAFRUIT` is enabled, the firmware reads GPS data from I2C and feeds TinyGPS++ directly.
The UART GPS path remains available if `USE_GPS_I2C_ADAFRUIT` is set to `0`.

#### Wiring

- GPS `VCC` -> board `VCC` (according to module voltage requirements)
- GPS `GND` -> board `GND`
- GPS `SDA` -> board `SDA`
- GPS `SCL` -> board `SCL`

Keep I2C wiring short and use a clean shared ground. If instability is observed, add local decoupling close to the GPS module.

#### Runtime test commands (Meade)

- `:gT30000#` -> attempt GPS acquisition for 30 seconds (`1` on success, `0` on timeout)
- `:Gt#` -> current latitude
- `:Gg#` -> current longitude
- `:XGP#` -> GPS parser diagnostics: `bytes,sentences,sats,valid,age_ms,last_byte#`
- `:XGQ#` -> raw recent bytes in hex for troubleshooting: `n,HH.HH...#`

Interpretation hints:

- If `sentences` stays at `0` while `bytes` increases, transport data is present but not parsed as valid NMEA.
- If both `bytes` and `sentences` stay near `0`, check I2C wiring/address/power.

### SKR EXP 12864 boot diagnostics

For `skr14turbo`, a boot-only EXP display diagnostic can be enabled with:

- `USE_MINI12864_BOOT_SPLASH 1` in `Configuration_local.hpp`

When enabled, firmware runs a short splash sequence using U8g2 and cycles through common SKR EXP pin mappings (`MAP A`..`MAP D`) to help identify wiring and controller compatibility.

This diagnostic is independent from `DISPLAY_TYPE`, so you can keep the normal menu disabled while validating hardware.

If the display backlight turns on but no text appears:

- verify EXP cable orientation,
- test contrast adjustment on classic 12864 modules,
- confirm that the display controller is compatible with the current ST7567 driver path.

### SKR EXP runtime display setup (U8g2)

The firmware supports runtime menu rendering on SPI EXP displays with digital rotary encoder input.

#### Display type selection

In `Configuration_local.hpp`, set one of:

- `#define DISPLAY_TYPE DISPLAY_TYPE_MINI12864_V2_EXP` (Mini12864 V2/ST7920 over U8g2 **software SPI**; improved compatibility vs parallel-mode init)
- `#define DISPLAY_TYPE DISPLAY_TYPE_MINI12864_ST7567_EXP` (Mini12864 ST7567 path)
- `#define DISPLAY_TYPE DISPLAY_TYPE_12864_ST7920_EXP` (classic 12864 ST7920 path)

Optional:

- `#define DISPLAY_FLIP_180 1` to rotate display orientation by 180 degrees.

#### EXP wiring notes (SKR 1.4 Turbo)

- SPI lines: use EXP header mapping (`SCK`, `MOSI`, `CS`, optional `DC`, optional `RST`) from board pin defines.
- Encoder: `ENCODER_A`, `ENCODER_B`, `ENCODER_BTN` are read as digital inputs with pull-ups.
- Logic level is **3.3V** on LPC1769 pins. Avoid feeding 5V logic directly to signal pins.
- If screen powers but stays blank, check ribbon orientation and try cable inversion where applicable.

#### Minimal example

```cpp
#define DISPLAY_TYPE DISPLAY_TYPE_MINI12864_V2_EXP
#define DISPLAY_FLIP_180 0
```

#### Validation checklist (for PR / hardware bring-up)

- Boot: initial screen and startup menu are visible.
- Encoder rotation: menu navigation is smooth and deterministic.
- Encoder push: select/enter action is recognized reliably.
- No endstop/display pin collision in active configuration.
- Serial logs show selected `DISPLAY_TYPE` and active pin mapping at boot.

### Meade Command Documentation
The Meade commands page on the Wiki is generated by running:
```shell
python .\scripts\MeadeCommandParser.py
```
from the main directory. The page is generated in the scripts folder and needs to be copy pasted into the Wiki manually. Please add the version manually (for now) by pasting something like this:
```
> This documentation is current as of Firmware **V1.13.9**
{.is-warning}
```
