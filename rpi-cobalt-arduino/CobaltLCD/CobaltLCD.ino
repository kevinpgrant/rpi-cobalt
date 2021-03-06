/*
 CobaltLCD - Used to drive a Cobalt Networks LCD Front Panel
 
 This code is based on example code from the 
 following link, which is in the public domain.
 
 http://www.arduino.cc/en/Tutorial/SerialEvent
 
 */
 
#define LCD_EN  22
#define BUTTON_EN 23
#define BUTTON_TR 24
#define LCD_RS  25
#define LCD_RW  26
#define COMMAND_CHAR ~

// serial input mode defines
#define SERIAL_INPUT_DATA 0
#define SERIAL_INPUT_CMND 1

// default mode is to input to the LCD.
int serInpMode = SERIAL_INPUT_DATA;

// character reference for serial input
unsigned int serOffRef = 0;

// buffer for incoming serial data
char serBuff[32]; 

// include the LCD library code:
#include <LiquidCrystal.h>
#include <avr/pgmspace.h>

// character map for the raspberry icon, stored in PROGMEMM
PROGMEM const uint8_t  lcd_char_map[6][8] = {
  { B00011, B01100, B10000, B01100, B00011, B01100, B11110, B10001, },
  { B00000, B11011, B00100, B01110, B11111, B01110, B10001, B00000, },
  { B11000, B00110, B00001, B00110, B11000, B00110, B01111, B10001, },
  { B10000, B01011, B01100, B00100, B00101, B00011, B00000, B00000, },
  { B10001, B01110, B10001, B10001, B00000, B10001, B01110, B00000, },  
  { B00001, B11010, B00110, B00100, B10100, B11000, B00000, B00000, }
  
};

// instantiate the LCD Library with our predefined pins
LiquidCrystal CobaltLCD(LCD_RS, LCD_RW, LCD_EN, 30, 31, 32, 33, 34, 35, 36,37);

// connection tracking
boolean piConnected = false;  // whether the string is complete

char portBuffers[3];

void setup() {
  // set up the button controller outputs
  pinMode(BUTTON_EN, OUTPUT);
  pinMode(BUTTON_TR, OUTPUT);  
  digitalWrite(BUTTON_EN, HIGH);
  digitalWrite(BUTTON_TR, HIGH);
  
  // start the LCD
  CobaltLCD.begin(16,2);
  CobaltLCD.clear();
  
  // wait 3 seconds, this can probably be removed.
  delay(3000);
  
  // print the default booting screen
  CobaltLCD.home();
  CobaltLCD.print("PiCobalt");
  CobaltLCD.setCursor(0,1);
  CobaltLCD.print("Booting...");
 
  // set up the custom RPi logo.
  int curr_char=0;
  int curr_byte=0;
  byte charData[8];

  for(curr_char=0; curr_char < 6; curr_char++){
    for(curr_byte=0; curr_byte < 8; curr_byte++){
     charData[curr_byte] = pgm_read_byte_near( &lcd_char_map[curr_char][curr_byte] );
    }
    CobaltLCD.createChar(curr_char, charData);
  }

  // print the top 3 parts of the icon.    
  CobaltLCD.setCursor(13,0);
  CobaltLCD.write(byte(0));
  CobaltLCD.write(byte(1));
  CobaltLCD.write(byte(2));
  
  // print the bottom 3 parts of the icon.
  CobaltLCD.setCursor(13,1);
  CobaltLCD.write(byte(3));
  CobaltLCD.write(byte(4));
  CobaltLCD.write(byte(5));
  
  // initialize serial:
  Serial.begin(115200);
  Serial.println("~RPiCobalt v1.0 Starting");
}

void loop() {
  // only read the buttons if we are conncted.
  if(piConnected) {
    readButtons();
  } 
 

}
byte button_map[8]={1, 1, 1, 1, 1, 1, 1, 0 };
byte button_map_prev[8]={1, 1, 1, 1, 1, 1, 1, 0 };
void readButtons(){
  // Disable the LCD
  
  //digitalWrite(LCD_EN, HIGH);
  
  // change pin mode
  int i = 0;
  for(i=30; i<38; i++)
    pinMode(i, INPUT);
  
  int btnChanged=0;
  digitalWrite(BUTTON_EN, LOW);
  digitalWrite(BUTTON_TR, LOW);
  for(i=0; i<7; i++){
    button_map[i]=digitalRead(i+31);
    if(button_map[i]!=button_map_prev[i]){
      btnChanged=1;
    }
  }
  button_map[7]='\0';
 
  if(btnChanged) { 
    for(i=0; i<7; i++){
    button_map_prev[i]=button_map[i];
      Serial.print(button_map[i]);
    }
    Serial.print("\r\n");
    button_map[7]='\0';
  }
  digitalWrite(BUTTON_EN, HIGH);
  digitalWrite(BUTTON_TR, HIGH);
  
  for(i=30; i<38; i++)
    pinMode(i, OUTPUT);
   
  
}


void checkSerCmd(){

  // this function is where we will be checking serBuff
  // against our list of commands and running the neccessary 
  // code for each

  // NOTE: serBuff is no larger than 32, so no strncmp should be larger
  // than that if you are adding commands here.  Also, it's best if
  // commands dont overlap, because the first one listed would 
  // be executed as well as the second. (ie: ~DIS and ~DISPLAY)

  if(!strncmp(serBuff, "~HLO", 4)) {
    if(piConnected) {
      Serial.println("~WARN: 301 Already Connected.");
      return;
    }
    // connect request.
    piConnected = true;
    Serial.println("~Connected");
    
    // clear the bottom line
    CobaltLCD.setCursor(0,1);
    CobaltLCD.print("RPi Connected");
    return;
  }
  
  if(!strncmp(serBuff, "~STAT", 5)) {
    // print the status back to the client.
    if(piConnected == false ) {
      Serial.println("~CNCT");
    } else {
      Serial.println("~OK");
    }
    return;
  }
  
  Serial.println("~ERROR: 402 Unrecognized Command.");
  
}


/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  // TODO: This function needs some serioius cleanup and/or
  // possible reworking.
  
  // Function Rework Notes: 
  // XXX: We cannot rely on Serial.available to give us bytes available.
  // XXX: We also cannot rely on the user for perfectly timed input.
  // XXX: We must account for delays between bytes.
  // XXX: Perhaps using a serial input mode structure like
  //       - COMMAND_MODE
  //       - DATA_MODE
  // when in either of these modes, you cannot switch until the serOffRef is reset
  // to 0 (zero) with a newline character or in the case of the LCD Data, after a 
  // loop around after 32 characters.
  //
  // Default mode should be LCD Data Input.
  // data should be read from the serial port until a NULL character or a negative 
  // character is read, indicating no more data.(Since we cannot rely on Serial.available.)
  
  char tmpChar = 0;
  
  // we can assume at least one byte because this function is called when new data is 
  // available on the serial line.
  tmpChar = (char)Serial.read();
  
  // this will exit when no new characters are read.
  while(tmpChar > 0) {
  
    if(tmpChar == '\n'  || tmpChar == '\r' || serOffRef > 32) { 
      // if we get a newline, or we are larger than 32 characters, 
      // so time to reset serOffRef and if in 
      // command mode, reset to data mode.
      
      // new line at the beginning, or empty line, is a status request
      if((tmpChar == '\n' || tmpChar == '\r') && serOffRef == 0) {
        // print the status back to the client.
        if(piConnected == false ) {
          Serial.println("~CNCT");
        } else {
          Serial.println("~OK");
        }        
      }
      
      // reset serOffRef
      serOffRef = 0;
      if(serInpMode == SERIAL_INPUT_CMND) {
        // Check the command, callout?
        checkSerCmd();
        serInpMode = SERIAL_INPUT_DATA;
        // all following characters will be LCD Characters.
      }

      // skip this character and get the next one.
      tmpChar = (char)Serial.read();
      continue; 
    }
    
    if(tmpChar == '~' && serOffRef == 0){
      // if we get a ~ at the beginning of a "line", this means what follows until
      // the next newline is a command, so switch to command mode.
      serInpMode = SERIAL_INPUT_CMND;
    }
    
    if((serOffRef == 0 && serInpMode == SERIAL_INPUT_DATA)) {
      // first character of a new display in data mode, not a new line, 
      // clear the LCD
      CobaltLCD.home();
      CobaltLCD.clear();
      CobaltLCD.home();
    }
    
    // a read could have happened above, so let's double check we have a character
    if(tmpChar <= 0) {
      // nope, leave the loop and exit the function
      break;
    }
    
    // ok we have a character, so let's process it.
    switch (serInpMode) {
      case SERIAL_INPUT_DATA:
        // we are in data mode, so let's just spit the character out to the LCD.
        if(serOffRef == 15) {
          // move to line two if we are greater than 16 chars
          CobaltLCD.setCursor(0,1);
        }
        CobaltLCD.print(tmpChar);
        break;
      case SERIAL_INPUT_CMND: 
        // we are in command mode, so add the character to the buffer.
        serBuff[serOffRef] = tmpChar;
        break;
      default:
        // in an unknown state, print that back to the pi
        Serial.println("~ERROR: 401 Unknown Serial State, send enter to reset.");  
    }
  
    // everything is done, so let's read the next char and increment the offset
    // buffer.
    tmpChar = (char)Serial.read();
    serOffRef++;  
  }
  
}
/*  int byteOfft = 0;
  int i = 0;
  int availableBytes=Serial.available();

  while (availableBytes && byteOfft < 3) {    
    char tmpChar = (char)Serial.read();
    if((tmpChar == '\n' || tmpChar == '\r') && byteOfft == 0) {
      // print ~CNCT to the pi until it replies with an ~HLO
      Serial.println("~CNCT");
    }
    if(tmpChar == '~' && byteOfft == 0) {
       // a command from the pi
       // XXX: Currently only HLO is supported.
       char tmpBuff[4];
       for(i=0; i<4; i++)
         tmpBuff[1]='\0';

       while(byteOfft < 3 && availableBytes) {
         tmpChar = (char)Serial.read();
         if(tmpChar > 0) {
           tmpBuff[byteOfft] = (char)Serial.read();
           byteOfft++;
           delay(10);
         }
       }
       tmpBuff[3] = '\0';
       
       // now let's see what we got.
       if(!strncmp(tmpBuff, "HLO", 3)){
         // got a HLO from the pi.
         piConnected = true;
         Serial.println("~Connected");
          // clear the bottom line
          CobaltLCD.setCursor(0,1);
          CobaltLCD.print("RPi Connected");
       }
       
       //Ignore all else.
    } else {
      // read string for the LCD
      int char_count=0;
    
      if(piConnected){
        // XXX: May need reworking.
        // Read in characters for the LCD.
        while(char_count < 32){
          // a read happens above, so print that char first.
          if(tmpChar > 0) {
            if(char_count == 0) {
              // clear the screen and reset to home.
              CobaltLCD.setCursor(0,0);
              CobaltLCD.clear(); 
              CobaltLCD.setCursor(0,0);
            
              CobaltLCD.print(tmpChar);
              
              // then do another read for the next char.
              tmpChar = (char)Serial.read();
              char_count++;
            
              if(char_count == 15) {
                CobaltLCD.setCursor(0,1);
              }
            }
          } else {
            // break the loop
            char_count = 33;
          }
          delay(10);
        }
      }
    }
    byteOfft++; 
  }
  
}
*/


