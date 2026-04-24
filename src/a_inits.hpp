#pragma once

#include "../Configuration.hpp"
#include "inc/Globals.hpp"

#ifndef NEW_STEPPER_LIB
PUSH_NO_WARNINGS
    #include <AccelStepper.h>
POP_NO_WARNINGS
#endif

#include "Utility.hpp"
#include "DayTime.hpp"
#include "Mount.hpp"
#include "MeadeCommandProcessor.hpp"

// TODO: we have to change driver type to DRIVER_TYPE_TMC2209 and add a new definition for the actual mode (e.g. DRIVER_MODE_UART)
#if (RA_DRIVER_TYPE == DRIVER_TYPE_TMC2209_STANDALONE) || (RA_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART)                                     \
    || (DEC_DRIVER_TYPE == DRIVER_TYPE_TMC2209_STANDALONE) || (DEC_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART)                                \
    || (AZ_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART) || (ALT_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART)                                       \
    || (FOCUS_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART)
PUSH_NO_WARNINGS
    #include <TMCStepper.h>
POP_NO_WARNINGS
#endif

#if USE_GPS == 1
PUSH_NO_WARNINGS
    #if USE_GPS_I2C_ADAFRUIT == 1
        #include <Wire.h>
    #else
    #if defined(GPS_USE_SOFTWARE_SERIAL) && (GPS_USE_SOFTWARE_SERIAL == 1)
        #include <SoftwareSerial.h>
    #endif
    #endif
    #include <TinyGPS++.h>
POP_NO_WARNINGS

    #if USE_GPS_I2C_ADAFRUIT == 0
    #if defined(GPS_USE_SOFTWARE_SERIAL) && (GPS_USE_SOFTWARE_SERIAL == 1)
SoftwareSerial gpsSerial(GPS_SERIAL_RX_PIN, GPS_SERIAL_TX_PIN);  // RX, TX
    #endif
    #endif
TinyGPSPlus gps;
#endif

////////////////////////////////////
// Stepper definitions /////////////
#define RAmotorPin1 RA_STEP_PIN
#define RAmotorPin2 RA_DIR_PIN

// DEC Motor pins
#define DECmotorPin1 DEC_STEP_PIN
#define DECmotorPin2 DEC_DIR_PIN

// AZ Motor pins
#if (AZ_STEPPER_TYPE != STEPPER_TYPE_NONE)
    #define AZmotorPin1 AZ_STEP_PIN
    #define AZmotorPin2 AZ_DIR_PIN
#endif

// ALT Motor pins
#if (ALT_STEPPER_TYPE != STEPPER_TYPE_NONE)
    #define ALTmotorPin1 ALT_STEP_PIN
    #define ALTmotorPin2 ALT_DIR_PIN
#endif

// Focus Motor pins
#if (FOCUS_STEPPER_TYPE != STEPPER_TYPE_NONE)
    #define FOCUSmotorPin1 FOCUS_STEP_PIN
    #define FOCUSmotorPin2 FOCUS_DIR_PIN
#endif

// End Stepper Definitions //////////////
/////////////////////////////////////////

/////////////////////////////////////////
// Driver definitions ///////////////////
#if (RA_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART) || (DEC_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART)                                          \
    || (AZ_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART) || (ALT_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART)                                       \
    || (FOCUS_DRIVER_TYPE == DRIVER_TYPE_TMC2209_UART)
    #define R_SENSE 0.11f  // 0.11 for StepStick
#endif
// End Driver Definitions ///////////////
/////////////////////////////////////////

// Menu IDs
#define RA_Menu          1
#define DEC_Menu         2
#define HA_Menu          3
#define Heat_Menu        4
#define Calibration_Menu 5
#define Focuser_Menu     6
#define Control_Menu     7
#define Home_Menu        8
#define POI_Menu         9
#define Status_Menu      10
#define Config_Menu      11

// How many menu items at most?
#define MAXMENUITEMS 12

// Mini12864 / graphic displays use a rotary encoder: navigation is rotate to cycle,
// click to enter/edit. Keypad displays keep their original directional-key workflow.
#if DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7567 || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_UC1701                            \
    || DISPLAY_TYPE == DISPLAY_TYPE_LCD_GRAPHIC_U8G2_ST7920 || DISPLAY_TYPE == DISPLAY_TYPE_MINI12864_V2
    #define USES_ROTARY_ENCODER 1
#else
    #define USES_ROTARY_ENCODER 0
#endif

// When true, the top-level menu bar is in navigation mode: rotating cycles between
// top-level menus, clicking enters the active menu. Set to false upon entering a menu,
// set back to true when a sub-menu's "Exit" item is activated.
bool topLevelMenuNav = (USES_ROTARY_ENCODER == 1);

// Sub-menus set this to true to request a return to the top-level carousel.
// c_buttons.hpp's loop inspects and clears it after each menu dispatch.
bool requestBackToTop = false;

#if SUPPORT_GUIDED_STARTUP == 1
bool inStartup = true;  // Start with a guided startup
#else
bool inStartup = false;  // Start with a guided startup
#endif

// Serial control variables
bool okToUpdateMenu                = true;   // Can be used to supress rendering the first line of the menu.
bool quitSerialOnNextButtonRelease = false;  // Used to detect SELECT button to quit Serial mode.
uint8_t rotaryMolSens              = 5;      // Default 5 (neutral: 1 cran = 1 event); overwritten from EEPROM in setup().

// Language support
#include "EPROMStore.hpp"
#include "translations.hpp"
Language currentLanguage = EEPROMStore::getLanguage();  // Load from EEPROM

// RA variables
int RAselect;

// DEC variables
int DECselect;

// HA variables
int HAselect;
