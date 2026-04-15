#ifndef LCDBUTTONS_HPP_
#define LCDBUTTONS_HPP_

#include "inc/Globals.hpp"

// LCD shield buttons
enum lcdButton_t
{
    btnRIGHT,
    btnUP,
    btnDOWN,
    btnLEFT,
    btnSELECT,
    btnNONE,
    btnINVALID,
};

// Forward declaration
class LcdMenu;

class LcdButtons
{
  public:
    LcdButtons(byte pin, LcdMenu *lcdMenu);
    LcdButtons(LcdMenu *lcdMenu);

    lcdButton_t currentKey()
    {
        checkKey();
        return _newKey;
    }

    lcdButton_t currentState()
    {
        checkKey();
        return _currentKey;
    }

    int currentAnalogState();

    bool keyChanged(lcdButton_t *pNewKey);

  private:
    void checkKey();

  private:
    unsigned long _lastKeyChange;
    unsigned long _encoderEdgeMs;
    unsigned long _encoderBtnChangeMs;
    byte _analogPin;
    uint8_t _encoderPrevAB;
    int8_t _encoderAcc;
    bool _encoderRawA;
    bool _encoderRawB;
    bool _encoderStableA;
    bool _encoderStableB;
    bool _encoderBtnRaw;
    bool _encoderBtnStable;
    lcdButton_t _lastKey;
    lcdButton_t _newKey;
    lcdButton_t _lastNewKey;
    lcdButton_t _currentKey;
    LcdMenu *_lcdMenu;
};

#endif  // LCDBUTTONS_HPP_