# Arduino IR remote for windows

- This is a simple IR remote implementation for windows using Arduino IRremote and Keboard libraries.
- Only [Arduino Pro Micro][df3] or Leonardo can be used here (due to Keyboard library compatibility).
- IR reciever module used here is [VS1838B][df1]
- Generic IR [transmitter][df2] is used.

## Wiring

![wiring](https://user-images.githubusercontent.com/6823535/208236893-01d3c11c-f45e-4988-9f7a-bda29f02d772.jpg)

## Programming

- Flash [irmonitor.ino][df4] first.
- Open serial monitor.
- press each button on remote and note down the decoded values.
- Open [irremote-keyboard.ino][df5] in [ArduinoIDE][df6].
- Edit the decoded values as per what you noted down earlier.
- Edit Keyboard.press() as per your requirements
- Flash the program and remote is ready to use

## Notes

- After flashing the first program ([irmonitor.ino][df4]), in serial monotor sometimes, same remote keys produce random codes. I dont know the explanation for this.
- In that case, unplug [VS1838B][df1] and power cycle arduino, most cases that resolve this issue.

[df1]: <https://robu.in/product/ir-receiving-head-vs1838b-remote-control-receiver/>
[df2]: <https://robu.in/product/black-ir-remote-control-without-battery/>
[df3]: <https://robu.in/product/pro-micro-5v-16m-mini-leonardo-micro-controller-development-board-for-arduino/>
[df4]: <https://github.com/simtcrom/arduino/blob/main/irremote-windows-pc/irmonitor.ino>
[df5]: <https://github.com/simtcrom/arduino/blob/main/irremote-windows-pc/irremote-keyboard.ino>
[df6]: <https://www.arduino.cc/en/software>
