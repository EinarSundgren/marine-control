import time
import serial

ser = serial.Serial('/dev/ttyACM0', 50, timeout=1)
for x in range(0, 1000):
	#print "We're on time %d" % (0xFFFF)
	#ser.write(['\x00','\x04','\x00']);
	input = '00'    
	ser.write(input.decode("hex"))

#ser = serial.Serial(0)
#ser.setRTS(False)
#time.sleep(3)
#ser.setRTS(True)
#time.sleep(3)
#ser.setRTS(False)