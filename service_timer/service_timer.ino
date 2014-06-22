#include <util/crc16.h>
#include <EEPROM.h>
#include <avr/eeprom.h>

#define NUMBER_OF_TIMINGS 50
#define WARNING_INTERVAL 0.8
#define DELAY_CYCLE_MS 120000 //Store the info each 2 minutes.
#define DELAY_CYCLE_MINS DELAY_CYCLE_MS/1000/60
#define MINUTES_PER_HOUR 60
#define HIGH_CHECKSUM_ADRESS 500
#define LOW_CHECKSUM_ADRESS 1000

#define START_SAVE_ADRESS 0
#define UPPER_OFFSET 512

#define PIN_INTERNAL_LED 13
#define PIN_VOLTAGE_SENSE 3

#define DEBUG 1

/*
Defines of lsb and msb for reading and writing to EEPROM and serial.
*/
#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

  typedef struct ll{  
    uint16_t runTime; //In minutes since last reset
    uint16_t stopTime; //In minutes
  } TIMING;

  TIMING timings[NUMBER_OF_TIMINGS];
  TIMING * current = timings;
  boolean doService;
  boolean planService;
  boolean write_to_upper = false;
  uint16_t accumDelay; // Accumulated delay
  uint16_t incomingByte = 0;   // for incoming serial data
  //uint16_t * save_adress = START_SAVE_ADRESS
  uint8_t eeprom_checksum;

/*
  Function declarations
*/
void setup();
void change_timing(uint16_t id, uint16_t runTime, uint16_t stopTime);
void save_timing_to_eeprom(uint16_t adress, uint16_t runtime, uint16_t stopTime);
void read_state_from_eeprom(); // No params since there is limited possibilities
byte calc_parity_value(TIMING * input);


int main(void) {
init();
setup();


while(true) {
  
  /*
    Alternate between upper and lower eeprom memory 
    areas to protect from brown outs.
  */
  if (write_to_upper) {
    write_to_upper = false;
  } else {
    write_to_upper = true;
  }

  current = timings;
  doService = false;
  planService = false;
  
  /*
  if (Serial.available() > 0) {
                // read the incoming byte:
                incomingByte = Serial.read();

                // say what you got:
                Serial.print("I received: ");
                Serial.println(incomingByte, DEC);
  }
  */
  delay(DELAY_CYCLE_MS-accumDelay);

  for (int i = 0; i < NUMBER_OF_TIMINGS; i ++){
    Serial.print(i);
    current = &timings[i];
    current->runTime += DELAY_CYCLE_MINS;

    /*
        Alterate between memory areas
    */
    //if (write_to_upper){
     // save_timing_to_eeprom((i*4) + UPPER_OFFSET, current->currTime, current->stopTime);
    save_timing_to_eeprom((uint16_t *) (i*5), current);
    //} else {
      
    //}

    #if 0
      Serial.write (" Current:");
      Serial.print (current->runTime);
      Serial.write (" Stop:");
      Serial.print (current->stopTime);
      Serial.write ("\n");
    #endif

    if (current->runTime >= current->stopTime) {
    
    #if DEBUG
      Serial.write("Curr ");
      Serial.print(current->runTime);
      Serial.write(" >= ");
      Serial.print(current->stopTime);
      Serial.write("\n");
    #endif

      doService = true;  
    }
    
    if ((current->runTime >= current->stopTime * WARNING_INTERVAL) 
        && (doService == false)) {
        planService = true;
    }
  }

  if (doService) {
    digitalWrite(13, HIGH);   
    delay(1000);              // wait for a second
    digitalWrite(13, LOW);    // set the LED off
    delay(1000);             // wait for a second
    Serial.write("Do service \n");
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
    Serial.write("Plan service \n");
    accumDelay = 1900;
  } else {
    Serial.write("All clear \n");
    accumDelay = 0;
  }

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

  // Set the interrupt for voltage drop on the optocoupler
  attachInterrupt(PIN_VOLTAGE_SENSE, falling, FALLING);
  Serial.begin(9600);

  // Just temporary initialization
  
  #if 0
  for (int i = 0; i < NUMBER_OF_TIMINGS; i ++) {
    current = &timings[i];
    current->runTime = 0;
    current->stopTime = (100*MINUTES_PER_HOUR)+i; // 100 hours
  }
  #else
  read_state_from_eeprom((uint16_t*) START_SAVE_ADRESS);
  #endif
  }

  void change_timing(uint16_t id, uint16_t runTime, uint16_t stopTime) {
    timings[id].runTime = runTime;
    timings[id].stopTime = stopTime;
  }

  void save_timing_to_eeprom(uint16_t * adress, TIMING * input) {
    
     eeprom_write_word(adress, input->runTime);
     eeprom_write_word(adress+1, input->stopTime);
     eeprom_write_byte((uint8_t*) (adress+2), calc_parity_value(input));
     Serial.write("Saved ");
     Serial.print(eeprom_read_word(adress));
     Serial.write(" as runtime at adress ");
     Serial.print((int) adress);
     Serial.write(" as runtime and ");
     Serial.print(eeprom_read_word(adress+1));
     Serial.write(" as stoptime at adress ");
     Serial.print((int) (adress +1));
     Serial.write(" \n");
  }

  void read_state_from_eeprom(uint16_t * adress) {
    for (int i = 0; i<NUMBER_OF_TIMINGS; i ++){
      adress = (uint16_t *) (i*5);
      timings[i].runTime = eeprom_read_word (adress);
      timings[i].stopTime = eeprom_read_word (adress+1);
      Serial.write("Read ");
      Serial.print(timings[i].runTime);
      Serial.write(" as runtime from adress ");
      Serial.print((int)adress);
      Serial.write(" and ");
      Serial.print(timings[i].stopTime);
      Serial.write(" as stoptime ");
      Serial.write(" from adress");
      Serial.print((int) (adress+1));

      Serial.write(" and ");
      Serial.print(eeprom_read_byte((uint8_t*) (adress+2)) ^ 
                   calc_parity_value(&timings[i]));
      Serial.write(" as parity check. \n");

    }
  }

  uint8_t calc_parity_value(TIMING * input) {
    return  lowByte(input->stopTime) ^ 
            highByte(input->stopTime) ^ 
            lowByte(input->runTime) ^
            highByte(input->stopTime);
  }

