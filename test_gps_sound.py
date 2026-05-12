#!/usr/bin/env python3
"""Test GPS fix sound on COM3"""

import serial
import time

def send_command(ser, cmd):
    """Send a Meade command and get response"""
    print(f"Sending: {cmd}")
    ser.write(cmd.encode())
    time.sleep(0.1)
    
    # Read response
    response = ""
    while ser.in_waiting > 0:
        response += ser.read(1).decode('ascii', errors='ignore')
    
    if response:
        print(f"Response: {response}")
    else:
        print("No response")
    return response

def main():
    # Connect to COM3
    try:
        ser = serial.Serial('COM3', 19200, timeout=1)
        print(f"Connected to COM3 at 19200 baud")
        
        # Initialize
        print("\n=== Initialize ===")
        send_command(ser, ":I#")
        time.sleep(0.2)
        
        # Try GPS sync (should trigger sound on successful fix)
        print("\n=== GPS Sync (should trigger sound if fix found) ===")
        send_command(ser, ":gT#")
        time.sleep(1)
        
        # Query GPS diagnostics
        print("\n=== GPS Diagnostics ===")
        send_command(ser, ":XGP#")
        time.sleep(0.2)
        
        ser.close()
        print("\nDone!")
        
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
