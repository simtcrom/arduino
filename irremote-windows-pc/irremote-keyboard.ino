#include <IRremote.h>
#include <Keyboard.h>
int IRpin = 10;

void setup()
{
  IrReceiver.begin(IRpin);
}

void loop()
{
  if(IrReceiver.decode())
  {
    unsigned long Comes_Data = IrReceiver.decodedIRData.decodedRawData;
    switch(Comes_Data)
      {
        case 3108437760:  Keyboard.press(KEY_LEFT_CTRL); //left ctrl key
                         Keyboard.press(KEY_UP_ARROW); //up arrow key
                         delay(100);
                         Keyboard.releaseAll();
                         break;
        case 3927310080:  Keyboard.press(KEY_LEFT_CTRL); //left ctrl key
                         Keyboard.press(KEY_DOWN_ARROW); //down arrow key
                         delay(100);
                         Keyboard.releaseAll();
                         break;
        case 3208707840:  Keyboard.press(32); //space key
                         delay(100);
                         Keyboard.releaseAll();
                         break;
        case 3091726080:  Keyboard.press(109); //m key
                         delay(100);
                         Keyboard.releaseAll();
                         break;
        case 3158572800:  Keyboard.press(KEY_RIGHT_ARROW); //right arrow key
                         delay(100);
                         Keyboard.releaseAll();
                         break;
        case 3141861120:  Keyboard.press(KEY_LEFT_ARROW); //left arrow key
                         delay(100);
                         Keyboard.releaseAll();
                         break; 
      }
    IrReceiver.resume();    
  }
}
