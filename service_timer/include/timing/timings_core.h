#ifndef TIMINGS_CORE_H
#define TIMINGS_CORE_H

#include <Arduino.h>

	#define NUMBER_OF_TIMINGS 0x10 // 0x20
	#define WARNING_INTERVAL 0.8
	#define DELAY_CYCLE_MS 60000
	#define DELAY_CYCLE_MINS 1 //DELAY_CYCLE_MS/1000/60
	#define MINUTES_PER_HOUR 60

	// The central struct used for saving the timings.
	typedef struct ll{  
    uint16_t runTime; //In minutes since last reset
    uint16_t stopTime; //In minutes
  	} TIMING;

  	/*
  	Increases timings according to NUMBER_OF_TIMINGS define
  	*/
	void increase_all_timings(TIMING * timings);

	/*
	Changes timing identified by ID which is the relative position to the start.
	*/
	void change_timing(uint16_t id, uint16_t runTime, uint16_t stopTime);

 #endif