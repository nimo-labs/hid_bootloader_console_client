/*
 * This file is part of the hid_bootloader_console_client distribution
 * (https://github.com/nimo-labs/hid_bootloader_console_client).
 * Copyright (c) 2020 Nimolabs Ltd. www.nimo.uk
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef HID_BL_PROTOCOL_H
#define HID_BL_PROTOCOL_H

#define HID_BL_PROTOCOL_MAX_PACKET_SIZE 64
#define HID_BL_PROTOCOL_PACKET_HEADER_SIZE 8
#define HID_BL_PROTOCOL_DATA_SIZE HID_BL_PROTOCOL_MAX_PACKET_SIZE - HID_BL_PROTOCOL_PACKET_HEADER_SIZE

/*Packet types */
#define HID_BL_PROTOCOL_ACK 0x01
#define HID_BL_PROTOCOL_NAK 0x02
#define HID_BL_PROTOCOL_APP_GET_VER 0x03
#define HID_BL_PROTOCOL_APP_SEND_VER 0x04
#define HID_BL_PROTOCOL_BL_GET_VER 0x05
#define HID_BL_PROTOCOL_BL_SEND_VER 0x06
#define HID_BL_PROTOCOL_ERASE_INT_FLASH 0x07
#define HID_BL_PROTOCOL_WRITE_INT_FLASH 0x08
#define HID_BL_PROTOCOL_READ_INT_FLASH 0x09
#define HID_BL_PROTOCOL_VERIFY_INT_FLASH 0x0A
#define HID_BL_PROTOCOL_ERASE_EXT_FLASH 0x0B
#define HID_BL_PROTOCOL_READ_EXT_FLASH 0x0C
#define HID_BL_PROTOCOL_WRITE_EXT_FLASH 0x0D
#define HID_BL_PROTOCOL_VERIFY_EXT_FLASH 0x0E
#define HID_BL_PROTOCOL_COPY_EXT_TO_INT 0x0F
#define HID_BL_PROTOCOL_RUN_INT 0x10
#define HID_BL_PROTOCOL_GET_MFR_ID 0x11
#define HID_BL_PROTOCOL_GET_PART_ID 0x12
#define HID_BL_PROTOCOL_SEND_MFR_ID 0x13
#define HID_BL_PROTOCOL_SEND_PART_ID 0x14
/*******************************************/

/*Board IDs (Only used by USB hosts) */
#define MFR_NUVOTON 0x000000DA
#define PART_M032LG6AE 0x01132601

/*******************************************/

struct __attribute__((packed)) hidBlProtocolPacket_s
{
    unsigned char packetType;
    unsigned char dataLen;
    unsigned long address;
    unsigned char data[HID_BL_PROTOCOL_DATA_SIZE];
};

void hidBlProtocolDeSerialisePacket(struct hidBlProtocolPacket_s *packet, const unsigned char *rawData);
char hidBlProtocolSerialisePacket(struct hidBlProtocolPacket_s *packet, unsigned char *serialPacket, unsigned char buffLen);
char hidBlProtocolEncodePacket(struct hidBlProtocolPacket_s *packet, unsigned long address, unsigned char packetType, const unsigned char *data, unsigned char dataLen);

#endif // HID_BL_PROTOCOL_H
