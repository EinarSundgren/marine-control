#ifndef EEPROM_CORE_H
#define EEPROM_CORE_H
	#define START_SAVE_ADRESS 0

	void save_timings_to_eeprom(uint16_t * adress, TIMING * input);
	void read_state_from_eeprom(uint16_t * adress, TIMING* timings);
	uint8_t calc_parity_value(TIMING * input);

#endif