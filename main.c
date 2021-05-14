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

/*- Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "usb_hid.h"
#include "support.h"

#include "hidBlProtocol.h"

/*- Definitions -------------------------------------------------------------*/
#define VERSION "v0.1"

#define PAGE_SIZE 64
#define ERASE_SIZE 256
#define BL_REQUEST 0x78656c41

#define CONFIG_MARKER_BEGIN 0xc70d8e1a
#define CONFIG_MARKER_END 0xceab9d56

unsigned char *fileBuff;
unsigned int sz;
usb_hid_device_t *device = NULL;
unsigned long flashAddress = 0;
uint32_t mfrId = 0;
uint32_t partId = 0;

void dumpUsb(unsigned char *packet)
{
    printf("resp: ");
    for (int i = 0; i < 32; i++)
    {
        printf("%.2X ", packet[i]);
    }
    printf("\r\n");
}

char *strToLower(char * string)
{
    for(size_t i =0; i < strlen(string); i++)
        string[i] = tolower((unsigned char)string[i]);
    return string;
}

void waitResp(void)
{
    uint8_t resp[64];
    uint8_t waiting = 1;
    struct hidBlProtocolPacket_s recvPacket;

    mfrId = 0;
    partId = 0;

    while (waiting)
    {
        if (USB_HID_SUCCESS == usb_hid_read(device, resp, sizeof(resp)))
        {
            hidBlProtocolDeSerialisePacket(&recvPacket, resp);
            switch(recvPacket.packetType)
            {
            case HID_BL_PROTOCOL_ACK :
                waiting = 0;
                break;
            case HID_BL_PROTOCOL_SEND_MFR_ID:
                printf("Mfr ID: 0x%.2X", recvPacket.data[3]);
                printf("%.2X", recvPacket.data[2]);
                printf("%.2X", recvPacket.data[1]);
                printf("%.2X\n", recvPacket.data[0]);

                mfrId = recvPacket.data[3] << 24;
                mfrId |= recvPacket.data[2] << 16;
                mfrId |= recvPacket.data[1] << 8;
                mfrId |= recvPacket.data[0];

                waiting = 0;
                break;
            case HID_BL_PROTOCOL_SEND_PART_ID:
                printf("Part ID: 0x%.2X", recvPacket.data[3]);
                printf("%.2X", recvPacket.data[2]);
                printf("%.2X", recvPacket.data[1]);
                printf("%.2X\n", recvPacket.data[0]);

                partId = recvPacket.data[3] << 24;
                partId |= recvPacket.data[2] << 16;
                partId |= recvPacket.data[1] << 8;
                partId |= recvPacket.data[0];
                waiting = 0;
                break;

            }
        }
    }
}

void writeFile(void)
{
    struct hidBlProtocolPacket_s sendPacket;
    unsigned long fileLoc = 0;
    unsigned char serPkt[64];

    while (1)
    {
        printf("Progress: %ld%%\r", (fileLoc * 100) / sz);
        if (hidBlProtocolEncodePacket(&sendPacket, flashAddress + fileLoc, HID_BL_PROTOCOL_WRITE_INT_FLASH, &fileBuff[fileLoc], 32))
        {
            exit(1);
        }
        hidBlProtocolSerialisePacket(&sendPacket, serPkt, sizeof(serPkt));
        if (USB_HID_SUCCESS != usb_hid_write(device, serPkt, sizeof(serPkt)))
            error_exit("1hid_write()");
        fileLoc += 32;
        waitResp();
        if (fileLoc > sz)
        {
            if (hidBlProtocolEncodePacket(&sendPacket, 0, HID_BL_PROTOCOL_RUN_INT, NULL, 0))
            {
                exit(1);
            }
            hidBlProtocolSerialisePacket(&sendPacket, serPkt, sizeof(serPkt));
            if (USB_HID_SUCCESS != usb_hid_write(device, serPkt, sizeof(serPkt)))
                error_exit("2hid_write()");
            break;
        }
    }
    printf("Progress 100%%\n");
}

void eraseIntFlash(void)
{
    struct hidBlProtocolPacket_s sendPacket;
    unsigned char serPkt[64];
    if (hidBlProtocolEncodePacket(&sendPacket, 0, HID_BL_PROTOCOL_ERASE_INT_FLASH, 0, 0))
    {
        exit(1);
    }
    hidBlProtocolSerialisePacket(&sendPacket, serPkt, sizeof(serPkt));
    if (USB_HID_SUCCESS != usb_hid_write(device, serPkt, sizeof(serPkt)))
        error_exit("hid_write()");
    waitResp();
}

unsigned int checkUc(char *requestedUc)
{
    struct hidBlProtocolPacket_s sendPacket;
    unsigned char serPkt[64];
    printf("Requested UC: %s\n", strToLower(requestedUc));

    if(0 == strncmp(strToLower(requestedUc), "m032lg6ae", 9))
    {
        /* Query BL for mfr and part */

        if (hidBlProtocolEncodePacket(&sendPacket, 0, HID_BL_PROTOCOL_GET_MFR_ID, 0, 0))
        {
            exit(1);
        }

        hidBlProtocolSerialisePacket(&sendPacket, serPkt, sizeof(serPkt));
        if (USB_HID_SUCCESS != usb_hid_write(device, serPkt, sizeof(serPkt)))
            error_exit("hid_write()");
        waitResp();

        if(MFR_NUVOTON != mfrId)
        {
            printf("Error, microcontroller Manufacturer ID doesn't match device\n");
            exit(1);
        }

        if (hidBlProtocolEncodePacket(&sendPacket, 0, HID_BL_PROTOCOL_GET_PART_ID, 0, 0))
        {
            exit(1);
        }

        hidBlProtocolSerialisePacket(&sendPacket, serPkt, sizeof(serPkt));
        if (USB_HID_SUCCESS != usb_hid_write(device, serPkt, sizeof(serPkt)))
            error_exit("hid_write()");
        waitResp();
        if(PART_M032LG6AE != partId)
        {
            printf("Error, microcontroller Part ID doesn't match device\n");
            exit(1);
        }
    }
    else
    {
        printf("Unknown microcontroller, try hid_boot l\n");
        return 1;
    }
    return 0;
}

void printUsage(void)
{
    printf("Usage:\n\n");
    printf("hid_boot cmd device mem_loc filename\n\n");
    printf("Where:\n");
    printf("cmd is one of p (program), e (erase internal flash), l (list supported controllers)\n");
    printf("device is the microcontroller part number.\n");
    printf("mem_loc is the start location in hex (e.g. 0x1000)\n");
    printf("filename is the firmware filename\n\n");
}

//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    usb_hid_device_t devices[256];
    int n_devices;
    FILE *ptr;

    /*Check command line args*/
    switch(argc)
    {
    case 2:
        if (0 == strncmp(argv[1], "l", 1))
        {
            printf("Supported controllers:\n\n");
            printf("m032lg2ae - Nuvoton M032LG2AE\n");
            printf("\n");
            exit(0);
        }
        else
            printUsage();
        exit(1);
        break;
    case 5:
        break;
    default:
        printUsage();
        exit(1);
        break;
    }


    /************************/
    ptr = fopen(argv[4], "rb"); // r for read, b for binary
    if (NULL == ptr)
        error_exit("File not found.");
    flashAddress = strtol(argv[3], NULL, 16); /*Starting point in flash of the application*/

    /************************/
    fseek(ptr, 0L, SEEK_END);
    sz = ftell(ptr);
    rewind(ptr);
    fileBuff = malloc(sz);

    if (sz != fread(fileBuff, 1, sz, ptr)) // read entire file into buffer
        error_exit("fread returned fewer bytes that expected!");
    fclose(ptr);

    usb_hid_init();

    n_devices = usb_hid_enumerate(devices, 256);

    for (int i = 0; i < n_devices; i++)
    {
        //  printf("%s\n", devices[i].product);
        if (strstr(devices[i].product, "HID Boot Loader"))
        {
            check(NULL == device, "Multiple devices found.");
            device = &devices[i];
        }
    }

    check(NULL != device, "Supported bootloader not found.");

    usb_hid_open(device);

    checkUc(argv[2]);

    if(0 == strncmp(argv[1], "w", 1))
        writeFile();
    else if(0 == strncmp(argv[1], "e", 1))
        eraseIntFlash();
    else
    {
        printUsage();
        exit(1);
    }

    usb_hid_close(device);

    usb_hid_cleanup(devices, n_devices);

    return 0;
}