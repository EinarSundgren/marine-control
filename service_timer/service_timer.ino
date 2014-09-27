#include <util/crc16.h>

#include <Time.h>
#include "include/proxy/mproxy_protocol.h"
#include "include/timing/timings_core.h"
#include "include/eeprom/eeprom_core.h"

#define HIGH_CHECKSUM_ADRESS 500
#define LOW_CHECKSUM_ADRESS 1000

#define PIN_INTERNAL_LED 13
#define PIN_VOLTAGE_SENSE 3
#define PIN_3_INTERRUPT 1
#define PIN_SEND_RESET 5

/*
Debug flags
*/
#define DEBUG 0

  TIMING timings[NUMBER_OF_TIMINGS];
  TIMING * current = timings;
  MC_PROXY_PROTOCOL in_data;

  boolean write_to_upper = false;

  uint16_t accumDelay; // Accumulated delay
  uint8_t eeprom_checksum;

/*
  Function declarations
*/
void setup();


/*
Interrupt related functions
*/
void power_off();
void power_back();

int main(void) {
  init();
  setup();
  mcProtocolInit(&in_data);

 // Just temp before we can calculate it properly.
  uint8_t crc = 0x02;
  unsigned long lastSave = millis();
  while(true) {
    //current = 
    //timings;
    //doService = false;
    //planService = false;
  
  
    // Delay loop for the expected time
   
    while (millis()-lastSave<DELAY_CYCLE_MS) {
      if (Serial.available()){
        //digitalWrite(13, HIGH);
          uint8_t buffer_result = buffer_protocol(&in_data, Serial.read());
          uint8_t process_result;

          if (buffer_result == FULL_FRAME_BUFFERED) { 
                process_result = process_frame(&in_data, timings);
                mcProtocolInit(&in_data);
            }

          // fay of the changes are updating, resend the data to the serial
          if (process_result == STOPTIME_CHANGED || process_result == RUNTIME_CHANGED){
           broadcast_all_timings(timings);
          };
          
    } else {
      //digitalWrite(13, LOW);
    }

  }

  lastSave = millis();
  increase_all_timings(&timings[0]);
  broadcast_all_timings(&timings[0]);

/*
  if (doService) {
    digitalWrite(13, HIGH);   
    delay(1000);              // wait for a second
    digitalWrite(13, LOW);    // set the LED off
    delay(1000);             // wait for a second
    // Serial.write("Do service \n");
    accumDelay = 2000;
  }



  else if (planService){  
    digitalWrite(13, HIGH);   // set the LED on
    delay(300);              // wait for a second
    digitalWrite(13, LOW);    // set the LED off
    delay(300);             // wait for a second
    digitalWrite(13, HIGH);   // set the LED on
    delay(300);              // wait for a second
    digitalWrite(13, LOW);    // set the LED off
    delay(1000);             // wait for a second
    // Serial.write("Plan service \n");
    accumDelay = 1900;
  } else {
    // Serial.write("All clear \n");
    accumDelay = 0;
  }

  */

  /*
  if(write_to_upper){
  EEPROM.write(HIGH_CHECKSUM_ADRESS, eeprom_checksum);
  } else {
  EEPROM.write(LOW_CHECKSUM_ADRESS, eeprom_checksum);
  }
  */

  

  }
}

  void setup() {         
    pinMode(PIN_INTERNAL_LED, OUTPUT);
    pinMode(PIN_VOLTAGE_SENSE, INPUT);
    pinMode(PIN_SEND_RESET, OUTPUT);
//    digitalWrite(PIN_SEND_RESET, HIGH);
//    wdt_disable();

    // Set the interrupt for voltage drop on the optocoupler
    attachInterrupt(PIN_3_INTERRUPT, power_off, FALLING);

    Serial.begin(9600);
    //Serial.write("Starting\n");

    //Just temporary initialization
  
  #if 0
  for (int i = 0; i < NUMBER_OF_TIMINGS; i ++) {
    current = &timings[i];
    current->runTime = 0;
    current->stopTime = (100*MINUTES_PER_HOUR)+i; // 100 hours
  }
  #else
    read_state_from_eeprom((uint16_t*) START_SAVE_ADRESS, timings);
    broadcast_all_timings(timings);
  #endif
  }

  void power_off(){
    #if DEBUG
    Serial.write("Saving");
    #endif
    // Stop the power down interrupt from recurring.
    digitalWrite(13, HIGH);
    detachInterrupt(PIN_3_INTERRUPT);
    
    noInterrupts();
    save_timings_to_eeprom(START_SAVE_ADRESS, (TIMING *) &timings[0]);
    interrupts();
    
    //digitalWrite(13, LOW);
    
    attachInterrupt(PIN_3_INTERRUPT, power_back, RISING);
    // wdt_enable(WDTO_8S);
    while(1){};
  }

  void power_back() {
    digitalWrite(13, LOW);

    //detachInterrupt(PIN_3_INTERRUPT);
    //attachInterrupt(PIN_3_INTERRUPT, power_off, FALLING);
    //wdt_enable(WDTO_4S);
    // Jump to start of program.
    asm volatile ("  jmp 0");
  }