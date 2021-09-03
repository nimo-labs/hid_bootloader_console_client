#!/usr/bin/python3

import hidBlProtocol
import sys
import platform
from sys import exit
from enum import Enum
import subprocess
import sys

try:
    import usb.core
    import usb.util
except:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pyusb"])
    import usb.core
    import usb.util


class Command(Enum):
    NONE = 1
    LIST = 2
    ERASE = 3
    WRITE = 4
    DEV_VER = 5
    EXT_WRITE = 6
    EXT_READ = 7
    CP_EXT_INT = 8
    REBOOT_BOOTLOADER = 9


usbBuf = bytearray(64)


def READ_RETRIES():
    return 4


def usbSendRecv(ep, sendBuf):
    recvBuf = bytearray(64)
    retries = READ_RETRIES()

    ep.write(sendBuf)

    # RUN_INT is the only command not to ACK
    if hidBlProtocol.Packet.RUN_INT.value is not sendBuf[0]:
        while retries > 0:
            try:
                recvBuf = dev.read(0x81, 64, 5000)
               # print("Success")
                if hidBlProtocol.Packet.USB_WAIT.value == recvBuf[0]:
                    print("Waiting...")
                else:
                    return recvBuf
            except:
               # print("timeout")
                retries = retries - 1
                ep.write(sendBuf)


def printUsage():
    print("Usage examples:\n")
    print("Copy external flash to internal:")
    print("    hidBoot c\r\n")
    print("Device bootloader version:")
    print("    hidBoot d\r\n")
    print("Erase internal flash:")
    print("    hidBoot e\r\n")
    print("List supported controllers:")
    print("    hidBoot l\r\n")
    print("Reboot application into bootloader:")
    print("    hidBoot r\r\n")
    print("Write internal flash:")
    print("    hidBoot w 0x3000 blinky.bin\r\n")
    print("Write external flash:")
    print("    hidBoot x blinky.bin\r\n")
    exit(1)


# ########### MAIN ##############
if len(sys.argv) >= 2:
    if 'c' == sys.argv[1]:
        command = Command.CP_EXT_INT
    elif 'd' == sys.argv[1]:
        command = Command.DEV_VER
    elif 'e' == sys.argv[1]:
        command = Command.ERASE
    elif 'l' == sys.argv[1]:
        command = Command.LIST
        print("Supported controllers:\n")
        print("Nuvoton M032LG6AE")
        print("Atmel ATSAMD21E17A")
        print("\n")
        exit(0)
    elif 'r' == sys.argv[1]:
        command = Command.REBOOT_BOOTLOADER
    elif 'w' == sys.argv[1]:
        if len(sys.argv) < 4:
            print("Error, too few arguments for command.\n")
            printUsage()
        command = Command.WRITE
    elif 'x' == sys.argv[1]:
        if len(sys.argv) < 2:
            print("Error, too few arguments for command.\n")
            printUsage()
        command = Command.EXT_WRITE
    elif 'y' == sys.argv[1]:
        command = Command.EXT_READ
    else:
        printUsage()
else:
    printUsage()

# find our device
dev = usb.core.find(idVendor=0x0416, idProduct=0x5020)

# was it found?
if dev is None:
    print('Device not found')
    exit(1)

if "Linux" == platform.system():
    if dev.is_kernel_driver_active(0):
        try:
            dev.detach_kernel_driver(0)
            # print("kernel driver detached")
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
    print("Checking firmware bootloader version:\n")
    hidBlProtocol.hidBlProtocolEncodePacket(
        0, 0, hidBlProtocol.Packet.GET_BL_VER, '', 0, 0, usbBuf)
    usbBuf = usbSendRecv(ep, usbBuf)
    print("Version: %d.%d.%d" % (usbBuf[8], usbBuf[7], usbBuf[6]))
elif Command.CP_EXT_INT == command:
    print("Copying external flash to internal...", end='', flush=True)
    hidBlProtocol.hidBlProtocolEncodePacket(
        0, 0, hidBlProtocol.Packet.COPY_EXT_TO_INT, '', 0, 0, usbBuf)
    usbBuf = usbSendRecv(ep, usbBuf)
    if hidBlProtocol.Packet.ACK.value != usbBuf[0]:
        print("Copy failed")
    else:
        print("Done.")
        hidBlProtocol.hidBlProtocolEncodePacket(
            0, 0, hidBlProtocol.Packet.RUN_INT, '', 0, 0, usbBuf)
        usbBuf = usbSendRecv(ep, usbBuf)
elif Command.ERASE == command:
    print("Eraseing firmware...", end='', flush=True)
    hidBlProtocol.hidBlProtocolEncodePacket(
        0, 0, hidBlProtocol.Packet.ERASE_INT_FLASH, '', 0, 0, usbBuf)
    usbBuf = usbSendRecv(ep, usbBuf)
    if hidBlProtocol.Packet.ACK.value != usbBuf[0]:
        print("Erase failed")
    else:
        print("Done.")

elif Command.WRITE == command:
    try:
        f = open(sys.argv[3], "rb")
        image = f.read()
        f.close()
    except:
        print("File not found!")
 #   print(image)
 #   print("Len: ", len(image))
    print("Writing firmware")
    filePtr = 0
    progress = 0
    while filePtr < len(image):
        progress = (filePtr / len(image)) * 100
        print("Progress: " + "{:.0f}".format(progress) + "%    \r", end='')
        if (len(image) - filePtr) >= 32:
            segLen = 32
        else:
            segLen = len(image) - filePtr
        hidBlProtocol.hidBlProtocolEncodePacket(
            0, int(sys.argv[2], 16) + filePtr, hidBlProtocol.Packet.WRITE_INT_FLASH, image, filePtr, segLen, usbBuf)
        filePtr = filePtr + 32
        usbBuf = usbSendRecv(ep, usbBuf)
        if hidBlProtocol.Packet.ACK.value != usbBuf[0]:
            print("\nWrite failed, check address?")
    hidBlProtocol.hidBlProtocolEncodePacket(
        0, 0, hidBlProtocol.Packet.RUN_INT, '', 0, 0, usbBuf)
    usbBuf = usbSendRecv(ep, usbBuf)
    print('')

elif Command.EXT_WRITE == command:
    try:
        f = open(sys.argv[2], "rb")
        image = f.read()
        f.close()
    except:
        print("File not found!")
        exit(1)
 #   print(image)
 #   print("Len: ", len(image))
    print("Erasing ext flash")
    hidBlProtocol.hidBlProtocolEncodePacket(
        0, 0, hidBlProtocol.Packet.ERASE_EXT_FLASH, '', 0, 0, usbBuf)
    usbBuf = usbSendRecv(ep, usbBuf)
    print("Writing ext flash image")
    filePtr = 0
    progress = 0
    while filePtr < len(image):
        progress = (filePtr / len(image)) * 100
        print("Progress: " + "{:.0f}".format(progress) + "%    \r", end='')
        if (len(image) - filePtr) >= 32:
            segLen = 32
        else:
            segLen = len(image) - filePtr
        hidBlProtocol.hidBlProtocolEncodePacket(
            0, filePtr, hidBlProtocol.Packet.WRITE_EXT_FLASH, image, filePtr, segLen, usbBuf)
        filePtr = filePtr + 32
        usbBuf = usbSendRecv(ep, usbBuf)
        if hidBlProtocol.Packet.ACK.value != usbBuf[0]:
            print("\nWrite failed, check address?")
        # hidBlProtocol.hidBlProtocolEncodePacket(
            # 0, 0, hidBlProtocol.Packet.RUN_INT, '', 0, 0, usbBuf)
    usbBuf = usbSendRecv(ep, usbBuf)
    print('')
elif Command.EXT_READ == command:
    # try:
    #     f = open(sys.argv[4], "rb")
    #     image = f.read()
    #     f.close()
    # except:
    #     print("File not found!")

    print("Reading ext flash image")
    hidBlProtocol.hidBlProtocolEncodePacket(
        0, 0, hidBlProtocol.Packet.READ_EXT_FLASH, '', 0, 0, usbBuf)
    usbBuf = usbSendRecv(ep, usbBuf)
    print('Sent')
elif Command.REBOOT_BOOTLOADER == command:
    print("Rebooting into bootloader")
    hidBlProtocol.hidBlProtocolEncodePacket(
        0, 0, hidBlProtocol.Packet.REBOOT_BOOTLOADER, '', 0, 0, usbBuf)
    usbBuf = usbSendRecv(ep, usbBuf)
