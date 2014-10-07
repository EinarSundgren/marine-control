#include <Arduino.h>
#include <Serial.h>
#include "include/timing/timings_core.h"
#include "include/proxy/mproxy_protocol.h"
#include "include/eeprom/eeprom_core.h"

void broadcast_all_timings(TIMING * timings){
      int i;
      uint8_t parity = 0;
      binaryInteger runTime; //In minutes since last reset
      binaryInteger stopTime; //In minutes


    for (i = 0; i < NUMBER_OF_TIMINGS; i ++){

      // Empty the union
      runTime.binary[0]=0;
      runTime.binary[1]=0;
      runTime.binary[2]=0;
      runTime.binary[3]=0;
      stopTime.binary[0]=0;
      stopTime.binary[1]=0;
      stopTime.binary[2]=0;
      stopTime.binary[3]=0;

      // Fill it with the new curent values
      runTime.integer = timings[i].runTime;
      stopTime.integer = timings[i].stopTime;
      
      uint8_t fromAddress;
      uint8_t toAddress;
      fromAddress = i | ID_TIMER;
      toAddress = 0 | ID_INTERFACE;


      // Actual sending of the values.
      Serial.write((uint8_t)START_OF_FRAME); //Start of frame
      mcProxySendByte(toAddress); // To ID first interface
      mcProxySendByte(fromAddress); // From ID first timer
      mcProxySendByte(MSG_FULL_TIMER_FEFRESH); // Message type
      mcProxySendByte(sizeof(runTime.binary) + sizeof(stopTime.binary)); // 32 dec Bytes in payload
      mcProxySendInt(runTime); // Payload 2 first bytes
      mcProxySendInt(stopTime); // Payload 2 second bytes
      mcProxySendByte(
                              toAddress ^ 
                              fromAddress ^ 
                              MSG_FULL_TIMER_FEFRESH ^ 
                              (sizeof(runTime.binary) + sizeof(stopTime.binary)) ^ 
                              calc_parity_value(&timings[i])
                              );
      mcProxySendByte(0x02);
      Serial.write((uint8_t)END_OF_FRAME); //End of frame
  }
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

//digitalWrite(13, HIGH);
        protocol->previous_incoming = protocol->incomingByte; 
        protocol->incomingByte = incoming;
        uint8_t result = BUFFERING_FRAME;

        // Protocol handling
        switch (protocol->protocolState) {
          case READ_TO_BYTE:


            protocol->toByte = protocol->incomingByte;

            if ( (protocol->incomingByte & ID_TYPE_MASK) == ID_TIMER)
               {
              #if DEBUG
              Serial.write("From UI to timer.");
              Serial.write(END_OF_FRAME);
              # endif
              protocol->protocolState = READ_FROM_BYTE;
              } else if ( (protocol->incomingByte & ID_TYPE_MASK) == ID_GYRO) {
              protocol->protocolState = READ_FROM_BYTE;
              } else {            
                protocol->protocolState = READ_TO_BYTE;
                mcProtocolInit(protocol);
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
              mcProtocolInit(protocol);
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
              result = FULL_FRAME_BUFFERED;
              // Assuming that the next function takes care of clearing the protocol buffer


            } else {
              #if DEBUG
              Serial.write("Failed reading frame");
              Serial.write(END_OF_FRAME);
              # endif
              result = FAILED_READING_FRAME;
              mcProtocolInit(protocol);
            }
         break;

         default:
            protocol->protocolState = READ_TO_BYTE;
         break;
      }
      return result;
}

uint8_t process_frame(MC_PROXY_PROTOCOL * protocol, TIMING * timings) {
  digitalWrite(13, LOW);
  uint8_t result;
  // Check crc here!
  // If it is sent to timer...

  switch (protocol->toByte & ID_TYPE_MASK) {
  
  case ID_TIMER: 
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
            }
          break;

        case ID_ECHO:
          //binaryInteger b_int;
          //binaryFloat b_float; 
          
          result = ECHO_DATA;          
        

        break;

          default: 
            #if DEBUG
            Serial.write("Nothing...");
            Serial.write(END_OF_FRAME);
            #endif
            result = FAILED_DECODING_FRAME;
          break;
        }

          return result;
    

}



void mcProxySendByte(uint8_t bb){
  if (bb >= ESCAPE ) {
    Serial.write(ESCAPE);
    Serial.write(bb);
  } else {
    Serial.write(bb);
  }
}

void mcProxySendInt(binaryInteger bi){
  for (uint8_t i = 0; i < 4 ; i ++) {
    mcProxySendByte(bi.binary[i]);
  }
}

void mcProxySendFloat(binaryFloat bf){
  for (uint8_t i = 0 ; i < 4 ; i ++) {
    mcProxySendByte(bf.binary[i]);
  }
}