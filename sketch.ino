#define ir_lsr 3
#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

enum class state_e {pending, header_mark, in_header, bit_mark, in_bit, ready};

volatile state_e state = state_e::pending;
volatile uint32_t dataRaw = 0x00000000;
uint32_t dataReady = 0;
uint8_t bitcount = 0;

uint32_t bitSpaceStartUs = 0;
uint32_t headerStartUs = 0;
uint32_t frameStartMs = 0;

void getIr() {
  uint32_t ms = millis();
  uint32_t us = micros();

  bool isFalling = digitalRead(ir_lsr);
  bool isRising = !isFalling;

  //Serial.println(dataRaw, HEX);

  if(state == state_e::pending) {
    frameStartMs = ms;
    bitcount = 0;
    dataRaw = 0;
    if(isRising) {
      state = state_e::header_mark;
      headerStartUs = us;
    }
  } else if (state == state_e::header_mark) {
    if (isFalling && ms - frameStartMs > 7 && ms - frameStartMs < 11) {
      state = state_e::in_header;
      headerStartUs = us;
    } else {
      state = state_e::pending;
    }
  } else if (state == state_e::in_header) {
    if(isRising && us - headerStartUs > 3500 && us - headerStartUs < 5500) {
      state = state_e::bit_mark;
    } else {
      state = state_e::pending;
    }
  } else if (state == state_e::bit_mark) {
    if(isFalling) {
      bitSpaceStartUs = us;
      state = state_e::in_bit;
    } else {
      state = state_e::pending;
    }
  } else if (state == state_e::in_bit) {
    if(isRising) {
      state = state_e::bit_mark;
      if (us - bitSpaceStartUs > 350 && us - bitSpaceStartUs < 650) {
        //skip
      } else if (us - bitSpaceStartUs > 1400 && us - bitSpaceStartUs < 1900) {
        bitSet(dataRaw, bitcount);
      }
      else state = state_e::pending;

      if(++bitcount>=32) {
        bitcount = 0;
        state = state_e::ready;
      }

    }
    else state = state_e::pending;

  }

}

void setup() {
  Serial.begin(9600);
  pinMode(ir_lsr, INPUT_PULLUP);
  attachInterrupt( digitalPinToInterrupt(ir_lsr) , getIr, CHANGE);
  lcd.begin(16, 2);
}

bool LED_ON = false;

void loop() {

  if(state==state_e::ready) {
    //Serial.println(dataRaw, HEX);
    int its = 1;
    for ( uint8_t j = 0 ; j < 32 ; j+=8 ) {
      for (int k = 0; k < 8; k++) {
        bitWrite(dataReady, j+k, bitRead(dataRaw, 32-(8*its)+k));
      }
      its++;
    }
    Serial.println(dataReady, HEX);

    //Starts from 32 because that is the beginning of a number.
    //Example: B11001100. The first one is actuall the 7th bit.
    //for ( uint8_t i = 32 ; i > 0; i--)   {
      //Serial.print( bitRead( dataReady, i-1) ) ;
      //if ( (i-1) % 8 == 0  ) Serial.print(' ' ) ;
    //}
    state = state_e::pending;

    lcd.clear();
    lcd.print("HEX: ");

    lcd.print(dataReady, HEX);
    
  }

}