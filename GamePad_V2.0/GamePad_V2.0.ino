#include <UsbGamePad.h>


// If the timer isr is corrected
// to not take so long change this to 0.
#define BYPASS_TIMER_ISR 0      //disabled bypass, seems to be working fine now?
#define ENABLE_DEBUGGING 0      //don't expect proper input reads from pin0 and pin1 (rx/tx) if enabled.

//powersaving constants
#define ENABLE_POWERSAVING 0 //TODO: fix the interrupt issue, then test the concept.
#if ENABLE_POWERSAVING
  #define SECONDSTILLSLEEP 10
  #define WAKEPIN 2
#endif

//general constants
#define REPORTSIZE 3

unsigned long previousMillis = 0;

byte tempBuffer[REPORTSIZE];
byte sendBuffer[REPORTSIZE];
bool updatesAvailable;

void setup() {

 
  //Set pinmodes arduino style
  // First 8 bits
  pinMode(A0, INPUT_PULLUP);  // Button 6         B00000001 PORTC0
  pinMode(A1, INPUT_PULLUP);  // Select           B00000010 PORTC1
  pinMode(A2, INPUT_PULLUP);  // Start            B00000100 PORTC2
  pinMode(A3, INPUT_PULLUP);  // Button 8 (back)  B00001000 PORTC3
  pinMode(A4, INPUT_PULLUP);  // Button 9 (back)  B00010000 PORTC4
  pinMode(A5, INPUT_PULLUP);  // Button 10 (back) B00100000 PORTC5
  pinMode(0, INPUT_PULLUP);   // Left             B00000001 PORTD0
  pinMode(1, INPUT_PULLUP);   // Up               B00000010 PORTD1


  // Second 8 bits
  pinMode(8, INPUT_PULLUP);   // Button 7 (back)  B00000001 PORTB0
  pinMode(9, INPUT_PULLUP);   // Button 1         B00000010 PORTB1
  pinMode(10, INPUT_PULLUP);  // Button 2         B00000100 PORTB2
  pinMode(11, INPUT_PULLUP);  // Button 3         B00001000 PORTB3
  pinMode(12, INPUT_PULLUP);  // Button 4         B00010000 PORTB4
  pinMode(13, INPUT_PULLUP);  // Button 5         B00100000 PORTB5
  pinMode(6, INPUT_PULLUP);   // Right            B01000000 PORTD6
  pinMode(7, INPUT_PULLUP);   // Down             B10000000 PORTD7

  #if BYPASS_TIMER_ISR
    // disable timer 0 overflow interrupt (used for millis)
    TIMSK0&=!(1<<TOIE0); // ++
  #endif
  
  #if ENABLE_DEBUGGING
    //put your debugging stuff here
    Serial.begin(9600);
    Serial.print("Debugging enabled");
  #endif
  
}
  
#if BYPASS_TIMER_ISR
  void delayMs(unsigned int ms) {
    for (int i = 0; i < ms; i++) {
      delayMicroseconds(1000);
    };
  }
#endif

  
void loop() {
 
  UsbGamePad.update();  
  
  //keep as 50 or less
  #if BYPASS_TIMER_ISR
    delayMs(20);
  #else
    delay(20);
  #endif
    
  updatesAvailable = false;

  //read pins and assign to temp buffer whenever it does not equal to the sendbuffer. Also set updatesAvailable to TRUE if a change was detected.
  for (int i = 0; i < REPORTSIZE; i++){
    tempBuffer[i] = readPinsToByte(i);
    if (tempBuffer[i] != sendBuffer[i]) {
      sendBuffer[i] = tempBuffer[i];
      updatesAvailable = true;
    }
  }
    
  if (updatesAvailable) {

    //assign the send buffer to the report buffer.
    for (int i = 0; i < REPORTSIZE; i++) {
      UsbGamePad.reportBuffer[i] = sendBuffer[i];
    }

    //whenever ready, send the stuff to the host and set the updatesAvailable to FALSE
    if (usbInterruptIsReady()) {
      usbSetInterrupt(UsbGamePad.reportBuffer, sizeof( UsbGamePad.reportBuffer));
      updatesAvailable = false;
    }
  }
}

byte readPinsToByte(int bufferNumber) {
  
  //bufferNumber is the position within report array which will be reported to the host.
  //you can define here how the report data is sourced. 
  
  byte result = 0x00;

  switch (bufferNumber) {

    case 0:
      //define logic for buffer zero here
      //first left shift 6 bits
      byte temp;
      temp = (byte) (PIND << 6);

      //then shift back again, this way we have clean 2 bits we need
      temp = (byte) temp >> 6;
      
      //left shift PINC by 2 bits and apply OR with result from PIND (temp)
      result = (byte) ~((PINC << 2) | temp);
      break;
    case 1:
      //define logic for buffer zero here
      //shift PINB 2 bits to the left and PIND 6 bits to the right. 
      //Combine both results by using OR logic and assign it to result
      result = (byte) ~((PINB << 2) | (PIND >> 6));
      break;
    case 2:
      //define logic for buffer zero here
      //we're not using these hence set to 0x00 which is same as B00000000
      result = 0x00;
      break;
    default:
      result = 0x00;
      
  }
  
  #if ENABLE_DEBUGGING
    //put your debugging stuff here
  
    Serial.print("PINC STATE:");
    Serial.println((byte) (PINC), BIN);
    Serial.print("PIND STATE:");
    Serial.println((byte) (PIND), BIN);
    
    Serial.println("RESULT:");
    Serial.println( (byte) (PINC << 2) , BIN);
    byte test = (byte) (PIND << 6);
    Serial.println(test, BIN);
    test = test >> 6;
    Serial.println(test, BIN);
    Serial.println( (byte) ((PINC << 2) | test) , BIN);

    //make sure to end with return
  #endif

  return result;
  
}

