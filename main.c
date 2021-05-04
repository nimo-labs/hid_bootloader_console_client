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

void dumpUsb(unsigned char *packet)
{
    printf("resp: ");
    for (int i = 0; i < 32; i++)
    {
        printf("%.2X ", packet[i]);
    }
    printf("\r\n");
}

void waitResp(void)
{
    uint8_t resp[64];
    struct hidBlProtocolPacket_s recvPacket;
    while (1)
    {
        if (USB_HID_SUCCESS == usb_hid_read(device, resp, sizeof(resp)))
        {
            hidBlProtocolDeSerialisePacket(&recvPacket, resp);
            if (HID_BL_PROTOCOL_ACK == recvPacket.packetType)
                break;
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
            error_exit("hid_write()");
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
                error_exit("hid_write()");
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
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    usb_hid_device_t devices[256];
    int n_devices;
    FILE *ptr;

    /*Check command line args*/
    if (argc < 4) /*Figure out something better here */
    {
        printf("Usage:\n\n");
        printf("hid_boot cmd mem_loc filename\n\n");
        printf("Where:\n");
        printf("cmd is one of p (program) or e (erase internal flash)\n");
        printf("mem_loc is the start location in hex (e.g. 0x1000)\n");
        printf("filename is the firmware filename\n\n");
        exit(1);
    }

    /************************/
    ptr = fopen(argv[3], "rb"); // r for read, b for binary
    if (NULL == ptr)
        error_exit("File not found.");
    flashAddress = strtol(argv[2], NULL, 16); /*Starting point in flash of the application*/

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

    if(0 == strncmp(argv[1], "w", 1))
        writeFile();
    else if(0 == strncmp(argv[1], "e", 1))
        eraseIntFlash();

    usb_hid_close(device);

    usb_hid_cleanup(devices, n_devices);

    return 0;
}