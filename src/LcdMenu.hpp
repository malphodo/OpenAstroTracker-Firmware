#ifndef _LCDMENU_HPP_
#define _LCDMENU_HPP_

#if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD
    #include <LiquidCrystal.h>
#elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23008 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23017
    #include <LiquidTWI2.h>
#elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_JOY_I2C_SSD1306 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567                              \
    || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920                         \
    || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    #include <U8x8lib.h>  // https://github.com/olikraus/u8g2
#endif

#if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                            \
    || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    #include <U8g2lib.h>
#endif

// A single menu item (like RA, HEAT, POL, etc.)
// The ID is just a number, it has no relevance for the order of the items
// The display is what is shown on the menu.
class MenuItem
{
    const char *_display;  // What to display on the screen
    byte _id;              // The ID of the menu item
  public:
    MenuItem(const char *display, byte id)
    {
        _display = display;
        _id      = id;
    }

    const char *display()
    {
        return _display;
    }

    byte id()
    {
        return _id;
    }
};

// Class that drives the LCD screen with a menu
// You add a string and an id item and this class handles the display and navigation
class LcdMenu
{
  public:
    // Create a new menu, using the given number of LCD display columns and rows
    LcdMenu(byte cols, byte rows, int maxItems);

    void startup();

#if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD && defined(LCD_BRIGHTNESS_PIN)
    // Function to test LCD hardware, some units are shipped with defects
    static bool testIfLcdIsBad();
#endif

    // Find a menu item by its ID
    MenuItem *findById(byte id);

    // Add a new menu item to the list (order matters)
    void addItem(const char *disp, byte id);

    // Get the currently active item ID
    byte getActive();

    // Set the active menu item
    void setActive(byte id);

    // Pass thru utility function
    void setCursor(byte col, byte row);

    // Set and get the brightness of the backlight
    void setBacklightBrightness(int level, bool persist = true);
    void setBacklightEnabled(bool enabled, bool persist = true);
    int getBacklightBrightness() const;
    bool isBacklightEnabled() const;
    void getBacklightBrightnessRange(int *minPtr, int *maxPtr) const;
    bool isMini12864RgbSupported() const;
    bool isMini12864RgbEnabled() const;
    void setMini12864RgbEnabled(bool enabled);
    void getMini12864Rgb(uint8_t *r, uint8_t *g, uint8_t *b) const;
    void setMini12864Rgb(uint8_t r, uint8_t g, uint8_t b);
    // Per-LED control (1-based index, 1..MINI12864_V3_LED_COUNT).
    // Returns true if the LED index is valid and the color was applied.
    bool setMini12864RgbLed(uint8_t ledIndex1Based, uint8_t r, uint8_t g, uint8_t b);
    uint8_t getMini12864LedCount() const;

    // Preset backlight modes for the Mini12864 V3 WS2812 chain. Selectable at runtime via
    // the Meade extension command :XSMn#. Presets are volatile (not persisted to EEPROM).
    enum Mini12864BacklightMode : uint8_t
    {
        MINI12864_MODE_OFF         = 0,  // All LEDs off
        MINI12864_MODE_NIGHT_ASTRO = 1,  // Red LCD + dim red encoder (night vision)
        MINI12864_MODE_COMFORT     = 2,  // Golden-orange LCD, encoder off
        MINI12864_MODE_DAY         = 3,  // White LCD, soft blue encoder
        MINI12864_MODE_DEFAULT     = 4,  // White on every LED (boot default)
        MINI12864_MODE_COUNT       = 5
    };
    bool applyMini12864BacklightMode(uint8_t mode);
    uint8_t getMini12864BacklightMode() const;

    // Visibility self-test pattern (graphic displays only). Cycles buffer clear,
    // full black fill and invert a few times so a working Mini12864 panel produces
    // clearly visible transitions. Useful to confirm SPI/controller wiring.
    void runVisibilityTestPattern();

    // Toggle controller-level pixel polarity (ST7565/ST7567/UC1701 A6h/A7h).
    void setDisplayInverted(bool inverted);

    // Pass thru utility function
    void clear();

    // Go to the next menu item from currently active one
    void setNextActive();

    // Update the display of the LCD with the current menu settings
    // This iterates over the menu items, building a menu string by concatenating their display string.
    // It also places the selector arrows around the active one.
    // It then sends the string to the LCD, keeping the selector arrows centered in the same place.
    void updateDisplay();

    // Print a string to the LCD at the current cursor position, substituting the special arrows and padding with spaces to the end
    void printMenu(String line);

    // Print a character at a specific position
    void printAt(int col, int row, char ch);

#if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23008 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23017
    uint8_t readButtons();
#endif

  private:
    // Print a single character at the current cursor location and advance cursor by one. Substitutes special chars.
    void printChar(char ch);

  private:
#if DISPLAY_TYPE > 0

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD
    LiquidCrystal _lcd;  // The LCD screen that we'll display the menu on
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23008 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23017
    LiquidTWI2 _lcd;  // The LCD screen that we'll display the menu on
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_JOY_I2C_SSD1306
    U8X8_SSD1306_128X32_UNIVISION_HW_I2C _lcd;
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567
    U8G2_ST7567_ENH_DG128064I_F_4W_SW_SPI _lcd;
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701
    U8G2_UC1701_MINI12864_F_4W_SW_SPI _lcd;
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920
    U8G2_ST7920_128X64_F_SW_SPI _lcd;
    #elif DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
        #if MINI12864_CONTROLLER == MINI12864_CONTROLLER_UC1701
    U8G2_UC1701_MINI12864_F_4W_SW_SPI _lcd;
        #elif MINI12864_CONTROLLER == MINI12864_CONTROLLER_ST7565
    U8G2_ST7565_ERC12864_F_4W_SW_SPI _lcd;
        #else
    U8G2_ST7567_ENH_DG128064I_F_4W_SW_SPI _lcd;
        #endif
    #endif

    byte const _cols;
    byte const _rows;
    byte const _maxItems;
    byte const _charHeightRows;  // Height of character in display native rows
    bool _lcdBadHw;

    MenuItem **_menuItems;  // The first menu item (linked list)
    byte _numMenuItems;
    byte _activeMenuIndex;
    byte _longestDisplay;    // The number of characters in the longest menu item
    byte _columns;           // The number of columns in the LCD display
    byte _activeRow;         // The row that the LCD cursor is on
    byte _activeCol;         // The column that the LCD cursor is on
    static constexpr byte MAX_DISPLAY_ROWS = 8;
    String _lastDisplay[MAX_DISPLAY_ROWS];  // The last string that was displayed on each row
    byte _brightness;
    bool _backlightEnabled;
    byte _lastNonZeroBrightness;
    bool _mini12864RgbEnabled;
    uint8_t _mini12864RgbR;
    uint8_t _mini12864RgbG;
    uint8_t _mini12864RgbB;
    // Per-LED color buffer for the WS2812 chain (R,G,B x N LEDs).
    // Sized for the BTT Mini12864 V3 hardware (3 LEDs). Indices are 0-based here.
    uint8_t _mini12864LedRgb[9];
    uint8_t _mini12864BacklightMode;

    #if DISPLAY_TYPE != DISPLAY_TYPE_LCD_JOY_I2C_SSD1306 && DISPLAY_TYPE != DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567                             \
        && DISPLAY_TYPE != DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701 && DISPLAY_TYPE != DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920                      \
        && DISPLAY_TYPE != DISPLAY_TYPE_MINI12864_V2
    enum specialChar_t : byte
    {
        _degrees,
        _minutes,
        _leftArrow,
        _rightArrow,
        _upArrow,
        _downArrow,
        _tracking,
        _noTracking,
        SPECIAL_CHAR_MAX,
    };
    static_assert(SPECIAL_CHAR_MAX <= 8, "LCD only supports a maximum of 8 special characters");

    // The special character bitmaps
    static byte RightArrowBitmap[8];
    static byte LeftArrowBitmap[8];
    static byte UpArrowBitmap[8];
    static byte DownArrowBitmap[8];
    static byte DegreesBitmap[8];
    static byte MinutesBitmap[8];
    static byte TrackingBitmap[8];
    static byte NoTrackingBitmap[8];
    #endif

#endif
};

#endif
