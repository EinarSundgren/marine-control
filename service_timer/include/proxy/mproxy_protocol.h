#ifndef MPROXY_PROTOCOL_H
#define MPROXY_PROTOCOL_H


#include <Arduino.h>
#include "include/timing/timings_core.h"



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
#define FULL_FRAME_BUFFERED 0xFC

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

void broadcast_all_timings(TIMING * timings);

// Send and receive the datatypes through the proxy properly escaped.
void mcProxySendInt(binaryInteger bi);
void mcProxySendFloat(binaryFloat bf);
void mcProxySendByte(uint8_t bb);

// Init the protocol datastructure
void mcProtocolInit(MC_PROXY_PROTOCOL * protocol);

// Buffer incominf full frame
uint8_t buffer_protocol(MC_PROXY_PROTOCOL * protocol, uint8_t incoming);

//Act on complete frame
uint8_t process_frame(MC_PROXY_PROTOCOL * protocol, TIMING * timings);


#endif