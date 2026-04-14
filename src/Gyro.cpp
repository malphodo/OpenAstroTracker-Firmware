#include "../Configuration.hpp"
#include "Utility.hpp"
#include "Gyro.hpp"

#if USE_GYRO_LEVEL == 1

PUSH_NO_WARNINGS
    #include <Wire.h>  // I2C communication library
POP_NO_WARNINGS

/**
 * Tilt, roll, and temperature measurementusing the MPU-6050 MEMS gyro.
 * See: https://invensense.tdk.com/products/motion-tracking/6-axis/mpu-6050/
 * Datasheet: https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
 * Register descriptions: https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
 * */

bool Gyro::isPresent(false);
static uint8_t s_mpuAddr = 0x68;

void Gyro::startup()
/* Starts up the MPU-6050 device.
   Reads the WHO_AM_I register to verify if the device is present.
   Wakes device from power-down.
   Sets accelerometers to minimum bandwidth to reduce measurement noise.
*/
{
    // Initialize interface to the MPU6050
    LOG(DEBUG_INFO, "[GYRO]:: Starting");
    Wire.begin();

    // Read WHO_AM_I and accept both MPU6050 addresses:
    // 0x68 (AD0 low, most common) and 0x69 (AD0 high).
    byte id       = 0;
    bool idReadOk = false;
    isPresent     = false;
    for (uint8_t addr : {static_cast<uint8_t>(0x68), static_cast<uint8_t>(0x69)})
    {
        Wire.beginTransmission(addr);
        Wire.write(MPU6050_REG_WHO_AM_I);
        if (Wire.endTransmission() != 0)
        {
            continue;
        }

        Wire.requestFrom(addr, static_cast<uint8_t>(1));
        if (Wire.available() < 1)
        {
            continue;
        }

        id       = Wire.read();
        idReadOk = true;
        // Valid WHO_AM_I for MPU-6050 has lower 6 bits = 0x34.
        if ((id & 0x7E) == 0x68)
        {
            s_mpuAddr = addr;
            isPresent = true;
            break;
        }
    }

    if (!isPresent)
    {
        LOG(DEBUG_INFO, "[GYRO]:: Not found! WHO_AM_I ok=%d val=0x%02X", idReadOk ? 1 : 0, id);
        return;
    }

    // Execute 1 byte write to MPU6050_REG_PWR_MGMT_1
    Wire.beginTransmission(s_mpuAddr);
    Wire.write(MPU6050_REG_PWR_MGMT_1);
    Wire.write(0);  // Disable sleep, 8 MHz clock
    Wire.endTransmission();

    // Execute 1 byte write to MPU6050_REG_PWR_MGMT_1
    Wire.beginTransmission(s_mpuAddr);
    Wire.write(MPU6050_REG_CONFIG);
    Wire.write(6);  // 5Hz bandwidth (lowest) for smoothing
    Wire.endTransmission();

    LOG(DEBUG_INFO, "[GYRO]:: Started on I2C 0x%02X", s_mpuAddr);
}

void Gyro::shutdown()
/* Shuts down the MPU-6050 device.
   Currently does nothing.
*/
{
    LOG(DEBUG_INFO, "[GYRO]: Shutdown");
    // Nothing to do
}

angle_t Gyro::getCurrentAngles()
/* Returns roll & tilt angles from MPU-6050 device in angle_t object in degrees.
   If MPU-6050 is not found then returns {0,0}.
*/
{
    const int windowSize = 16;
    // Read the accelerometer data
    struct angle_t result;
    result.pitchAngle = 0;
    result.rollAngle  = 0;
    if (!isPresent)
        return result;  // Gyro is not available

    for (int i = 0; i < windowSize; i++)
    {
        // Execute 6 byte read from MPU6050_REG_WHO_AM_I
        Wire.beginTransmission(s_mpuAddr);
        Wire.write(MPU6050_REG_ACCEL_XOUT_H);
        Wire.endTransmission();
        Wire.requestFrom(s_mpuAddr, static_cast<uint8_t>(6));  // Read 6 registers total, each axis value is stored in 2 registers
        if (Wire.available() < 6)
        {
            continue;
        }
        int16_t AcX = Wire.read() << 8 | Wire.read();  // X-axis value
        int16_t AcY = Wire.read() << 8 | Wire.read();  // Y-axis value
        int16_t AcZ = Wire.read() << 8 | Wire.read();  // Z-axis value

        // Calculating the Pitch angle (rotation around Y-axis)
        result.pitchAngle += ((atanf(-1 * AcX / sqrtf(powf(AcY, 2) + powf(AcZ, 2))) * 180.0f / 3.14159265f) * 2.0f) / 2.0f;
        // Calculating the Roll angle (rotation around X-axis)
        result.rollAngle += ((atanf(-1 * AcY / sqrtf(powf(AcX, 2) + powf(AcZ, 2))) * 180.0f / 3.14159265f) * 2.0f) / 2.0f;

        delay(10);  // Decorrelate measurements
    }

    result.pitchAngle /= windowSize;
    result.rollAngle /= windowSize;
    #if GYRO_AXIS_SWAP == 1
    float temp        = result.pitchAngle;
    result.pitchAngle = result.rollAngle;
    result.rollAngle  = temp;
    #endif
    return result;
}

float Gyro::getCurrentTemperature()
/* Returns MPU-6050 device temperature in degree C.
   If MPU-6050 is not found then returns 99 (C).
*/
{
    if (!isPresent)
        return 99.0f;  // Gyro is not available

    // Execute 2 byte read from MPU6050_REG_TEMP_OUT_H
    Wire.beginTransmission(s_mpuAddr);
    Wire.write(MPU6050_REG_TEMP_OUT_H);
    Wire.endTransmission();
    Wire.requestFrom(s_mpuAddr, static_cast<uint8_t>(2));  // Read 2 registers total, the temperature value is stored in 2 registers
    if (Wire.available() < 2)
        return 99.0f;
    int16_t tempValue = Wire.read() << 8 | Wire.read();  // Raw Temperature value

    // Calculating the actual temperature value
    float result = static_cast<float>(tempValue) / 340.0f + 36.53f;
    return result;
}
#endif
