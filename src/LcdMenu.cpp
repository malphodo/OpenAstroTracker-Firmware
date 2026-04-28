#include "inc/Globals.hpp"
#include "../Configuration.hpp"
#include "Utility.hpp"
#include "libs/MappedDict/MappedDict.hpp"
#include "EPROMStore.hpp"
#include "LcdMenu.hpp"
#include "../assets/logos/xbm/config_gear_32.xbm"

#if DISPLAY_TYPE != DISPLAY_TYPE_NONE

    #if DISPLAY_FLIP_180 == 1
        #define LCD_U8G2_ROT U8G2_R2
    #else
        #define LCD_U8G2_ROT U8G2_R0
    #endif
    #ifndef LCD12864_RST_PIN
        #define LCD12864_RST_PIN U8X8_PIN_NONE
    #endif
    #ifndef LCD12864_ST7920_SCK_PIN
        #define LCD12864_ST7920_SCK_PIN LCD12864_SCK_PIN
    #endif
    #ifndef LCD12864_ST7920_MOSI_PIN
        #define LCD12864_ST7920_MOSI_PIN LCD12864_MOSI_PIN
    #endif
    #ifndef LCD12864_ST7920_CS_PIN
        #define LCD12864_ST7920_CS_PIN LCD12864_CS_PIN
    #endif
    #ifndef MINI12864_V3_RGB_PIN
        #if (MINI12864_VARIANT == MINI12864_VARIANT_V3) && defined(LCD12864_CS_PIN)
            #define MINI12864_V3_RGB_PIN LCD12864_CS_PIN
        #else
            #define MINI12864_V3_RGB_PIN -1
        #endif
    #endif
    #ifndef MINI12864_V2_BACKLIGHT_PIN
        #define MINI12864_V2_BACKLIGHT_PIN -1
    #endif
    #ifndef MINI12864_V2_BACKLIGHT_ACTIVE_HIGH
        #define MINI12864_V2_BACKLIGHT_ACTIVE_HIGH 1
    #endif

static inline bool mini12864V2BacklightPinValid()
{
    return MINI12864_V2_BACKLIGHT_PIN >= 0;
}

#if (MINI12864_VARIANT == MINI12864_VARIANT_V3)
    #ifndef MINI12864_V3_LED_COUNT
        // BTT Mini12864 V3 has 3 daisy-chained WS2812 LEDs: 2 behind the LCD + 1 on the encoder.
        #define MINI12864_V3_LED_COUNT 3
    #endif

    static inline bool mini12864RgbPinValid()
    {
        return MINI12864_V3_RGB_PIN >= 0;
    }

    static inline void writeMini12864NeopixelByte(uint8_t value)
    {
        for (uint8_t mask = 0x80; mask != 0; mask >>= 1)
        {
            digitalWrite(MINI12864_V3_RGB_PIN, HIGH);
            if ((value & mask) != 0)
            {
                delayMicroseconds(1);
            }
            digitalWrite(MINI12864_V3_RGB_PIN, LOW);
            delayMicroseconds(1);
        }
    }

    // Note: standard WS2812/WS2812B chips expect bytes in GRB order, but the WS2812-style
    // driver on the BTT Mini12864 V3 has been observed to interpret the stream as RGB. This
    // is tested empirically (R=255 produced green in GRB mode). Change WIRE_USES_GRB to 1
    // only if the hardware is confirmed to be strict GRB.
    #ifndef MINI12864_V3_WIRE_USES_GRB
        #define MINI12864_V3_WIRE_USES_GRB 0
    #endif

    static inline void writeMini12864OneLed(uint8_t r, uint8_t g, uint8_t b)
    {
        #if MINI12864_V3_WIRE_USES_GRB
        writeMini12864NeopixelByte(g);
        writeMini12864NeopixelByte(r);
        writeMini12864NeopixelByte(b);
        #else
        writeMini12864NeopixelByte(r);
        writeMini12864NeopixelByte(g);
        writeMini12864NeopixelByte(b);
        #endif
    }

    static void writeMini12864RgbNow(uint8_t r, uint8_t g, uint8_t b)
    {
        if (!mini12864RgbPinValid())
        {
            return;
        }

        noInterrupts();
        for (uint8_t i = 0; i < MINI12864_V3_LED_COUNT; i++)
        {
            writeMini12864OneLed(r, g, b);
        }
        interrupts();
        delayMicroseconds(80);  // Latch
    }

    // Per-LED variant: takes an array of MINI12864_V3_LED_COUNT * 3 bytes in R,G,B order.
    static void writeMini12864RgbChain(const uint8_t *rgbTriplets)
    {
        if (!mini12864RgbPinValid())
        {
            return;
        }

        noInterrupts();
        for (uint8_t i = 0; i < MINI12864_V3_LED_COUNT; i++)
        {
            uint8_t r = rgbTriplets[i * 3 + 0];
            uint8_t g = rgbTriplets[i * 3 + 1];
            uint8_t b = rgbTriplets[i * 3 + 2];
            writeMini12864OneLed(r, g, b);
        }
        interrupts();
        delayMicroseconds(80);  // Latch
    }
#endif

    // Class that drives the LCD screen with a menu
    // You add a string and an id item and this class handles the display and navigation
    // Create a new menu, using the given number of LCD display columns and rows
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD
LcdMenu::LcdMenu(byte cols, byte rows, int maxItems)
    : _lcd(LCD_PIN8, LCD_PIN9, LCD_PIN4, LCD_PIN5, LCD_PIN6, LCD_PIN7), _cols(cols), _rows(rows), _maxItems(maxItems),
      _charHeightRows(1)  // 1 character = 1 row
{
}
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23008 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23017
LcdMenu::LcdMenu(byte cols, byte rows, int maxItems)
    : _lcd(0x20), _cols(cols), _rows(rows), _maxItems(maxItems), _charHeightRows(1)  // 1 character = 1 row
{
        #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23017
    _lcd.setMCPType(LTI_TYPE_MCP23017);
        #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23008
    _lcd.setMCPType(LTI_TYPE_MCP23008);
        #endif
}
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_JOY_I2C_SSD1306
LcdMenu::LcdMenu(byte cols, byte rows, int maxItems)
    : _cols(cols), _rows(rows), _maxItems(maxItems), _charHeightRows(2)  // For 7x14 font 1 character = 2 rows (2x8 pixels)
{
}
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567
LcdMenu::LcdMenu(byte cols, byte rows, int maxItems)
    : _lcd(LCD_U8G2_ROT, LCD12864_SCK_PIN, LCD12864_MOSI_PIN, LCD12864_CS_PIN, LCD12864_DC_PIN, LCD12864_RST_PIN),
      _cols(cols), _rows(rows), _maxItems(maxItems), _charHeightRows(1)
{
}
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701
LcdMenu::LcdMenu(byte cols, byte rows, int maxItems)
    : _lcd(LCD_U8G2_ROT, LCD12864_SCK_PIN, LCD12864_MOSI_PIN, LCD12864_CS_PIN, LCD12864_DC_PIN, LCD12864_RST_PIN),
      _cols(cols), _rows(rows), _maxItems(maxItems), _charHeightRows(1)
{
}
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920
LcdMenu::LcdMenu(byte cols, byte rows, int maxItems)
    : _lcd(LCD_U8G2_ROT, LCD12864_ST7920_SCK_PIN, LCD12864_ST7920_MOSI_PIN, LCD12864_ST7920_CS_PIN, LCD12864_RST_PIN),
      _cols(cols), _rows(rows), _maxItems(maxItems), _charHeightRows(1)
{
}
    #elif DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
        // Mini12864 V2/V3 uses an ST7567 (or UC1701) controller in 4-wire software SPI mode.
        // Default pin mapping is provided by the SKR pins header (LCD12864_MINI_*_PIN).
        #ifndef LCD12864_MINI_SCK_PIN
            #define LCD12864_MINI_SCK_PIN LCD12864_SCK_PIN
        #endif
        #ifndef LCD12864_MINI_MOSI_PIN
            #define LCD12864_MINI_MOSI_PIN LCD12864_MOSI_PIN
        #endif
        #ifndef LCD12864_MINI_CS_PIN
            #define LCD12864_MINI_CS_PIN LCD12864_CS_PIN
        #endif
        #ifndef LCD12864_MINI_DC_PIN
            #define LCD12864_MINI_DC_PIN LCD12864_DC_PIN
        #endif
        #ifndef LCD12864_MINI_RST_PIN
            #define LCD12864_MINI_RST_PIN LCD12864_RST_PIN
        #endif
LcdMenu::LcdMenu(byte cols, byte rows, int maxItems)
    : _lcd(LCD_U8G2_ROT, LCD12864_MINI_SCK_PIN, LCD12864_MINI_MOSI_PIN, LCD12864_MINI_CS_PIN, LCD12864_MINI_DC_PIN, LCD12864_MINI_RST_PIN),
      _cols(cols), _rows(rows), _maxItems(maxItems), _charHeightRows(1)
{
}
    #endif

void LcdMenu::startup()
{
    LOG(DEBUG_INFO, "[LCD]: LcdMenu startup");

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD
    _lcd.begin(_cols, _rows);
        #if defined(LCD_BRIGHTNESS_PIN)
    _lcdBadHw = testIfLcdIsBad();
        #endif
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23008 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23017
    _lcd.begin(_cols, _rows);
    _lcd.setBacklight(RED);
    _lcdBadHw = false;
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_JOY_I2C_SSD1306
    _lcd.begin();
    _lcd.setPowerSave(0);
    _lcd.clear();
    _lcd.setFont(u8x8_font_7x14_1x2_f);  // Each 7x14 character takes up 2 8-pixel rows
    _lcdBadHw = false;
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                       \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    #if DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    // Some Mini12864 boards route an LCD reset line to EXP1_5 (P1_20). BTT V3 does not,
    // but pulsing it LOW->HIGH is harmless when unconnected and rescues V2 clones that would
    // otherwise boot with the controller stuck in reset.
    pinMode(P1_20, OUTPUT);
    digitalWrite(P1_20, LOW);
    delay(10);
    digitalWrite(P1_20, HIGH);
    delay(50);
    LOG(DEBUG_INFO, "[LCD]: Mini12864 pulse-reset on P1_20 done");
    #endif
    _lcd.begin();
    _lcd.setPowerSave(0);
    #if DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    // Apply display-wide pixel polarity. ST7565/ST7567/UC1701 share commands:
    //   0xA6 = normal (drawn = dark on clear)
    //   0xA7 = inverse (drawn = clear on dark)
    _lcd.sendF("c", (MINI12864_INVERT_DISPLAY != 0) ? 0xA7 : 0xA6);
    // Force a reasonable contrast at boot so the panel is legible regardless of any EEPROM state.
    _lcd.setContrast(MINI12864_CONTRAST);
        #if MINI12864_CONTROLLER == MINI12864_CONTROLLER_UC1701
    LOG(DEBUG_INFO, "[LCD]: Mini12864 controller=UC1701, contrast=%d", (int) MINI12864_CONTRAST);
        #elif MINI12864_CONTROLLER == MINI12864_CONTROLLER_ST7565
    LOG(DEBUG_INFO, "[LCD]: Mini12864 controller=ST7565, contrast=%d", (int) MINI12864_CONTRAST);
        #else
    LOG(DEBUG_INFO, "[LCD]: Mini12864 controller=ST7567, contrast=%d", (int) MINI12864_CONTRAST);
        #endif
    LOG(DEBUG_INFO, "[LCD]: Mini12864 pins SCK=%d MOSI=%d CS=%d DC=%d RST=%d",
        (int) LCD12864_MINI_SCK_PIN, (int) LCD12864_MINI_MOSI_PIN, (int) LCD12864_MINI_CS_PIN,
        (int) LCD12864_MINI_DC_PIN, (int) LCD12864_MINI_RST_PIN);
    #endif
    _lcd.setFont(u8g2_font_6x12_tf);
    _lcd.clearBuffer();
    #if DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    _lcd.drawStr(0, 14, "OpenAstroTracker");
    _lcd.drawFrame(0, 0, 128, 64);  // Visible border helps confirm the panel is actually driven.
    #endif
    _lcd.sendBuffer();
    _lcdBadHw = false;
    #endif

    _brightness = EEPROMStore::getBrightness();
    LOG(DEBUG_INFO, "[LCD]: Brightness from EEPROM is %d", _brightness);
    #if DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    // With USE_DUMMY_EEPROM (and first boot on real EEPROM), getBrightness() returns 0 which
    // would immediately blank the Mini12864 panel via setContrast(0). Fall back to the configured
    // boot contrast so the display stays visible until the user tunes it.
    if (_brightness <= 0 || _brightness > 255)
    {
        _brightness = (int) MINI12864_CONTRAST;
        LOG(DEBUG_INFO, "[LCD]: EEPROM brightness invalid, using MINI12864_CONTRAST=%d", _brightness);
    }
    #endif
    _backlightEnabled = (_brightness > 0);
    _lastNonZeroBrightness = (_brightness > 0) ? _brightness : 180;
    setBacklightBrightness(_brightness, false);

    _numMenuItems    = 0;
    _activeMenuIndex = 0;
    _longestDisplay  = 0;
    _columns         = _cols;
    _activeRow       = -1;
    _activeCol       = -1;
    for (byte i = 0; i < MAX_DISPLAY_ROWS; i++)
    {
        _lastDisplay[i] = "";
    }
    _menuItems       = new MenuItem *[_maxItems];
    _mini12864RgbEnabled = false;
    _mini12864RgbR       = 0;
    _mini12864RgbG       = 0;
    _mini12864RgbB       = 0;
    for (uint8_t i = 0; i < sizeof(_mini12864LedRgb); i++)
    {
        _mini12864LedRgb[i] = 0;
    }
    _mini12864BacklightMode = MINI12864_MODE_DEFAULT;
    _mini12864ButtonBlinkMode = MINI12864_BUTTON_BLINK_NONE;
    _mini12864EditBlinkActive = false;
    _mini12864ButtonBlinkRunning = false;
    for (uint8_t i = 0; i < sizeof(_mini12864SavedButtonRgb); i++)
    {
        _mini12864SavedButtonRgb[i] = 0;
    }

    #if (MINI12864_VARIANT == MINI12864_VARIANT_V3)
    if (mini12864RgbPinValid())
    {
        pinMode(MINI12864_V3_RGB_PIN, OUTPUT);
        #ifdef MINI12864_V3_RGB_ENABLED
        _mini12864RgbEnabled = (MINI12864_V3_RGB_ENABLED != 0);
        #else
        _mini12864RgbEnabled = true;
        #endif
        _mini12864RgbR = MINI12864_V3_RGB_R;
        _mini12864RgbG = MINI12864_V3_RGB_G;
        _mini12864RgbB = MINI12864_V3_RGB_B;
        writeMini12864RgbNow(_mini12864RgbEnabled ? _mini12864RgbR : 0, _mini12864RgbEnabled ? _mini12864RgbG : 0,
            _mini12864RgbEnabled ? _mini12864RgbB : 0);
    }
    #endif

    #if DISPLAY_TYPE != DISPLAY_TYPE_LCD_JOY_I2C_SSD1306 && DISPLAY_TYPE != DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567                             \
        && DISPLAY_TYPE != DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701 && DISPLAY_TYPE != DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920                      \
        && DISPLAY_TYPE != DISPLAY_TYPE_MINI12864_V2
    // Create special characters for degrees and arrows
    _lcd.createChar(_degrees, DegreesBitmap);
    _lcd.createChar(_minutes, MinutesBitmap);
    _lcd.createChar(_leftArrow, LeftArrowBitmap);
    _lcd.createChar(_rightArrow, RightArrowBitmap);
    _lcd.createChar(_upArrow, UpArrowBitmap);
    _lcd.createChar(_downArrow, DownArrowBitmap);
    _lcd.createChar(_tracking, TrackingBitmap);
    _lcd.createChar(_noTracking, NoTrackingBitmap);
    #endif
}

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD && defined(LCD_BRIGHTNESS_PIN)
/**
 * @brief Check to see if there is a problem in the LCD backlight circuit
 * @details The 'broken' designs connect D10 directly to the base of
 * an NPN transistor. This will cause a short when D10 is set to HIGH as there
 * is no current limiting resistor in the path between D10 to the base and the
 * emitter to ground.
 * Adapted from https://forum.arduino.cc/index.php?topic=96747.0
 * Link also notes a HW fix that can be applied to enable full brightness.
 * @returns true if the LCD was manufactured incorrectly, false otherwise
 */
bool LcdMenu::testIfLcdIsBad()
{
    /*
     * Set the pin to an input with pullup disabled, this should be safe on all shields.
     * The reason for the digitalWrite() first is that only the newer Arduino
     * cores disable the pullup when setting the pin to INPUT.
     * On boards that have a pullup on the transistor base,
     * this should cause the backlight to be on.
     */
    digitalWrite(LCD_BRIGHTNESS_PIN, LOW);
    pinMode(LCD_BRIGHTNESS_PIN, INPUT);

    /*
     * Since the pullup was turned off above by setting the pin to input mode,
     * it should drive the pin LOW which should be safe given the known design flaw.
     */
    pinMode(LCD_BRIGHTNESS_PIN, OUTPUT);

    /*
     * !!! WARNING !!!
     * This line is NOT safe thing to use on the broken designs!
     */
    digitalWrite(LCD_BRIGHTNESS_PIN, HIGH);

    // Now see if a short is pulling down the HIGH output.
    delayMicroseconds(5);  // Give some time for the signal to drop
    const int pinValue = digitalRead(LCD_BRIGHTNESS_PIN);

    // Restore the pin to a safe state: Input with pullup turned off
    digitalWrite(LCD_BRIGHTNESS_PIN, LOW);
    pinMode(LCD_BRIGHTNESS_PIN, INPUT);

    /*
     * If the level read back is not HIGH then there is a problem because the
     * pin is being driven HIGH by the AVR.
     */
    const bool lcdIsBad = (pinValue != HIGH);
    LOG(DEBUG_INFO, "[LCD]: HW is bad? %s", lcdIsBad ? "YES" : "NO");
    return lcdIsBad;
}
    #endif

// Find a menu item by its ID
MenuItem *LcdMenu::findById(byte id)
{
    for (byte i = 0; i < _numMenuItems; i++)
    {
        if (_menuItems[i]->id() == id)
        {
            return _menuItems[i];
        }
    }
    return NULL;
}

// Add a new menu item to the list (order matters)
void LcdMenu::addItem(const char *disp, byte id)
{
    _menuItems[_numMenuItems++] = new MenuItem(disp, id);
    _longestDisplay             = max((size_t) _longestDisplay, strlen(disp));
}

// Get the currently active item ID
byte LcdMenu::getActive()
{
    return _menuItems[_activeMenuIndex]->id();
}

// Set the active menu item
void LcdMenu::setActive(byte id)
{
    for (byte i = 0; i < _numMenuItems; i++)
    {
        if (_menuItems[i]->id() == id)
        {
            _activeMenuIndex = i;
            break;
        }
    }
}

// Pass thru utility function
void LcdMenu::setCursor(byte col, byte row)
{
    _activeRow = row;
    _activeCol = col;
}

// Pass thru utility function
void LcdMenu::clear()
{
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                         \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    _lcd.clearBuffer();
    _lcd.sendBuffer();
    #else
    _lcd.clear();
    #endif
}

// Set the brightness of the backlight
void LcdMenu::setBacklightBrightness(int level, bool persist)
{
    _brightness = level;
    _backlightEnabled = (_brightness > 0);
    if (_brightness > 0)
    {
        _lastNonZeroBrightness = _brightness;
    }

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD && defined(LCD_BRIGHTNESS_PIN)
    // Not supported on ESP32 due to lack of built-in analogWrite()
    if (_lcdBadHw)
    {
        // On 'bad' hardware you can only turn off or on
        if (_brightness > 0)
        {
            pinMode(LCD_BRIGHTNESS_PIN, INPUT);
        }
        else
        {
            pinMode(LCD_BRIGHTNESS_PIN, OUTPUT);
        }
    }
    else
    {
        analogWrite(LCD_BRIGHTNESS_PIN, _brightness);
    }
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23008 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23017
        // Nothing to do?
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_JOY_I2C_SSD1306
    _lcd.setContrast(_brightness);
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                       \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    _lcd.setContrast(_brightness);
    #if DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2 && (MINI12864_VARIANT == MINI12864_VARIANT_V2)
    if (mini12864V2BacklightPinValid())
    {
        pinMode(MINI12864_V2_BACKLIGHT_PIN, OUTPUT);
        #if MINI12864_V2_BACKLIGHT_ACTIVE_HIGH
        digitalWrite(MINI12864_V2_BACKLIGHT_PIN, _backlightEnabled ? HIGH : LOW);
        #else
        digitalWrite(MINI12864_V2_BACKLIGHT_PIN, _backlightEnabled ? LOW : HIGH);
        #endif
    }
    #endif
    #endif

    if (persist)
    {
        LOG(DEBUG_INFO, "[LCD]: Saving %d as brightness", _brightness);
        EEPROMStore::storeBrightness(_brightness);
    }
}

void LcdMenu::setBacklightEnabled(bool enabled, bool persist)
{
    if (enabled)
    {
        byte restoreLevel = (_lastNonZeroBrightness > 0) ? _lastNonZeroBrightness : 180;
        setBacklightBrightness(restoreLevel, persist);
    }
    else
    {
        setBacklightBrightness(0, persist);
    }
}

// Get the current brightness
int LcdMenu::getBacklightBrightness() const
{
    return _brightness;
}

bool LcdMenu::isBacklightEnabled() const
{
    return _backlightEnabled;
}

bool LcdMenu::isMini12864RgbSupported() const
{
    #if (MINI12864_VARIANT == MINI12864_VARIANT_V3)
    return mini12864RgbPinValid();
    #else
    return false;
    #endif
}

bool LcdMenu::isMini12864RgbEnabled() const
{
    return _mini12864RgbEnabled;
}

void LcdMenu::setMini12864RgbEnabled(bool enabled)
{
    _mini12864RgbEnabled = enabled;
    #if (MINI12864_VARIANT == MINI12864_VARIANT_V3)
    writeMini12864RgbNow(_mini12864RgbEnabled ? _mini12864RgbR : 0, _mini12864RgbEnabled ? _mini12864RgbG : 0,
        _mini12864RgbEnabled ? _mini12864RgbB : 0);
    #endif
}

void LcdMenu::getMini12864Rgb(uint8_t *r, uint8_t *g, uint8_t *b) const
{
    if (r != NULL)
    {
        *r = _mini12864RgbR;
    }
    if (g != NULL)
    {
        *g = _mini12864RgbG;
    }
    if (b != NULL)
    {
        *b = _mini12864RgbB;
    }
}

void LcdMenu::setMini12864Rgb(uint8_t r, uint8_t g, uint8_t b)
{
    _mini12864RgbR = r;
    _mini12864RgbG = g;
    _mini12864RgbB = b;
    #if (MINI12864_VARIANT == MINI12864_VARIANT_V3)
    for (uint8_t i = 0; i < MINI12864_V3_LED_COUNT; i++)
    {
        _mini12864LedRgb[i * 3 + 0] = r;
        _mini12864LedRgb[i * 3 + 1] = g;
        _mini12864LedRgb[i * 3 + 2] = b;
    }
    if (_mini12864RgbEnabled)
    {
        writeMini12864RgbNow(_mini12864RgbR, _mini12864RgbG, _mini12864RgbB);
    }
    #endif
}

bool LcdMenu::setMini12864RgbLed(uint8_t ledIndex1Based, uint8_t r, uint8_t g, uint8_t b)
{
    #if (MINI12864_VARIANT == MINI12864_VARIANT_V3)
    if (ledIndex1Based < 1 || ledIndex1Based > MINI12864_V3_LED_COUNT)
    {
        return false;
    }
    uint8_t idx = (uint8_t)(ledIndex1Based - 1);
    _mini12864LedRgb[idx * 3 + 0] = r;
    _mini12864LedRgb[idx * 3 + 1] = g;
    _mini12864LedRgb[idx * 3 + 2] = b;
    _mini12864RgbEnabled = true;
    writeMini12864RgbChain(_mini12864LedRgb);
    return true;
    #else
    (void) ledIndex1Based;
    (void) r;
    (void) g;
    (void) b;
    return false;
    #endif
}

uint8_t LcdMenu::getMini12864LedCount() const
{
    #if (MINI12864_VARIANT == MINI12864_VARIANT_V3)
    return (uint8_t) MINI12864_V3_LED_COUNT;
    #else
    return 0;
    #endif
}

// Per-LED preset definitions for the BTT Mini12864 V3 WS2812 chain.
// Layout (confirmed on hardware): LED1 = encoder RIGHT, LED2 = encoder LEFT, LED3 = LCD backlight.
// Table stored as [mode][led][rgb] with values in 0..255.
static const uint8_t MINI12864_PRESETS[LcdMenu::MINI12864_MODE_COUNT][3][3] = {
    // MINI12864_MODE_OFF
    {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    // MINI12864_MODE_NIGHT_ASTRO: dim red encoder + red LCD for night vision
    {{80, 0, 0}, {80, 0, 0}, {255, 0, 0}},
    // MINI12864_MODE_COMFORT: encoder off, LCD golden-orange
    {{0, 0, 0}, {0, 0, 0}, {255, 160, 20}},
    // MINI12864_MODE_DAY: soft blue encoder, white LCD
    {{0, 0, 50}, {0, 0, 50}, {255, 255, 255}},
    // MINI12864_MODE_DEFAULT: white on every LED
    {{255, 255, 255}, {255, 255, 255}, {255, 255, 255}},
};

bool LcdMenu::applyMini12864BacklightMode(uint8_t mode)
{
    #if (MINI12864_VARIANT == MINI12864_VARIANT_V3)
    if (mode >= (uint8_t) MINI12864_MODE_COUNT)
    {
        return false;
    }
    for (uint8_t led = 0; led < MINI12864_V3_LED_COUNT; led++)
    {
        uint8_t srcLed = (led < 3) ? led : 2;  // clamp if the chain is smaller than table
        _mini12864LedRgb[led * 3 + 0] = MINI12864_PRESETS[mode][srcLed][0];
        _mini12864LedRgb[led * 3 + 1] = MINI12864_PRESETS[mode][srcLed][1];
        _mini12864LedRgb[led * 3 + 2] = MINI12864_PRESETS[mode][srcLed][2];
    }
    _mini12864BacklightMode = mode;
    _mini12864RgbEnabled = (mode != (uint8_t) MINI12864_MODE_OFF);
    writeMini12864RgbChain(_mini12864LedRgb);
    LOG(DEBUG_INFO, "[LCD]: applyMini12864BacklightMode(%d) applied", (int) mode);
    return true;
    #else
    (void) mode;
    return false;
    #endif
}

uint8_t LcdMenu::getMini12864BacklightMode() const
{
    return _mini12864BacklightMode;
}

bool LcdMenu::setMini12864ButtonBlinkMode(uint8_t mode)
{
    if (mode > (uint8_t) MINI12864_BUTTON_BLINK_EDIT)
    {
        return false;
    }
    _mini12864ButtonBlinkMode = mode;
    return true;
}

uint8_t LcdMenu::getMini12864ButtonBlinkMode() const
{
    return _mini12864ButtonBlinkMode;
}

void LcdMenu::setMini12864EditBlinkActive(bool active)
{
    _mini12864EditBlinkActive = active;
}

bool LcdMenu::isMini12864EditBlinkActive() const
{
    return _mini12864EditBlinkActive;
}

void LcdMenu::tickMini12864Effects(unsigned long nowMs)
{
    #if (MINI12864_VARIANT == MINI12864_VARIANT_V3)
    if (!isMini12864RgbSupported())
    {
        return;
    }

    const uint8_t activeMode = _mini12864EditBlinkActive ? (uint8_t) MINI12864_BUTTON_BLINK_EDIT : _mini12864ButtonBlinkMode;
    if (activeMode == (uint8_t) MINI12864_BUTTON_BLINK_NONE)
    {
        if (_mini12864ButtonBlinkRunning)
        {
            for (uint8_t i = 0; i < 6; i++)
            {
                _mini12864LedRgb[i] = _mini12864SavedButtonRgb[i];
            }
            _mini12864ButtonBlinkRunning = false;
            writeMini12864RgbChain(_mini12864LedRgb);
        }
        return;
    }

    if (!_mini12864ButtonBlinkRunning)
    {
        for (uint8_t i = 0; i < 6; i++)
        {
            _mini12864SavedButtonRgb[i] = _mini12864LedRgb[i];
        }
        _mini12864ButtonBlinkRunning = true;
    }

    const bool phase = (((nowMs / 250UL) % 2UL) != 0UL);
    if (activeMode == (uint8_t) MINI12864_BUTTON_BLINK_WARNING)
    {
        // Warning mode: both encoder button LEDs alternate together blue <-> red.
        if (phase)
        {
            // Blue phase
            _mini12864LedRgb[0] = 0; _mini12864LedRgb[1] = 0; _mini12864LedRgb[2] = 255;
            _mini12864LedRgb[3] = 0; _mini12864LedRgb[4] = 0; _mini12864LedRgb[5] = 255;
        }
        else
        {
            // Red phase
            _mini12864LedRgb[0] = 255; _mini12864LedRgb[1] = 0; _mini12864LedRgb[2] = 0;
            _mini12864LedRgb[3] = 255; _mini12864LedRgb[4] = 0; _mini12864LedRgb[5] = 0;
        }
    }
    else
    {
        // Edit mode: encoder button LEDs blink red.
        uint8_t v = phase ? 255 : 0;
        _mini12864LedRgb[0] = v; _mini12864LedRgb[1] = 0; _mini12864LedRgb[2] = 0;
        _mini12864LedRgb[3] = v; _mini12864LedRgb[4] = 0; _mini12864LedRgb[5] = 0;
    }

    writeMini12864RgbChain(_mini12864LedRgb);
    #else
    (void) nowMs;
    #endif
}

void LcdMenu::setDisplayInverted(bool inverted)
{
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                       \
        || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    _lcd.sendF("c", inverted ? 0xA7 : 0xA6);
    LOG(DEBUG_INFO, "[LCD]: setDisplayInverted=%d", inverted ? 1 : 0);
    #else
    (void) inverted;
    #endif
}

void LcdMenu::setDisplayPower(bool on)
{
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                       \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    _lcd.setPowerSave(on ? 0 : 1);
    if (on)
    {
        _lcd.sendBuffer();
    }
    LOG(DEBUG_INFO, "[LCD]: setDisplayPower=%d", on ? 1 : 0);
    #else
    (void) on;
    #endif
}

void LcdMenu::runVisibilityTestPattern()
{
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                       \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    LOG(DEBUG_INFO, "[LCD]: runVisibilityTestPattern start");
    for (uint8_t i = 0; i < 3; i++)
    {
        _lcd.setPowerSave(0);
        _lcd.clearBuffer();
        _lcd.drawBox(0, 0, 128, 64);  // All black
        _lcd.sendBuffer();
        delay(500);
        _lcd.clearBuffer();
        _lcd.sendBuffer();  // All clear
        delay(500);
    }
    _lcd.clearBuffer();
    _lcd.drawFrame(0, 0, 128, 64);
    _lcd.setFont(u8g2_font_6x12_tf);
    _lcd.drawStr(6, 14, "LCD TEST OK");
    _lcd.drawStr(6, 30, "OpenAstroTracker");
    _lcd.drawStr(6, 46, ":XLT# passed");
    _lcd.sendBuffer();
    LOG(DEBUG_INFO, "[LCD]: runVisibilityTestPattern done");
    #else
    LOG(DEBUG_INFO, "[LCD]: runVisibilityTestPattern: display type not graphic, skipping");
    #endif
}

void LcdMenu::getBacklightBrightnessRange(int *minPtr, int *maxPtr) const
{
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD
    if (_lcdBadHw)
    {
        // Bad LCD displays are either on or off
        *minPtr = 0;
        *maxPtr = 1;
    }
    else
    #endif
    {
        // Full range otherwise
        *minPtr = 0;
        *maxPtr = 255;
    }
}

// Go to the previous menu item from currently active one (wraps around)
void LcdMenu::setPrevActive()
{
    _activeMenuIndex = adjustWrap(_activeMenuIndex, -1, 0, _numMenuItems - 1);

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                         \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    // Rotary page-by-page navigation mode: do not paint the top menu bar here.
    return;
    #endif

    // Update the display
    updateDisplay();

    // Clear submenu line, in case new menu doesn't print anything.
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                         \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    setCursor(0, 1);
    printMenu("");
    #else
    _lcd.setCursor(0, 1 * _charHeightRows);
    for (byte i = 0; i < _columns; i++)
    {
        _lcd.print(" ");
    }
    #endif
}

// Go to the next menu item from currently active one
void LcdMenu::setNextActive()
{
    _activeMenuIndex = adjustWrap(_activeMenuIndex, 1, 0, _numMenuItems - 1);

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                         \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    // Rotary page-by-page navigation mode: do not paint the top menu bar here.
    return;
    #endif

    // Update the display
    updateDisplay();

    // Clear submenu line, in case new menu doesn't print anything.
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                         \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    setCursor(0, 1);
    printMenu("");
    #else
    _lcd.setCursor(0, 1 * _charHeightRows);
    for (byte i = 0; i < _columns; i++)
    {
        _lcd.print(" ");
    }
    #endif
}

// Update the display of the LCD with the current menu settings
// This iterates over the menu items, building a menu string by concatenating their display string.
// It also places the selector arrows around the active one.
// It then sends the string to the LCD, keeping the selector arrows centered in the same place.
void LcdMenu::updateDisplay()
{
    static constexpr int MENU_BUF_LEN = 256;
    char bufMenu[MENU_BUF_LEN];
    char *pBufMenu      = &bufMenu[0];
    String menuString   = "";
    byte offsetToActive = 0;
    byte offset         = 0;

    char scratchBuffer[40];
    // Build the entire menu string
    for (byte i = 0; i < _numMenuItems; i++)
    {
        MenuItem *item = _menuItems[i];
        bool isActive  = i == _activeMenuIndex;
        snprintf(scratchBuffer, sizeof(scratchBuffer), "%c%s%c", isActive ? '>' : ' ', item->display(), isActive ? '<' : ' ');

        // For the active item remember where it starts in the string and insert selector arrows
        offsetToActive = isActive ? offset : offsetToActive;
        menuString += String(scratchBuffer);
        offset += strlen(scratchBuffer);
    }

    _lcd.setCursor(0, 0);
    _activeRow        = 0;
    _activeCol        = 0;
    int usableColumns = _columns - 1;  // Leave off last one to have distance to tracking indicator

    // Determine where to place the active menu item. (empty space around longest item divided by two).
    int margin           = (usableColumns - (_longestDisplay)) / 2;
    int offsetIntoString = offsetToActive - margin;

    // Pad the front if we don't have enough to offset the string to the arrow locations (happens on first item(s))
    while (offsetIntoString < 0)
    {
        *(pBufMenu++) = ' ';
        offsetIntoString++;
    }

    // Display the actual menu string
    while ((pBufMenu < bufMenu + usableColumns) && (offsetIntoString < (int) menuString.length()))
    {
        *(pBufMenu++) = menuString[offsetIntoString++];
    }

    // Pad the end with spaces so the display is cleared when getting to the last item(s).
    int columnsToRender = min((int) _columns, MENU_BUF_LEN - 1);
    while (pBufMenu < bufMenu + columnsToRender)
    {
        *(pBufMenu++) = ' ';
    }
    *(pBufMenu++) = 0;

    printMenu(String(bufMenu));

    setCursor(0, 1);
}

// Print the given character to the LCD, converting some special ones to our bitmaps
void LcdMenu::printChar(char ch)
{
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_JOY_I2C_SSD1306
    struct charData_t {
        const uint8_t *font;
        uint8_t encoding;
    };

    MappedDict<char, charData_t>::DictEntry_t lookupTable[] = {
        {'>', {.font = u8x8_font_open_iconic_arrow_1x1, .encoding = 64 + 14}},  // Right arrow
        {'<', {.font = u8x8_font_open_iconic_arrow_1x1, .encoding = 64 + 13}},  // Left arrow
        {'^', {.font = u8x8_font_open_iconic_arrow_1x1, .encoding = 64 + 15}},  // Up arrow
        {'~', {.font = u8x8_font_open_iconic_arrow_1x1, .encoding = 64 + 12}},  // Down arrow
        {'@', {.font = u8x8_font_7x14_1x2_f, .encoding = 176}},                 // Degrees
        {'&', {.font = u8x8_font_open_iconic_thing_1x1, .encoding = 64 + 15}},  // Tracking
        {'`', {.font = u8x8_font_open_iconic_thing_1x1, .encoding = 64 + 4}},   // Not tracking
    };
    auto buttonLookup      = MappedDict<char, charData_t>(lookupTable, ARRAY_SIZE(lookupTable));
    charData_t specialChar = {};
    const bool charInTable = buttonLookup.tryGet(ch, &specialChar);
    if (charInTable)
    {
        _lcd.setFont(specialChar.font);
        _lcd.drawGlyph(_lcd.tx, _lcd.ty, specialChar.encoding);
    }
    else
    {
        _lcd.setFont(u8x8_font_7x14_1x2_f);
        _lcd.drawGlyph(_lcd.tx, _lcd.ty, ch);
    }

    _lcd.tx += 1;
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                       \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    uint16_t x = (uint16_t) (_activeCol * 6);
    uint16_t y = (uint16_t) (_activeRow * 12 + 10);
    if (ch == '}')
    {
        // Draw a deterministic gear icon with primitives for better readability.
        uint16_t top = (uint16_t) (_activeRow * 12);
        uint16_t cx  = (uint16_t) (x + 5);
        uint16_t cy  = (uint16_t) (top + 6);
        _lcd.drawCircle(cx, cy, 3, U8G2_DRAW_ALL);
        _lcd.drawDisc(cx, cy, 1, U8G2_DRAW_ALL);
        _lcd.drawBox((uint16_t) (cx - 1), top, 2, 2);
        _lcd.drawBox((uint16_t) (cx - 1), (uint16_t) (top + 10), 2, 2);
        _lcd.drawBox((uint16_t) (x + 9), (uint16_t) (cy - 1), 2, 2);
        _lcd.drawBox(x, (uint16_t) (cy - 1), 2, 2);
        _activeCol += 2;
        return;
    }
    else
    {
        uint16_t codepoint = (uint8_t) ch;
        switch (ch)
        {
            case '@': codepoint = 176; break;  // degree symbol
            case '&': codepoint = '*'; break;  // tracking marker fallback
            case '`': codepoint = 'x'; break;  // no-tracking marker fallback
            case '~': codepoint = 'v'; break;  // down arrow fallback
            default: break;
        }
        _lcd.setFont(u8g2_font_6x12_tf);
        _lcd.drawGlyph(x, y, codepoint);
    }
    _activeCol++;
    #else
    MappedDict<char, specialChar_t>::DictEntry_t lookupTable[] = {
        {'>', _rightArrow},
        {'<', _leftArrow},
        {'^', _upArrow},
        {'~', _downArrow},
        {'@', _degrees},
        {'\'', _minutes},
        {'&', _tracking},
        {'`', _noTracking},
    };
    auto buttonLookup = MappedDict<char, specialChar_t>(lookupTable, ARRAY_SIZE(lookupTable));
    specialChar_t specialChar;
    const bool charInTable = buttonLookup.tryGet(ch, &specialChar);
    if (charInTable)
    {
        _lcd.write(specialChar);
    }
    else
    {
        _lcd.print(ch);
    }
    #endif
}

// Print a character at a specific position
void LcdMenu::printAt(int col, int row, char ch)
{
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                         \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    setCursor(col, row);
    #else
    _lcd.setCursor(col, _charHeightRows * row);
    #endif
    printChar(ch);
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                         \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    _lcd.sendBuffer();
    #endif
}

void LcdMenu::drawConfigLogoLarge()
{
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                         \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    // Reserve rows 1..4 (y=12..63) for the large icon while browsing top-level pages.
    _lcd.setDrawColor(0);
    _lcd.drawBox(0, 12, _columns * 6, 52);
    _lcd.setDrawColor(1);

    // Draw the embedded logo asset centered below the title row.
    const uint8_t logoX = 32;
    const uint8_t logoY = 20;
    _lcd.drawXBMP(logoX, logoY, config_gear_32_width, config_gear_32_height, config_gear_32_bits);

    _lcd.sendBuffer();
    #endif
}

void LcdMenu::clearConfigLogoLarge()
{
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                         \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    // Clear rows 1..4 where the large top-level Configuration logo is drawn.
    _lcd.setDrawColor(0);
    _lcd.drawBox(0, 12, _columns * 6, 52);
    _lcd.setDrawColor(1);
    _lcd.sendBuffer();
    #endif
}

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23008 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23017
uint8_t LcdMenu::readButtons()
{
    return _lcd.readButtons();
}
    #endif

// Print a string to the LCD at the current cursor position, substituting the special arrows and padding with spaces to the end
void LcdMenu::printMenu(String line)
{
    if (_activeRow >= MAX_DISPLAY_ROWS)
    {
        return;
    }

    if ((_lastDisplay[_activeRow] != line) || (_activeCol != 0))
    {
        _lastDisplay[_activeRow] = line;

        int spaces = _columns - line.length();
        #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                     \
            || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
        int pixelY = _activeRow * 12;
        _lcd.setDrawColor(0);
        _lcd.drawBox(0, pixelY, _columns * 6, 12);
        _lcd.setDrawColor(1);
        for (char i : line)
        {
            printChar(i);
        }
        while (spaces > 0)
        {
            printChar(' ');
            spaces--;
        }
        _lcd.sendBuffer();
        #else
        _lcd.setCursor(_activeCol, _charHeightRows * _activeRow);
        for (char i : line)
        {
            printChar(i);
        }

        // Clear the rest of the display
        while (spaces > 0)
        {
            _lcd.print(" ");
            spaces--;
        }
        #endif
    }
}

    #if DISPLAY_TYPE != DISPLAY_TYPE_LCD_JOY_I2C_SSD1306 && DISPLAY_TYPE != DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567                             \
        && DISPLAY_TYPE != DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701 && DISPLAY_TYPE != DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920                      \
        && DISPLAY_TYPE != DISPLAY_TYPE_MINI12864_V2

// The right arrow bitmap
byte LcdMenu::RightArrowBitmap[8] = {B00000, B01000, B01100, B01110, B01100, B01000, B00000, B00000};

// The left arrow bitmap
byte LcdMenu::LeftArrowBitmap[8] = {B00000, B00010, B00110, B01110, B00110, B00010, B00000, B00000};

byte LcdMenu::UpArrowBitmap[8] = {B00100, B01110, B11111, B00100, B00100, B00100, B00100, B00100};

byte LcdMenu::DownArrowBitmap[8] = {B000100, B000100, B000100, B000100, B000100, B011111, B001110, B000100};

byte LcdMenu::DegreesBitmap[8] = {B01100, B10010, B10010, B01100, B00000, B00000, B00000, B00000};

byte LcdMenu::MinutesBitmap[8] = {B01000, B01000, B01000, B00000, B00000, B00000, B00000, B00000};

byte LcdMenu::TrackingBitmap[8] = {B10111, B00010, B10010, B00010, B10111, B00101, B10110, B00101};

byte LcdMenu::NoTrackingBitmap[8] = {B10000, B00000, B10000, B00010, B10000, B00000, B10000, B00000};

    #endif

#else  // Headless (i.e. DISPLAY_TYPE == 0)

LcdMenu::LcdMenu(byte cols, byte rows, int maxItems)
{
}

MenuItem *LcdMenu::findById(byte id)
{
    return NULL;
}

void LcdMenu::addItem(const char *disp, byte id)
{
}

byte LcdMenu::getActive()
{
    return 0;
}

void LcdMenu::setActive(byte id)
{
}

void LcdMenu::setCursor(byte col, byte row)
{
}

void LcdMenu::clear()
{
}

void LcdMenu::setNextActive()
{
}

void LcdMenu::setPrevActive()
{
}

void LcdMenu::updateDisplay()
{
}

void LcdMenu::setBacklightEnabled(bool enabled, bool persist)
{
}

bool LcdMenu::isBacklightEnabled() const
{
    return false;
}

bool LcdMenu::isMini12864RgbSupported() const
{
    return false;
}

bool LcdMenu::isMini12864RgbEnabled() const
{
    return false;
}

void LcdMenu::setMini12864RgbEnabled(bool enabled)
{
}

void LcdMenu::getMini12864Rgb(uint8_t *r, uint8_t *g, uint8_t *b) const
{
}

void LcdMenu::setMini12864Rgb(uint8_t r, uint8_t g, uint8_t b)
{
}

bool LcdMenu::setMini12864RgbLed(uint8_t ledIndex1Based, uint8_t r, uint8_t g, uint8_t b)
{
    (void) ledIndex1Based;
    (void) r;
    (void) g;
    (void) b;
    return false;
}

uint8_t LcdMenu::getMini12864LedCount() const
{
    return 0;
}

bool LcdMenu::applyMini12864BacklightMode(uint8_t mode)
{
    (void) mode;
    return false;
}

uint8_t LcdMenu::getMini12864BacklightMode() const
{
    return 0;
}

void LcdMenu::printMenu(String line)
{
}

void LcdMenu::printChar(char ch)
{
}

void LcdMenu::printAt(int col, int row, char ch)
{
}

void LcdMenu::drawConfigLogoLarge()
{
}

void LcdMenu::clearConfigLogoLarge()
{
}

#endif
