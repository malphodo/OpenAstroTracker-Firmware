/**
 * @brief Pins configuration for BigTreeTech SKR 1.4 Turbo (LPC1769) based OAT.
 *
 * This mapping follows the SKR 1.4 axis socket naming and uses HardwareSerial
 * for TMC2209 UART, with one UART per axis by default.
 */

#pragma once

// TMC2209 UART lines on SKR 1.4 are P1.10 (X) and P1.09 (Y) per BTT/Marlin — not the default Serial1/Serial2 pins
// of the LPC1769 Arduino core (UART1=P0.15/16, UART2=P0.10/11). Use SoftwareSerial on the CS/UART pins like Marlin.
#define SW_SERIAL_UART 1

#ifndef UART_CONNECTION_TEST_TXRX
    #define UART_CONNECTION_TEST_TXRX 1
#endif

// X socket -> RA axis
#ifndef RA_STEP_PIN
    #define RA_STEP_PIN P2_02
#endif
#ifndef RA_DIR_PIN
    #define RA_DIR_PIN P2_06
#endif
#ifndef RA_EN_PIN
    #define RA_EN_PIN P2_01
#endif
#ifndef RA_DIAG_PIN
    #define RA_DIAG_PIN P1_29
#endif
// Single-wire UART to TMC PDN: RX and TX are the same physical pin (half-duplex).
#ifndef RA_SERIAL_PORT_RX
    #define RA_SERIAL_PORT_RX P1_10
#endif
#ifndef RA_SERIAL_PORT_TX
    #define RA_SERIAL_PORT_TX P1_10
#endif
#ifndef RA_DRIVER_ADDRESS
    #define RA_DRIVER_ADDRESS 0b00
#endif

// Y socket -> DEC axis
#ifndef DEC_STEP_PIN
    #define DEC_STEP_PIN P0_19
#endif
#ifndef DEC_DIR_PIN
    #define DEC_DIR_PIN P0_20
#endif
#ifndef DEC_EN_PIN
    #define DEC_EN_PIN P2_08
#endif
#ifndef DEC_DIAG_PIN
    #define DEC_DIAG_PIN P1_28
#endif
#ifndef DEC_SERIAL_PORT_RX
    #define DEC_SERIAL_PORT_RX P1_09
#endif
#ifndef DEC_SERIAL_PORT_TX
    #define DEC_SERIAL_PORT_TX P1_09
#endif
#ifndef DEC_DRIVER_ADDRESS
    #define DEC_DRIVER_ADDRESS 0b00
#endif

// Z socket -> AZ axis
#ifndef AZ_STEP_PIN
    #define AZ_STEP_PIN P0_22
#endif
#ifndef AZ_DIR_PIN
    #define AZ_DIR_PIN P2_11
#endif
#ifndef AZ_EN_PIN
    #define AZ_EN_PIN P0_21
#endif
#ifndef AZ_DIAG_PIN
    #define AZ_DIAG_PIN P1_27
#endif
#ifndef AZ_SERIAL_PORT
    #define AZ_SERIAL_PORT Serial3
#endif
#ifndef AZ_DRIVER_ADDRESS
    #define AZ_DRIVER_ADDRESS 0b00
#endif

// E0 socket -> ALT axis
#ifndef ALT_STEP_PIN
    #define ALT_STEP_PIN P2_13
#endif
#ifndef ALT_DIR_PIN
    #define ALT_DIR_PIN P0_11
#endif
#ifndef ALT_EN_PIN
    #define ALT_EN_PIN P2_12
#endif
#ifndef ALT_DIAG_PIN
    #define ALT_DIAG_PIN P1_26
#endif
#ifndef ALT_SERIAL_PORT
    #define ALT_SERIAL_PORT Serial1
#endif
#ifndef ALT_DRIVER_ADDRESS
    #define ALT_DRIVER_ADDRESS 0b00
#endif

// E1 socket -> FOCUS axis
#ifndef FOCUS_STEP_PIN
    #define FOCUS_STEP_PIN P1_15
#endif
#ifndef FOCUS_DIR_PIN
    #define FOCUS_DIR_PIN P1_14
#endif
#ifndef FOCUS_EN_PIN
    #define FOCUS_EN_PIN P1_16
#endif
#ifndef FOCUS_DIAG_PIN
    #define FOCUS_DIAG_PIN P1_25
#endif
#ifndef FOCUS_SERIAL_PORT
    #define FOCUS_SERIAL_PORT Serial2
#endif
#ifndef FOCUS_DRIVER_ADDRESS
    #define FOCUS_DRIVER_ADDRESS 0b00
#endif

// Homing sensors
#ifndef RA_HOMING_SENSOR_PIN
    #define RA_HOMING_SENSOR_PIN P1_24
#endif
#ifndef DEC_HOMING_SENSOR_PIN
    #define DEC_HOMING_SENSOR_PIN P1_23
#endif

// End switch sensors (optional)
#ifndef RA_ENDSWITCH_EAST_SENSOR_PIN
    #define RA_ENDSWITCH_EAST_SENSOR_PIN P1_22
#endif
#ifndef RA_ENDSWITCH_WEST_SENSOR_PIN
    #define RA_ENDSWITCH_WEST_SENSOR_PIN P1_21
#endif
#ifndef DEC_ENDSWITCH_UP_SENSOR_PIN
    #define DEC_ENDSWITCH_UP_SENSOR_PIN P1_20
#endif
#ifndef DEC_ENDSWITCH_DOWN_SENSOR_PIN
    #define DEC_ENDSWITCH_DOWN_SENSOR_PIN P1_19
#endif

// GPS uses dedicated serial by default.
#ifndef GPS_SERIAL_PORT
    #define GPS_SERIAL_PORT Serial3
#endif

// Dew heater outputs (HE0 / HE1)
#ifndef DEW_HEATER_1_PIN
    #define DEW_HEATER_1_PIN P2_05
#endif
#ifndef DEW_HEATER_2_PIN
    #define DEW_HEATER_2_PIN P2_04
#endif

// Serial port for external debugging
#if DEBUG_SEPARATE_SERIAL == 1
    #ifndef DEBUG_SERIAL_PORT
        #define DEBUG_SERIAL_PORT Serial1
    #endif
#else
    #ifndef DEBUG_SERIAL_PORT
        #define DEBUG_SERIAL_PORT Serial
    #endif
#endif
