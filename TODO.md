# SKR 1.4 Turbo Bring-up TODO

## Done

- [x] Add `skr14turbo` PlatformIO environment.
- [x] Restore stable USB CDC COM enumeration path on LPC1769.
- [x] Keep LX200/OATControl command I/O on USB CDC (`UsbSerial`) for SKR.
- [x] Enable TMC2209 UART on X/RA and Y/DEC using SKR PDN pins.
- [x] Enable full-axis TMC UART pin mapping for Z/AZ, E0/ALT, E1/FOCUS.
- [x] Enable GPS support for SKR with SoftwareSerial option.
- [x] Enable gyro build path compatibility on LPC1769 (`Wire` API adjustments).
- [x] Keep RA homing support enabled.

## Next

- [ ] Validate field behavior with all enabled options at once:
  - RA/DEC tracking + slew
  - AZ/ALT AutoPA movement
  - Focuser movement
  - GPS acquisition and time/location sync
  - Gyro level readout
- [ ] Add dedicated board documentation for recommended connector/pin wiring.
- [ ] Add optional debug command/report for active GPS source and UART pin mapping.
- [ ] Add support for `mini12864 v3` on SKR (EXP1/EXP2 mapping + UI integration).
