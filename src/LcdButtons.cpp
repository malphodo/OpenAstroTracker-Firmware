#include "inc/Globals.hpp"
#include "../Configuration.hpp"
#include "LcdMenu.hpp"
#include "LcdButtons.hpp"

#if DISPLAY_TYPE != DISPLAY_TYPE_NONE

LcdButtons::LcdButtons(byte pin, LcdMenu *lcdMenu)
{
    _lcdMenu       = lcdMenu;
    _analogPin     = pin;
    _lastKeyChange = 0;
    _encoderEdgeMs = 0;
    _encoderBtnChangeMs = 0;

    _newKey     = btnNONE;
    _lastNewKey = btnNONE;

    _currentKey = btnNONE;
    _lastKey    = btnNONE;
    _encoderPrevAB = 0;
    _encoderAcc = 0;
    _encoderRawA = false;
    _encoderRawB = false;
    _encoderStableA = false;
    _encoderStableB = false;
    _encoderBtnRaw = false;
    _encoderBtnStable = false;

    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_JOY_I2C_SSD1306
    // Initialize keypad
    pinMode(LCD_KEY_SENSE_X_PIN, INPUT);
    pinMode(LCD_KEY_SENSE_Y_PIN, INPUT);
    pinMode(LCD_KEY_SENSE_PUSH_PIN, INPUT);
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                      \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    pinMode(LCD12864_ENCODER_A_PIN, INPUT_PULLUP);
    pinMode(LCD12864_ENCODER_B_PIN, INPUT_PULLUP);
    pinMode(LCD12864_ENCODER_BTN_PIN, INPUT_PULLUP);
    _encoderRawA = (digitalRead(LCD12864_ENCODER_A_PIN) == LOW);
    _encoderRawB = (digitalRead(LCD12864_ENCODER_B_PIN) == LOW);
    _encoderStableA = _encoderRawA;
    _encoderStableB = _encoderRawB;
    _encoderPrevAB = ((_encoderStableA ? 1 : 0) << 1) | (_encoderStableB ? 1 : 0);
    _encoderBtnRaw = (digitalRead(LCD12864_ENCODER_BTN_PIN) == LOW);
    _encoderBtnStable = _encoderBtnRaw;
    #endif
}

LcdButtons::LcdButtons(LcdMenu *lcdMenu)
{
    _lcdMenu       = lcdMenu;
    _lastKeyChange = 0;
    _encoderEdgeMs = 0;
    _encoderBtnChangeMs = 0;

    _newKey     = btnNONE;
    _lastNewKey = btnINVALID;

    _currentKey = btnNONE;
    _lastKey    = btnINVALID;
    _encoderPrevAB = 0;
    _encoderAcc = 0;
    _encoderRawA = false;
    _encoderRawB = false;
    _encoderStableA = false;
    _encoderStableB = false;
    _encoderBtnRaw = false;
    _encoderBtnStable = false;
}

bool LcdButtons::keyChanged(lcdButton_t *pNewKey)
{
    checkKey();
    if (_newKey != _lastNewKey)
    {
        *pNewKey    = _newKey;
        _lastNewKey = _newKey;
        return true;
    }
    return false;
}

int LcdButtons::currentAnalogState()
{
    #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23008 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23017
    return 0;  // No analog value for these displays
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_JOY_I2C_SSD1306
    return analogRead(LCD_KEY_SENSE_Y_PIN);
    #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                      \
        || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    return 0;
    #else
    return analogRead(_analogPin);
    #endif
}

void LcdButtons::checkKey()
{
    #if DISPLAY_TYPE > 0
        #if DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23008 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_KEYPAD_I2C_MCP23017
    uint8_t buttons = _lcdMenu->readButtons();
    _currentKey     = btnNONE;
    if (buttons)
    {
        if (buttons & BUTTON_UP)
            _currentKey = btnUP;
        if (buttons & BUTTON_DOWN)
            _currentKey = btnDOWN;
        if (buttons & BUTTON_LEFT)
            _currentKey = btnLEFT;
        if (buttons & BUTTON_RIGHT)
            _currentKey = btnRIGHT;
        if (buttons & BUTTON_SELECT)
            _currentKey = btnSELECT;
    }
        #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_JOY_I2C_SSD1306
    uint16_t x(analogRead(LCD_KEY_SENSE_X_PIN));
    uint16_t y(analogRead(LCD_KEY_SENSE_Y_PIN));
    uint16_t push(analogRead(LCD_KEY_SENSE_PUSH_PIN));

    // Assumes analogReadResolution(12) (the default)
    int16_t const MIDSCALE = 4096 / 2;
    int16_t const DEADBAND = 500;

    _currentKey = btnNONE;
    if (x > (MIDSCALE + DEADBAND))
        _currentKey = btnRIGHT;
    if (x < (MIDSCALE - DEADBAND))
        _currentKey = btnLEFT;
    if (y > (MIDSCALE + DEADBAND))
        _currentKey = btnDOWN;  // Y appears reversed
    if (y < (MIDSCALE - DEADBAND))
        _currentKey = btnUP;
    if (push < MIDSCALE)
        _currentKey = btnSELECT;  // Active low
        #elif DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                  \
            || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    static const unsigned long ENCODER_DEBOUNCE_MS = 2;
    static const unsigned long BUTTON_DEBOUNCE_MS = 20;
    static const int8_t QUADRATURE_TABLE[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

    const unsigned long now = millis();
    const bool rawA = (digitalRead(LCD12864_ENCODER_A_PIN) == LOW);
    const bool rawB = (digitalRead(LCD12864_ENCODER_B_PIN) == LOW);
    const bool rawBtn = (digitalRead(LCD12864_ENCODER_BTN_PIN) == LOW);

    if ((rawA != _encoderRawA) || (rawB != _encoderRawB))
    {
        _encoderRawA = rawA;
        _encoderRawB = rawB;
        _encoderEdgeMs = now;
    }
    if ((now - _encoderEdgeMs) >= ENCODER_DEBOUNCE_MS)
    {
        _encoderStableA = _encoderRawA;
        _encoderStableB = _encoderRawB;
    }

    if (rawBtn != _encoderBtnRaw)
    {
        _encoderBtnRaw = rawBtn;
        _encoderBtnChangeMs = now;
    }
    if ((now - _encoderBtnChangeMs) >= BUTTON_DEBOUNCE_MS)
    {
        _encoderBtnStable = _encoderBtnRaw;
    }

    lcdButton_t encoderEvent = btnNONE;
    const uint8_t ab = ((_encoderStableA ? 1 : 0) << 1) | (_encoderStableB ? 1 : 0);
    const uint8_t transition = (_encoderPrevAB << 2) | ab;
    const int8_t step = QUADRATURE_TABLE[transition];
    if (step != 0)
    {
        _encoderAcc += step;
        if (_encoderAcc >= 4)
        {
            encoderEvent = btnRIGHT;
            _encoderAcc = 0;
        }
        else if (_encoderAcc <= -4)
        {
            encoderEvent = btnLEFT;
            _encoderAcc = 0;
        }
    }
    _encoderPrevAB = ab;

    _currentKey = btnNONE;
    if (_encoderBtnStable)
        _currentKey = btnSELECT;
    else if (encoderEvent != btnNONE)
        _currentKey = encoderEvent;
        #else
    const int analogKeyValue = currentAnalogState();
    if (analogKeyValue > 1000)
        _currentKey = btnNONE;
    else if (analogKeyValue < 50)
        _currentKey = btnRIGHT;
    else if (analogKeyValue < 240)
        _currentKey = btnUP;
    else if (analogKeyValue < 400)
        _currentKey = btnDOWN;
    else if (analogKeyValue < 600)
        _currentKey = btnLEFT;
    else if (analogKeyValue < 920)
        _currentKey = btnSELECT;
        #endif

    if (_currentKey != _lastKey)
    {
        _lastKey       = _currentKey;
        _lastKeyChange = millis();
    }
    else
    {
        // If the keys haven't changed in 5ms, commit the change to the new keys.
        if (millis() - _lastKeyChange > 5)
        {
            _newKey = _currentKey;
        }
    }
    #endif
}

#else

// Null implementation
LcdButtons::LcdButtons(byte pin, LcdMenu *lcdMenu)
    : _lastKeyChange(0), _analogPin(pin), _lastKey(btnNONE), _newKey(btnNONE), _lastNewKey(btnNONE), _currentKey(btnNONE), _lcdMenu(lcdMenu)
{
}

LcdButtons::LcdButtons(LcdMenu *lcdMenu)
    : _lastKeyChange(0), _analogPin(0), _lastKey(btnNONE), _newKey(btnNONE), _lastNewKey(btnNONE), _currentKey(btnNONE), _lcdMenu(lcdMenu)
{
}

bool LcdButtons::keyChanged(lcdButton_t *pNewKey)
{
    return false;
}

void LcdButtons::checkKey()
{
}

#endif