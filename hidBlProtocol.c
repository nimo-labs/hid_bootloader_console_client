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

#include <stdio.h>
#include "hidBlProtocol.h"

void hidBlProtocolDeSerialisePacket(struct hidBlProtocolPacket_s *packet, const unsigned char *rawData)
{
    unsigned char i;
    packet->packetType = rawData[0];
    packet->dataLen = rawData[1];
    packet->address = (rawData[2] << 24) | (rawData[3] << 16) | (rawData[4] << 8) | rawData[5];
    for (i = 0; i < packet->dataLen; i++)
        packet->data[i] = rawData[i + 6];
}

char hidBlProtocolSerialisePacket(struct hidBlProtocolPacket_s *packet, unsigned char *serialPacket, unsigned char buffLen)
{
    unsigned char i;
    if (buffLen < (HID_BL_PROTOCOL_PACKET_HEADER_SIZE + HID_BL_PROTOCOL_DATA_SIZE))
    {
        printf("hidBlProtocolSerialisePacket: buffer too short!\r\n");
        return 1;
    }
    serialPacket[0] = packet->packetType;
    serialPacket[1] = packet->dataLen;
    serialPacket[2] = (packet->address >> 24) & 0xFF;
    serialPacket[3] = (packet->address >> 16) & 0xFF;
    serialPacket[4] = (packet->address >> 8) & 0xFF;
    serialPacket[5] = packet->address & 0xFF;
    for (i = 0; i < packet->dataLen; i++)
        serialPacket[i + 6] = packet->data[i];
    return 0;
}

char hidBlProtocolEncodePacket(struct hidBlProtocolPacket_s *packet, unsigned long address, unsigned char packetType, const unsigned char *data, unsigned char dataLen)
{
    unsigned char i;
    if (dataLen > HID_BL_PROTOCOL_DATA_SIZE)
    {
        printf("hidBlProtocolEncodePacket: data too long!\r\n");
        return 1;
    }
    packet->packetType = packetType;
    packet->dataLen = dataLen;
    packet->address = address;
    for (i = 0; i < dataLen; i++)
        packet->data[i] = data[i];

    return 0;
}
