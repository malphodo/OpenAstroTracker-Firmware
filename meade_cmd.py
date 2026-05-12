#!/usr/bin/env python3
"""Send Meade commands to OpenAstroTracker via serial (Bluetooth)"""

import serial
import sys
import time

def main():
    port_name = sys.argv[1] if len(sys.argv) > 1 else "COM15"
    command = sys.argv[2] if len(sys.argv) > 2 else ":gT#"
    
    print(f"Opening {port_name} at 19200 baud...")
    
    try:
        # Increase timeout for Bluetooth
        ser = serial.Serial(port_name, 19200, timeout=2)
        time.sleep(0.5)
        
        print(f"Connected! Sending: {command}")
        ser.write(command.encode())
        
        # Wait for response
        time.sleep(2)
        
        response = ""
        while ser.in_waiting > 0:
            byte = ser.read(1)
            if byte:
                response += byte.decode('ascii', errors='replace')
        
        ser.close()
        
        if response:
            print(f"Response: {repr(response)}")
        else:
            print("No response received")
            
    except Exception as e:
        print(f"Error: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
