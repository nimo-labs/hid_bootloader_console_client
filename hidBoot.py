#!/usr/bin/python3

import usb.core
import usb.util
import hidBlProtocol
import sys
import time

from enum import Enum


class Command(Enum):
    NONE = 1
    LIST = 2
    ERASE = 3
    WRITE = 4
    DEV_VER = 5


usbBuf = bytearray(64)


def READ_RETRIES():
    return 2


def usbSendRecv(ep, sendBuf):
    oldCmd = sendBuf[0]
    recvBuf = bytearray(64)
    retries = READ_RETRIES()

    ep.write(sendBuf)

    waiting = 1
    sendBuf[0] = 0
    while waiting:
        recvBuf = dev.read(0x81, 64)
        if recvBuf[0] > 0:
            waiting = 0
            print(recvBuf)
    return recvBuf

    # while retries > 0:
    #     try:
    #         sendBuf[0] = 0
    #         recvBuf = dev.read(0x81, 64, 10000)
    #         print("Success")
    #         # print(recvBuf)
    #         return recvBuf
    #     except:
    #         print("timeout")
    #         retries = retries - 1
    #         sendBuf[0] = oldCmd
    #         ep.write(sendBuf)


def printUsage():
    print("Usage:\n")
    print("hidBoot cmd device mem_loc filename\n")
    print("Where:")
    print("cmd is one of")
    print("             d (device bootloader version)")
    print("             e (erase internal flash)")
    print("             l (list supported controllers)")
    print("             w (Write)")
    print("device is the microcontroller part number.")
    print("mem_loc is the start location in hex (e.g. 0x1000)")
    print("filename is the firmware filename\n")
    exit(1)


# ########### MAIN ##############
if len(sys.argv) >= 2:
    if 'l' == sys.argv[1]:
        command = Command.LIST
        print("Supported controllers:\n")
        print("m032lg6ae - Nuvoton M032LG6AE")
        print("\n")
        exit(0)
    elif 'e' == sys.argv[1]:
        command = Command.ERASE
    elif 'w' == sys.argv[1]:
        if len(sys.argv) < 5:
            print("Error, too few arguments for command.\n")
            printUsage()
        command = Command.WRITE
    elif 'd' == sys.argv[1]:
        command = Command.DEV_VER
        print("Checking firmware bootloader version:\n")
    else:
        printUsage()
else:
    printUsage()

# find our device
dev = usb.core.find(idVendor=0x0416, idProduct=0x5020)

# was it found?
if dev is None:
    raise ValueError('Device not found')

if dev.is_kernel_driver_active(0):
    try:
        dev.detach_kernel_driver(0)
        print("kernel driver detached")
    except usb.core.USBError as e:
        sys.exit("Could not detach kernel driver: ")
# else:
    # print("no kernel driver attached")


dev.set_configuration()

# get an endpoint instance
cfg = dev.get_active_configuration()
intf = cfg[(0, 0)]

ep = usb.util.find_descriptor(
    intf,
    # match the first OUT endpoint
    custom_match=lambda e: \
    usb.util.endpoint_direction(e.bEndpointAddress) == \
    usb.util.ENDPOINT_OUT)

assert ep is not None


if Command.DEV_VER == command:
    hidBlProtocol.hidBlProtocolEncodePacket(
        0, 0, hidBlProtocol.Packet.GET_BL_VER, '', 0, 0, usbBuf)
    usbBuf = usbSendRecv(ep, usbBuf)
    print("Version: %d.%d" % (usbBuf[7], usbBuf[6]))

elif Command.ERASE == command:
    hidBlProtocol.hidBlProtocolEncodePacket(
        0, 0, hidBlProtocol.Packet.ERASE_INT_FLASH, '', 0, 0, usbBuf)
    usbBuf = usbSendRecv(ep, usbBuf)
    if hidBlProtocol.Packet.ACK.value != usbBuf[0]:
        print("Erase failed")

elif Command.WRITE == command:
    try:
        f = open(sys.argv[4], "rb")
        image = f.read()
        f.close()
    except:
        print("File not found!")
    print(image)
    print("Len: ", len(image))
    filePtr = 0
    while filePtr < len(image):
        print('')
        if (len(image) - filePtr) >= 32:
            segLen = 32
        else:
            segLen = len(image) - filePtr
        hidBlProtocol.hidBlProtocolEncodePacket(
            0, int(sys.argv[3], 16) + filePtr, hidBlProtocol.Packet.WRITE_INT_FLASH, image, filePtr, segLen, usbBuf)
        filePtr = filePtr + 32
        usbBuf = usbSendRecv(ep, usbBuf)
        if hidBlProtocol.Packet.ACK.value != usbBuf[0]:
            print("Write failed")
