#include <Arduino.h>
#include "include/timing/timings_core.h"

 void change_timing(TIMING * timings, uint16_t id, uint16_t runTime, uint16_t stopTime) {
    timings[id].runTime = runTime;
    timings[id].stopTime = stopTime;
  }

    void increase_all_timings(TIMING * timings){
  // Update timings
  TIMING * current; 
  
  for (int i = 0; i < NUMBER_OF_TIMINGS; i ++){
    current = &timings[i];
    current->runTime += DELAY_CYCLE_MINS;

    #if DEBUG
      Serial.write("Current ");
      Serial.print(current->runTime);
      Serial.write(" Stoptime: ");
      Serial.print(current->stopTime);
      Serial.write("\n");
    #endif

    /*
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
    */
  }


  }