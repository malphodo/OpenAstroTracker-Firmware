# OpenAstroTracker Serial Commands

This firmware implements the Meade LX200 command style (`:<command>#`) plus OpenAstroTracker extensions (`:X...#`).

- Transport: serial UART
- Default monitor speed in this repo: `19200` (`platformio.ini`)
- Command framing: starts with `:`, ends with `#`

---

## Core LX200-Compatible Commands

### Initialize / Status

- `:I#` initialize serial-control mode
- `:D#` query mount slewing status (`|#` or ` #`)

### Get Coordinates / Time / Site

- `:GR#` current RA
- `:GD#` current DEC
- `:Gr#` target RA
- `:Gd#` target DEC
- `:GL#` local time (24h)
- `:Ga#` local time (12h)
- `:GC#` local date
- `:GG#` UTC offset
- `:Gt#` latitude
- `:Gg#` longitude
- `:GVP#` product string
- `:GVN#` firmware version

### Set Coordinates / Time / Site

- `:SrHH:MM:SS#` set target RA
- `:SdsDD*MM:SS#` set target DEC
- `:SLHH:MM:SS#` set local time
- `:SCMM/DD/YY#` set local date
- `:SGsHH#` set UTC offset
- `:StsDD*MM#` set latitude
- `:SgsDDD*MM#` set longitude

### Sync / Slew / Motion

- `:CM#` sync current position to target coordinates
- `:MS#` slew to target
- `:Mn#`, `:Ms#`, `:Me#`, `:Mw#` manual slew in direction
- `:MT1#` / `:MT0#` tracking on/off
- `:MGdnnnn#` guide pulse
- `:Q#` stop all
- `:Qd#` stop axis direction
- `:Qq#` leave serial-control mode

### Home / Park

- `:hF#` go home
- `:hP#` park
- `:hU#` unpark (tracking on)

### Slew Rate

- `:RS#` fastest
- `:RM#` find
- `:RC#` center
- `:RG#` guide

### Focuser (when enabled)

- `:F+#` move in
- `:F-#` move out
- `:Fn#` speed 1..4
- `:FF#` fastest speed
- `:FS#` slowest speed
- `:Fp#` read position
- `:FPnnn#` set position (no move)
- `:FB#` moving state
- `:FQ#` stop focuser

---

## OpenAstroTracker `:X...#` Extensions

### Mount / Motion / Limits / Calibration

- `:GX#` mount status summary
- `:GIS#` RA/DEC slewing state
- `:GIT#` tracking state
- `:GIG#` guiding state
- `:XGR#`, `:XGD#`, `:XGZ#`, `:XGA#` get steps/deg by axis
- `:XSR...#`, `:XSD...#`, `:XSZ...#`, `:XSA...#` set steps/deg by axis
- `:XSSn.nnn#` set speed calibration
- `:XGS#` read speed calibration
- `:XGST#` remaining safe time
- `:XGT#` absolute tracking speed
- `:XSBn#` backlash steps
- `:XGB#` read backlash steps
- `:XSDL...#` set/clear DEC limits
- `:XGDL#`, `:XGDLU#`, `:XGDLL#` read DEC limits

### Manual Slew / Raw Stepper Moves

- `:XSMn#` manual slewing mode on/off
- `:XSXn.nnn#` set RA manual speed
- `:XSYn.nnn#` set DEC manual speed
- `:MXxnnnnn#` move a stepper by steps (`r,d,f,z,l`)

### HA / LST / Home Offsets

- `:SHHH:MM#` set HA
- `:SHLHH:MM#` set LST
- `:XGH#` get HA
- `:XGL#` get LST
- `:XGHR#`, `:XGHD#` read RA/DEC homing offsets
- `:XSHRnnn#`, `:XSHDnnn#` set RA/DEC homing offsets

### GPS Diagnostics

- `:gT#` or `:gTnnn#` GPS sync attempt
- `:XGP#` parser diagnostics (`bytes,sentences,sats,valid,age,lastByte#`)
- `:XGQ#` recent raw GPS bytes hex dump

### Hall Sensor / Homing Diagnostics

- `:XGAA#` get AZ/ALT positions
- `:XGAH#` get autohoming state
- `:XGHL#` read raw hall GPIO levels
- `:XGHT#` read hall triggered logic levels
- `:XGHU#` hall 5-sample debug
- `:MHRx#`, `:MHDx#` hall-based homing commands

### LCD / Display Commands

- `:XGLC#` get LCD contrast value
- `:XSCnnn#` set LCD contrast
- `:XGLE#` get display enabled (`0/1`)
- `:XSLE0#` / `:XSLE1#` disable/enable display backlight
- `:XSI0#` / `:XSI1#` display invert off/on
- `:XLT#` visibility test pattern
- `:XSW0#` / `:XSW1#` LCD controller power-save off/on (runtime sleep/wake)

### Mini12864 V3 RGB / LED Commands

- `:XGUE#` RGB enabled state
- `:XGUC#` RGB color `R,G,B#`
- `:XGUR#` RGB enabled and color `enabled,R,G,B#`
- `:XGUM#` read preset mode
- `:XSUE0#` / `:XSUE1#` RGB off/on
- `:XSUCr,g,b#` set RGB color
- `:XSURnnn#`, `:XSUGnnn#`, `:XSUBnnn#` set R/G/B channels
- `:XSMn#` set Mini12864 preset mode (`0..4`)
- `:XSRrrrgggbbb#` set full RGB by triplets
- `:XSLNrrrgggbbb#` set single LED (`N=1..3`)

### Button LED Blink Modes (recent additions)

- `:XSB0#` no button blink mode
- `:XSB1#` warning mode (button LEDs alternate blue/red)
- `:XSB2#` edit mode (button LEDs blink red)
- `:XSE0#` clear forced edit-blink
- `:XSE1#` force edit-blink
- `:XGUF#` read base button blink mode (`0..2`)
- `:XGUX#` read forced edit-blink state (`0/1`)

### JogSens (recent additions)

- `:XGJ#` read JogSens (`1..8`)
- `:XSJn#` set JogSens (`1..8`, clamped, persisted to EEPROM)

### Misc

- `:XFR#` factory reset persistent config
- `:XGN#` network status (when WiFi enabled)

---

## Notes

- Some commands are feature-gated by compile-time options (GPS, focuser, RGB, hall sensors, etc.).
- Unknown/unsupported commands may return empty string, `0#`, or protocol-specific fallback depending on command family.
- The canonical implementation lives in `src/MeadeCommandProcessor.cpp`.
