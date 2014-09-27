#include <Arduino.h>
#include <EEPROM.h>
#include <avr/eeprom.h>

#include "include/timing/timings_core.h"


  uint8_t calc_parity_value(TIMING * input) {
    return  lowByte(input->stopTime) ^ 
            highByte(input->stopTime) ^ 
            lowByte(input->runTime) ^
            highByte(input->stopTime);
  }

void save_timings_to_eeprom(uint16_t * adress, TIMING * input) {
    //Serial.write("Saing");
    for (int i = 0; i < NUMBER_OF_TIMINGS; i ++){
     
     adress = (uint16_t *) (i*5);
     eeprom_write_word(adress, input->runTime);
     eeprom_write_word(adress+1, input->stopTime);
     eeprom_write_byte((uint8_t*) (adress+2), calc_parity_value(input));
    
     /*
     Serial.write("Saved ");
     Serial.print(eeprom_read_word(adress));
     Serial.write(" as runtime at adress ");
     Serial.print((int) adress);
     Serial.write(" as runtime and ");
     Serial.print(eeprom_read_word(adress+1));
     Serial.write(" as stoptime at adress ");
     Serial.print((int) (adress +1));
     Serial.write(" \n");
     */
     input ++;
   }
  }


  void read_state_from_eeprom(uint16_t * adress, TIMING * timings) {
    for (int i = 0; i<NUMBER_OF_TIMINGS; i ++){
      adress = (uint16_t *) (i*5);
      timings[i].runTime = eeprom_read_word (adress);
      timings[i].stopTime = eeprom_read_word (adress+1);
      #if DEBUG
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
      #endif

    }
  }

