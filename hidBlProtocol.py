from enum import Enum


def HID_BL_PROTOCOL_MAX_PACKET_SIZE():
    return 64


def HID_BL_PROTOCOL_PACKET_HEADER_SIZE():
    return 8


def HID_BL_PROTOCOL_DATA_SIZE():
    return HID_BL_PROTOCOL_MAX_PACKET_SIZE() - HID_BL_PROTOCOL_PACKET_HEADER_SIZE()


# Packet types
class Packet(Enum):
    ACK = 0x01
    NAK = 0x02
    APP_GET_VER = 0x03
    APP_SEND_VER = 0x04
    GET_BL_VER = 0x05
    SEND_BL_VER = 0x06
    ERASE_INT_FLASH = 0x07
    WRITE_INT_FLASH = 0x08
    READ_INT_FLASH = 0x09
    VERIFY_INT_FLASH = 0x0A
    ERASE_EXT_FLASH = 0x0B
    READ_EXT_FLASH = 0x0C
    WRITE_EXT_FLASH = 0x0D
    VERIFY_EXT_FLASH = 0x0E
    COPY_EXT_TO_INT = 0x0F
    RUN_INT = 0x10
    GET_MFR_ID = 0x11
    GET_PART_ID = 0x12
    SEND_MFR_ID = 0x13
    SEND_PART_ID = 0x14


hidBlProtocolPacket_s = {
    'packetType': 0x00,
    'dataLen': 0x00,
    'address': 0x00000000,
    'data': bytearray(HID_BL_PROTOCOL_DATA_SIZE())
}


def hidBlProtocolEncodePacket(self, address: int, packetType: int, data: bytearray, fileOffset: int, dataLen: int, buffer: bytearray):
    # print("Address: ", hex(address))
    # print("Pkt type: ", packetType)
    # print("dataLen: ", dataLen)

    i = 0
    # while i < dataLen:
    #     print(hex(data[i+fileOffset]), end='')
    #     print(' ', end='')
    #     i = i + 1
    # print('')

    addressBytes = address.to_bytes(4, 'big')

    buffer[0] = packetType.value
    buffer[1] = dataLen
    buffer[2] = addressBytes[0]
    buffer[3] = addressBytes[1]
    buffer[4] = addressBytes[2]
    buffer[5] = addressBytes[3]

    i = 0
    if dataLen > 0:
        while i < dataLen:
            buffer[i+6] = data[i+fileOffset]
            #print(str(i) + ' ', end='')
            i = i + 1
        # print('')
