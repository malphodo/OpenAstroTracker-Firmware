import serial
import time

print("Connecting to COM3...")
ser = serial.Serial('COM3', 19200, timeout=2)
time.sleep(0.5)

print("Sending: :I# (Initialize)")
ser.write(b':I#')
time.sleep(0.3)

print("Sending: :gT# (GPS sync - listen for beep if fix available)")
ser.write(b':gT#')
time.sleep(2)

# Read response
resp = b''
while ser.in_waiting > 0:
    resp += ser.read(1)
    time.sleep(0.01)

ser.close()

if resp:
    print("Response: " + str(resp))
else:
    print("Done - check if beep was triggered")
