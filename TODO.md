# SKR 1.4 Turbo Bring-up TODO

## Done

- [x] Add `skr14turbo` PlatformIO environment.
- [x] Restore stable USB CDC COM enumeration path on LPC1769.
- [x] Keep LX200/OATControl command I/O on USB CDC (`UsbSerial`) for SKR.
- [x] Enable TMC2209 UART on X/RA and Y/DEC using SKR PDN pins.
- [x] Enable full-axis TMC UART pin mapping for Z/AZ, E0/ALT, E1/FOCUS.
- [x] Add GPS diagnostics commands (`:XGP#`, `:XGQ#`) for transport/parser troubleshooting.
- [x] Add optional I2C GPS mode (`USE_GPS_I2C_ADAFRUIT`) for SKR builds.
- [x] Enable gyro build path compatibility on LPC1769 (`Wire` API adjustments).
- [x] Keep RA homing support enabled.
- [x] Document SKR power/USB behavior and I2C GPS usage in `README.md`.

## Next

- [ ] Validate field behavior with all enabled options at once:
  - RA/DEC tracking + slew
  - AZ/ALT AutoPA movement
  - Focuser movement
  - GPS acquisition and time/location sync (I2C mode, real sky test)
  - Gyro level readout
- [ ] Add dedicated board documentation for recommended connector/pin wiring.
- [ ] Add optional runtime report for active GPS transport (UART vs I2C) and active address/pin mapping.
- [ ] Add support for `mini12864 v3` on SKR (EXP1/EXP2 mapping + UI integration).
