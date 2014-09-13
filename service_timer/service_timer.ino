#include <util/crc16.h>
#include <EEPROM.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <Time.h>

#define NUMBER_OF_TIMINGS 0x10 // 0x20
#define WARNING_INTERVAL 0.8
#define DELAY_CYCLE_MS 60000
#define DELAY_CYCLE_MINS 1 //DELAY_CYCLE_MS/1000/60
#define MINUTES_PER_HOUR 60
#define HIGH_CHECKSUM_ADRESS 500
#define LOW_CHECKSUM_ADRESS 1000

#define START_SAVE_ADRESS 0
//#define UPPER_OFFSET 512

#define PIN_INTERNAL_LED 13
#define PIN_VOLTAGE_SENSE 3
#define PIN_3_INTERRUPT 1
#define PIN_SEND_RESET 5
/*
Protocol defines
*/
// Unit ID:s
#define ID_INTERFACE 0x00
#define ID_TIMER 0x10
#define ID_GYRO 0x20

// Protocol stream reading states
#define READ_TO_BYTE 1
#define READ_FROM_BYTE 2
#define READ_MESSAGE_TYPE 3
#define READ_DATA_LENGTH 4
#define READ_DATA 5
#define READ_CRC_BYTE_1 6
#define READ_CRC_BYTE_2 7
#define READ_EOF 8

// Process frame returns
#define RUNTIME_CHANGED 0x00
#define STOPTIME_CHANGED 0x01
#define FAILED_DECODING_FRAME 0xFF
#define FAILED_READING_FRAME 0xFE
#define BUFFERING_FRAME 0xFD

//
#define TIMER_DATA_FIELD_STOP 0x00
#define TIMER_DATA_FIELD_ELAPSED 0x01

// Bitmasks
#define ID_TYPE_MASK 0xF0
#define ID_SUBUNIT_MASK 0x0F

// Inbuffer size
#define PROTOCOL_DATA_BUFFER_SIZE 24

// Mesage types
#define MSG_FULL_TIMER_FEFRESH 0x01

// Protocol special characters
#define ESCAPE 0xF0
#define START_OF_FRAME 0xFF
#define END_OF_FRAME 0xFE

/*
Debug flags
*/
#define DEBUG 0

/*
Defines of lsb and msb for reading and writing to EEPROM and serial.
*/

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

  typedef union {
    float floatingPoint;
    uint8_t binary[4];
  } binaryFloat;

  typedef union {
    uint16_t integer;
    uint8_t binary[4];
  } binaryInteger;

  typedef struct MCProxyProtocol {
    uint8_t incomingByte;
    uint8_t previous_incoming;
    uint8_t protocolState;
    uint8_t messageType;
    uint8_t fromByte;
    uint8_t toByte;
    uint8_t datasize;
    uint8_t data[PROTOCOL_DATA_BUFFER_SIZE];
    uint8_t crc_1;
    uint8_t crc_2;

    uint8_t data_read_posion;

  } MC_PROXY_PROTOCOL;

  typedef struct ll{  
    uint16_t runTime; //In minutes since last reset
    uint16_t stopTime; //In minutes
  } TIMING;

  TIMING timings[NUMBER_OF_TIMINGS];
  TIMING * current = timings;
  MC_PROXY_PROTOCOL in_data;
  boolean doService;
  boolean planService;
  boolean write_to_upper = false;

  uint16_t accumDelay; // Accumulated delay
  uint8_t eeprom_checksum;

  binaryInteger runTime; //In minutes since last reset
  binaryInteger stopTime; //In minutes


/*
  Function declarations
*/
void setup();
void change_timing(uint16_t id, uint16_t runTime, uint16_t stopTime);
void save_timings_to_eeprom(uint16_t * adress, TIMING * input);
void read_state_from_eeprom(uint16_t * adress); 

void increase_all_timings(TIMING * timings);
void broadcast_all_timings(TIMING * timings);

uint8_t calc_parity_value(TIMING * input);

void mcProxySend(binaryInteger bi);
void mcProxySend(binaryFloat bf);
void mcProxySend(uint8_t bb);

void mcProtocolInit(MC_PROXY_PROTOCOL * protocol);
uint8_t process_frame(MC_PROXY_PROTOCOL * protocol);
uint8_t buffer_protocol(MC_PROXY_PROTOCOL * protocol, uint8_t incoming);
/*
Interrupt related functions
*/
void power_off();
void power_back();

void serial_in();

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
    doService = false;
    planService = false;
  
  
    // Delay loop for the expected time
    while (millis()-lastSave<DELAY_CYCLE_MS) {
      if (Serial.available()){
          uint8_t buffer_result = buffer_protocol(&in_data, Serial.read());          
          // fay of the changes are updating, resend the data to the serial
          if (buffer_result == STOPTIME_CHANGED || buffer_result == RUNTIME_CHANGED){
            broadcast_all_timings(timings);
          };
    } else {
      
    }

  }

  lastSave = millis();
  increase_all_timings(&timings[0]);
  broadcast_all_timings(&timings[0]);


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
    digitalWrite(PIN_SEND_RESET, HIGH);
    wdt_disable();

    // Set the interrupt for voltage drop on the optocoupler
    attachInterrupt(PIN_3_INTERRUPT, power_off, FALLING);

    Serial.begin(115200);
    Serial.write("Starting\n");

    //Just temporary initialization
  
  #if 0
  for (int i = 0; i < NUMBER_OF_TIMINGS; i ++) {
    current = &timings[i];
    current->runTime = 0;
    current->stopTime = (100*MINUTES_PER_HOUR)+i; // 100 hours
  }
  #else
    read_state_from_eeprom((uint16_t*) START_SAVE_ADRESS);
    broadcast_all_timings(timings);
  #endif
  }

  void change_timing(uint16_t id, uint16_t runTime, uint16_t stopTime) {
    timings[id].runTime = runTime;
    timings[id].stopTime = stopTime;
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

  }


void broadcast_all_timings(TIMING * timings){
    for (int i = 0; i < NUMBER_OF_TIMINGS; i ++){
      //TIMING current = &timings[i];

      // Pack into bytes for serializaition
//      runTime.integer = current->runTime;
//      stopTime.integer = current->stopTime;

      runTime.integer =  timings[i].runTime;
      stopTime.integer = timings[i].stopTime;


      //if (Serial){
      //Serial.println(current->runTime);
      
      //Serial.write((byte)START_OF_FRAME); //Start of frame
      uint8_t fromAddress;
      uint8_t toAddress;
      fromAddress = i | ID_TIMER;
      toAddress = 0 | ID_INTERFACE;

      mcProxySend(toAddress); // To ID first interface
      mcProxySend(fromAddress); // From ID first timer
      mcProxySend(MSG_FULL_TIMER_FEFRESH); // Message type
      mcProxySend(sizeof(runTime.binary) + sizeof(stopTime.binary)); // 32 dec Bytes in payload
      mcProxySend(runTime); // Payload 2 first bytes
      mcProxySend(stopTime); // Payload 2 second bytes
      mcProxySend(0x02);
      mcProxySend(0x02);
      Serial.write((uint8_t)END_OF_FRAME); //End of frame
  }
}


  void read_state_from_eeprom(uint16_t * adress) {
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

  uint8_t calc_parity_value(TIMING * input) {
    return  lowByte(input->stopTime) ^ 
            highByte(input->stopTime) ^ 
            lowByte(input->runTime) ^
            highByte(input->stopTime);
  }

  void power_off(){
    #if DEBUG
    Serial.write("Saving");
    #endif
    Serial.write("Saving");
    // Stop the power down interrupt from recurring.
    detachInterrupt(PIN_3_INTERRUPT);
    

    noInterrupts();
    //save_timings_to_eeprom(START_SAVE_ADRESS, (TIMING *) &timings[0]);
    interrupts();

    digitalWrite(13, HIGH);
    attachInterrupt(PIN_3_INTERRUPT, power_back, RISING);
    // wdt_enable(WDTO_8S);
    //while(1){delay(100);};
  }

  void power_back() {
    Serial.write("Resetting...");
    digitalWrite(13, LOW);

     detachInterrupt(PIN_3_INTERRUPT);
     //attachInterrupt(PIN_3_INTERRUPT, power_off, FALLING);
     //wdt_enable(WDTO_4S);
     asm volatile ("  jmp 0");
  }

  void serial_in(){
    //digitalWrite(13, HIGH);
    delay(500);
    //digitalWrite(13, LOW);
  }

void mcProtocolInit(MC_PROXY_PROTOCOL * protocol) {
  protocol->protocolState = READ_TO_BYTE;
  protocol->incomingByte = NULL;
  protocol->messageType = NULL;
  protocol->datasize = NULL;
  protocol->fromByte = NULL;
  protocol->toByte = NULL;
  protocol->data_read_posion = 0;
  for (uint8_t i = 0; i < PROTOCOL_DATA_BUFFER_SIZE; i ++) protocol->data[i]=NULL;
}


uint8_t buffer_protocol(MC_PROXY_PROTOCOL * protocol, uint8_t incoming) {

        protocol->previous_incoming = protocol->incomingByte; 
        protocol->incomingByte = incoming;
        uint8_t result = BUFFERING_FRAME;

        // Protocol handling
        switch (protocol->protocolState) {
          case READ_TO_BYTE:
            protocol->toByte = protocol->incomingByte;

            if ( (protocol->incomingByte & ID_TYPE_MASK) == ID_TIMER)
               {
              protocol->protocolState = READ_FROM_BYTE;
              } else if ( (protocol->incomingByte & ID_TYPE_MASK) == ID_GYRO) {
              protocol->protocolState = READ_FROM_BYTE;
              } else {            
                protocol->protocolState = READ_TO_BYTE;
                mcProtocolInit(&in_data);
              }

          break;

          case READ_FROM_BYTE:
            protocol->fromByte = protocol->incomingByte;
            protocol->protocolState = READ_MESSAGE_TYPE;
          break;

          case READ_MESSAGE_TYPE:
            protocol->messageType = protocol->incomingByte;
            protocol->protocolState = READ_DATA_LENGTH;
          break;

          case READ_DATA_LENGTH:
            protocol->datasize = protocol->incomingByte;
            // Check if required datasize is within allowed limits
            if (protocol->datasize < 1 || protocol->datasize > PROTOCOL_DATA_BUFFER_SIZE) {
              mcProtocolInit(&in_data);
            } else {
              protocol->protocolState = READ_DATA;
            }

          break;

          case READ_DATA:
            // Fill the protocol array with data to appropriate size.
            if (protocol->data_read_posion < protocol->datasize) {

              protocol->data[protocol->data_read_posion] = protocol->incomingByte;
              protocol->data_read_posion ++;
              break;

            } else {
              protocol->protocolState = READ_CRC_BYTE_1;
            }
            
          // NB, no break here!

          case READ_CRC_BYTE_1:
            protocol->crc_1 = protocol->incomingByte;
            protocol->protocolState = READ_CRC_BYTE_2;
          break;

         case READ_CRC_BYTE_2:
            protocol->crc_2 = protocol->incomingByte;
            protocol->protocolState = READ_EOF;
         break;

         case READ_EOF:
            // If last byte was EOF, process frame and reset. Else just reset.
            if (protocol->incomingByte == (uint8_t) END_OF_FRAME) {
              result = process_frame(&in_data);
              mcProtocolInit(&in_data);
            } else {
              #if DEBUG
              Serial.write("Failed reading frame");
              Serial.write(END_OF_FRAME);
              # endif
              result = FAILED_READING_FRAME;
              mcProtocolInit(&in_data);
            }
         break;

         default:
            protocol->protocolState = READ_TO_BYTE;
         break;
      }
      return result;
}

uint8_t process_frame(MC_PROXY_PROTOCOL * protocol) {
  uint8_t result;
  // Check crc here!
  // If it is sent to timer...
  if ((protocol->toByte & ID_TYPE_MASK) == ID_TIMER)
      {
          // If data is adressed to the stopfield...
          if (protocol->data[0] == TIMER_DATA_FIELD_STOP) {
          // Reassembling the two bytes to an 16 bit int.  
            timings[protocol->toByte & ID_SUBUNIT_MASK].stopTime = (uint16_t) (protocol->data[2] | ((uint16_t)protocol->data[1] << 8));           
            result = STOPTIME_CHANGED;
            #if DEBUG
              Serial.write("Changing stoptime");
              Serial.print(protocol->toByte & ID_SUBUNIT_MASK);
              Serial.write(" To: ");
              Serial.println( );
              Serial.write(END_OF_FRAME);
            #endif
          }
          else if (protocol->data[0] == TIMER_DATA_FIELD_ELAPSED){
            #if DEBUG
              Serial.write("Changing timing: ");
              Serial.print(protocol->toByte & ID_SUBUNIT_MASK);
              Serial.write(" To: ");
              Serial.println( );
              Serial.write(END_OF_FRAME);
            #endif
            result = RUNTIME_CHANGED;
            timings[protocol->toByte & ID_SUBUNIT_MASK].runTime = (uint16_t) (protocol->data[2] | ((uint16_t)protocol->data[1] << 8));            
          } else 
            #if DEBUG
            Serial.write("Nothing...");
            Serial.write(END_OF_FRAME);
            #endif
            result = FAILED_DECODING_FRAME;
          return result;
      }

}

void mcProxySend(uint8_t bb){
  if (bb >= ESCAPE ) {
    Serial.write(ESCAPE);
    Serial.write(bb);
  } else {
    Serial.write(bb);
  }
}

void mcProxySend(binaryInteger bi){
  for (uint8_t i = 0; i < 4 ; i ++) {
    mcProxySend(bi.binary[i]);
  }
}

void mcProxySend(binaryFloat bf){
  for (uint8_t i = 0 ; i < 4 ; i ++) {
    mcProxySend(bf.binary[i]);
  }
}
