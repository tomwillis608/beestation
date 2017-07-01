#!/usr/bin/python
import piVirtualWire.piVirtualWire as piVirtualWire
import time
import pigpio
import struct
import requests
import logging
import logging.handlers
LOG_FILENAME =  '/tmp/rftest.log'

def calcChecksum(packet):
    checkSum = sum([ int(i) for i in packet[:13]])
    return checkSum % 256

def sendToWeb(url):
    print ('URL ', url)
    r = requests.get(url)
    return r.status_code

if __name__ == "__main__":

    pi = pigpio.pi()
    rx = piVirtualWire.rx(pi, 4, 2000) # Set pigpio instance, TX module GPIO pin and baud rate

#    logging.basicConfig(filename=LOG_FILENAME, level=logging.DEBUG)
    logger = logging.getLogger('RFTLogger')
    logger.setLevel(logging.DEBUG)
    handler = logging.handlers.RotatingFileHandler(LOG_FILENAME,
                                      maxBytes=100000, backupCount=10)
    logger.addHandler(handler)
    # create console handler and set level to debug
    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)
    # create formatter
    formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
    # add formatter to ch
    ch.setFormatter(formatter)
    handler.setFormatter(formatter)
    # add ch to logger
    logger.addHandler(ch)

    fmt = "<LLLBB"
    print ("Top of RF test")
    logger.info('Top of RF test')
    while True:

            while rx.ready():
                packet = rx.get()
                if (14 == len(packet)):
                    print(packet)
                    id = packet[0] + packet[1]*256 + packet[2]*256*256 + packet[3]*256*256*256
                    data = packet[4] + packet[5]*256 + packet[6]*256*256 + packet[7]*256*256*256
                    packetCount = packet[8] + packet[9]*256 + packet[10]*256*256 + packet[11]*256*256*256
                    dataType = packet[12]
                    checksum = packet[13]
                    print ('Id: ', format(id,'X'))
                    print ('Data: ', format(data,'d'))
                    print ('Packet count: ', format(packetCount,'d'))
                    print ('Data type: ', format(dataType,'c'))
                    print ('Checksum: ', format(checksum,'X'))
                    calculatedChecksum = calcChecksum(packet)
                    print ('Calc Checksum: ', format(calculatedChecksum,'X'))
                    logger.debug('id=%X,data=%d,count=%d,type=%c,chk=%X,calc=%X' % 
                         (id, data, packetCount, dataType, checksum, calculatedChecksum)) 		
                    if (calculatedChecksum == checksum):
                        print ('Upload to server')
                        idx = format(id, 'X')
                        if (ord('+') == dataType):
                            if (0 == data):
                                url = "http://192.168.0.163/add_status.php?station=%X&message=Starting" %(id)
                                sendToWeb(url)
                            else :
                                url = "http://192.168.0.163/add_status.php?station=%X&message=%d cycles" %(id, data)
                                sendToWeb(url)
                        else:
                            url = "http://192.168.0.163/add_record.php?s=%X&r=%d&t=%c&n=%d" % (id, data, dataType, packetCount)
                            sendToWeb(url)
                        logger.debug('url=%s' % url)
                    time.sleep(0.01)

    rx.cancel()
    pi.stop()
