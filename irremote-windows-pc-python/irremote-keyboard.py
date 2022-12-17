#Download These Libraries
#pip install pyserial
#pip install keyboard

import serial
import keyboard

arduino = serial.Serial("COM8",9600)  #Change The "COM" To Yours

while True:

    HexData=arduino.readline().rstrip().decode()
    print(HexData)
        
    if HexData == "3108437760":                #f3
        keyboard.press_and_release('f3')
    elif HexData == "EA15FF00":              #f2
        keyboard.press_and_release('f2')
    elif HexData == "3208707840":              #spacebar
        keyboard.press_and_release('space')
    elif HexData == "B847FF00":              #f4
        keyboard.press_and_release('f4')
