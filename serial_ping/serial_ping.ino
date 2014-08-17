/*

ID to 8 bits
  Unit type 4 bits
  Unit sub ID 4 bits

ID from 8 bits
  Unit type 4 bits leftmost
  Unit sub ID 4 bits

Message type 8 bits
  Full timer refresh 0x01 
      predefined values are sent 
  
  Change single request 0x02
      First byte denotes which value to change
      Following predefined bytes the value

Data length 8 bits

Payload 

CRC
End of frame Unescaped

Unit codes first 4 bits 
    0x00 = user interface
    0x10 = timer
    0x20 = gyro
    0x30 = accelerometer
    0x40 = magnetometer
    0x50 = barometric preassure
    0x60 = hygrometer

Unit codes 4 lsb bits 
  Unit sub ID 


0xFF = Start of frame
0xFE = escape character


*/
#define ESCAPE 0xFE
#define START_OF_FRAME 0xFF
#define END_OF_FRAME 0xFD
typedef union {
  float floatingPoint;
  byte binary[4];
} binaryFloat;

typedef union {
  uint16_t integer;
  byte binary[4];
} binaryInteger;

/*
  binaryFloat hi;
  hi.floatingPoint = -11.7;
  Serial.begin(115200);
  Serial.write(hi.binary,4);
*/

void mcProxySend(binaryInteger bi);
void mcProxySend(binaryFloat bf);
void mcProxySend(byte bb);

binaryInteger runTime1; //In minutes since last reset
    binaryInteger stopTime1; //In minutes

    binaryInteger runTime2; //In minutes since last reset
    binaryInteger stopTime2; //In minutes

    binaryFloat acc1; 
    binaryFloat acc2; 
    binaryFloat acc3; 

void setup() {
  Serial.begin(115200);
      runTime1.integer = 34*60; //In minutes since last reset
    stopTime1.integer = 100*60; //In minutes

    runTime2.integer = 62*60; //In minutes since last reset
    stopTime2.integer = 200*60; //In minutes
}

    


   /*
    float * gyro1;
    float * gyro2;
    float * gyro3;

    gyro1 * = 1.5;
    gyro2 * = 2.5;
    gyro3 * = 3.5;

    float magnetometer1 = 7.00;
    float magnetometer2 = 8.00;
    float magnetometer3 = 9.00;

    float barometer = 10.0;
*/
    uint16_t crc = 02;



void loop() {
    acc1.floatingPoint = 1.0;
    acc2.floatingPoint = 2.0;
    acc3.floatingPoint = 3.0;

    runTime1.integer ++;
    runTime2.integer ++;

  Serial.write((byte)START_OF_FRAME); //Start of frame
  mcProxySend(0x00); // To ID first interface
  mcProxySend(0x10); // From ID first timer
  mcProxySend(0x01);
  mcProxySend(sizeof(runTime1.binary) + sizeof(stopTime1.binary)); // 32 dec Bytes in payload
  mcProxySend(runTime1); // Payload 2 first bytes
  mcProxySend(stopTime1); // Payload 2 second bytes
  mcProxySend(crc);
  mcProxySend(crc);

  Serial.write((byte)START_OF_FRAME); //Start of frame
  mcProxySend(0x00); // To ID first interface
  mcProxySend(0x11); // From ID second timer
  mcProxySend(0x01);
  mcProxySend(sizeof(runTime2.binary) + sizeof(stopTime2.binary)); // 32 dec Bytes in payload
  mcProxySend(runTime2); // Payload 2 first bytes
  mcProxySend(stopTime2); // Payload 2 second bytes
  mcProxySend(crc);
  mcProxySend(crc);

  Serial.write((byte)START_OF_FRAME); //Start of frame
  mcProxySend(0x00); // To ID first interface
  mcProxySend(0x20); // From first gyro
  mcProxySend(0x01);
  mcProxySend(sizeof(runTime2.binary) + sizeof(stopTime2.binary)); // 32 dec Bytes in payload
  mcProxySend(runTime2); // Payload 2 first bytes
  mcProxySend(stopTime2); // Payload 2 second bytes
  mcProxySend(crc);
  mcProxySend(crc);
/*
  Serial.write(0xFF); //Start of frame
  Serial.write(0x00); // To ID first interface
  Serial.write(0x17); // From ID second timer
  Serial.write(0x20); // Bytes in payload
  Serial.write(runTime2.binary,4); // Payload 2 first bytes
  Serial.write(stopTime2.binary,4); // Payload 2 second bytes
  Serial.write(crc);

  */
/*
  Serial.write(0x00); // To ID first interface
  Serial.write(0x20); // From ID first gyro
  Serial.write(96); // Bytes in payload
  Serial.write(*gyro1); // Payload 2 first bytes
  Serial.write(*gyro2); // Payload 2 first bytes
  Serial.write(*gyro3); // Payload 2 first bytes
  Serial.write(crc);
*/

/*
  Serial.write(0xFF); //Start of frame
  Serial.write(0x00); // To ID first interface
  Serial.write(0x30); // From ID first accelerometer
  Serial.write(96); // Bytes in payload

  for (int i = 0; i < sizeof(acc1.binary)/sizeof(float); i ++) {
    if (acc1.binary[0] == START_OF_FRAME || * acc1.binary == ESCAPE) {Serial.write(ESCAPE);}
    Serial.write(acc1.binary[i]); // Payload 2 first bytes
  }
  
  for (int i = 0; i < sizeof(acc1.binary)/sizeof(float); i ++) {
    if (acc2.binary[0] == START_OF_FRAME || * acc2.binary == ESCAPE) {Serial.write(ESCAPE);}
    Serial.write(acc2.binary[i]); // Payload 2 first bytes
  }

  for (int i = 0; i < sizeof(acc1.binary)/sizeof(float); i ++) {
    if (acc3.binary[0] == START_OF_FRAME || * acc3.binary == ESCAPE) {Serial.write(ESCAPE);}
    Serial.write(acc3.binary[i]); // Payload 2 first bytes
  }

  Serial.write(crc);
*/
/*
  Serial.write(0x00); // To ID first interface
  Serial.write(0x40); // From ID first magneometer
  Serial.write(96); // Bytes in payload

  Serial.write(magnetometer1,4); // Payload 2 first bytes
  Serial.write(magnetometer2,4); // Payload 2 first bytes
  Serial.write(magnetometer3,4); // Payload 2 first bytes

  Serial.write(crc);

  Serial.write(0x00); // To ID first interface
  Serial.write(0x40); // From ID first magneometer
  Serial.write(32); // Bytes in payload
  Serial.write(barometer); // Payload 2 first bytes
  Serial.write(crc);


*/

  delay(500);
}

void mcProxySend(byte bb){
  if (bb >= ESCAPE ) {
    Serial.write(ESCAPE);
    Serial.write(bb);
  } else {
    Serial.write(bb);
  }
}

void mcProxySend(binaryInteger bi){
  for (byte i = 0; i < 4 ; i ++) {
    mcProxySend(bi.binary[i]);
  }
}

void mcProxySend(binaryFloat bf){
  for (byte i = 0 ; i < 4 ; i ++) {
    mcProxySend(bf.binary[i]);
  }
}