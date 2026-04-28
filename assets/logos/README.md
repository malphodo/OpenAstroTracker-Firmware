# Visual Logos Assets

This folder centralizes all visual logo assets used by the project.

- `svg/`: editable source logos (preferred authoring format)
- `xbm/`: monochrome bitmaps ready to embed in firmware display code

Current files:
- `svg/config_gear.svg`
- `xbm/config_gear_32.xbm`

Notes:
- Firmware does not load SVG files dynamically at runtime.
- XBM files are regenerated automatically from the SVG sources during PlatformIO builds.
- These assets are source files for development and are converted/embedded in code before compilation.