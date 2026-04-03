import os
import struct
import serial

MESSAGE_TOKEN = 0x23
CHANGE_BANK_REQUEST = 0X43
BOOT_UPDATE_REQUEST = 0X42
FIRMWARE_VERSION_REQUEST = 0X41

BOOT_ACK = 0x5f
BOOT_NACK = 0xaa


def compute_crc(buff, length):
    crc = 0xFFFFFFFF

    for byte in buff[0:length]:
        crc = crc ^ (byte)
        for i in range(32):
            if crc & 0x80000000:
                crc = (crc << 1) ^ 0x04C11DB7
            else:
                crc = (crc << 1)

    return crc & 0xFFFFFFFF


def send_data_serial(serial_com, data):
    data = data + (struct.pack("<I", compute_crc(data, len(data))))
    serial_com.write(data)

##-----------------

#how we connect the stm to the computer
comport = "0x14410000"
serial_com = serial.Serial(port=comport, baudrate=115200, timeout=15)

message = "hello world"

#convert the message to bytes
message = bytes(message, 'utf-8')

while(True):
    command = int(input("enter the comm number: (1,2,3 for update, change, or check):"))
    if command == 1:
        send_data_serial(serial_com, message)
    
    else:
        break
