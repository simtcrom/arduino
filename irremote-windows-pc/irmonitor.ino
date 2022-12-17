#include <IRremote.h>
int IRpin = 10;

void setup()
{
  Serial.begin(9600);  
  IrReceiver.begin(IRpin, ENABLE_LED_FEEDBACK);
}

void loop()
{
  if(IrReceiver.decode())
  {
    unsigned long Comes_Data = IrReceiver.decodedIRData.decodedRawData;
    Serial.println(Comes_Data);    
    IrReceiver.resume();
  }
}
