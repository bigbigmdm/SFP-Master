//
// ch341eeprom programmer version 0.1 (Beta) 
//
//  Programming tool for the 24Cxx serial EEPROMs using the Winchiphead CH341A IC
//
// (c) December 2011 asbokid <ballymunboy@gmail.com> 
// (c) 2024 Mikhail Medvedev <e-ink-reader@yandex.ru>
//     
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <libusb-1.0/libusb.h>
#include <asm/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "ch341eeprom.h"

extern FILE *debugout, *verbout;
uint32_t getnextpkt;                            // set by the callback function
uint32_t syncackpkt;                            // synch / ack flag used by BULK OUT cb function
uint16_t byteoffset;
uint8_t *readbuf;
struct libusb_device_handle *devHandle;
// --------------------------------------------------------------------------
// ch341configure()
//      lock USB device for exclusive use
//      claim default interface
//      set default configuration
//      retrieve device descriptor
//      identify device revision
// returns *usb device handle

struct libusb_device_handle *ch341configure(uint16_t vid, uint16_t pid) {

    struct libusb_device *dev;

    int32_t ret=0;                    // set to < 0 to indicate USB errors
    uint32_t i = 0;
    int32_t currentConfig = 0;

    uint8_t  ch341DescriptorBuffer[0x12];
    uint8_t  ch341InBuffer[IN_BUF_SZ];          // 0x100 bytes in size
    uint8_t  ch341OutBuffer[EEPROM_READ_BULKOUT_BUF_SZ];

    ret = libusb_init(NULL);
    if(ret < 0) {
        printf("Couldnt initialise libusb\n");
        return NULL;
    }

    //libusb_set_debug(NULL, 3); //deprecated
    libusb_set_option(NULL, 3);  // maximum debug logging level

    printf( "Searching USB buses for WCH CH341a i2c EEPROM programmer [%04x:%04x]\n",
            USB_LOCK_VENDOR, USB_LOCK_PRODUCT);

    if(!(devHandle = libusb_open_device_with_vid_pid(NULL, USB_LOCK_VENDOR, USB_LOCK_PRODUCT))) {
        printf("Couldn't open device [%04x:%04x]\n", USB_LOCK_VENDOR, USB_LOCK_PRODUCT);
        return NULL;
    }
 
    if(!(dev = libusb_get_device(devHandle))) {
        printf("Couldnt get bus number and address of device\n");
        return NULL;
    }

    printf("Found [%04x:%04x] as device [%d] on USB bus [%d]\n", USB_LOCK_VENDOR, USB_LOCK_PRODUCT,
        libusb_get_device_address(dev), libusb_get_bus_number(dev));

    printf("Opened device [%04x:%04x]\n", USB_LOCK_VENDOR, USB_LOCK_PRODUCT);


    if(libusb_kernel_driver_active(devHandle, DEFAULT_INTERFACE)) {
        ret = libusb_detach_kernel_driver(devHandle, DEFAULT_INTERFACE);
        if(ret) {
            printf("Failed to detach kernel driver: '%s'\n", strerror(-ret));
            return NULL;
        } else
            printf("Detached kernel driver\n");
    }

    ret = libusb_get_configuration(devHandle, &currentConfig);
    if(ret) {
        printf("Failed to get current device configuration: '%s'\n", strerror(-ret));
        return NULL;
    }

    if(currentConfig != DEFAULT_CONFIGURATION)
        ret = libusb_set_configuration(devHandle, currentConfig);

    if(ret) {
        printf("Failed to set device configuration to %d: '%s'\n", DEFAULT_CONFIGURATION, strerror(-ret));
        return NULL;
    }

    ret = libusb_claim_interface(devHandle, DEFAULT_INTERFACE); // interface 0

    if(ret) {
        printf("Failed to claim interface %d: '%s'\n", DEFAULT_INTERFACE, strerror(-ret));
        return NULL;
    }
    
    printf( "Claimed device interface [%d]\n", DEFAULT_INTERFACE);

    ret = libusb_get_descriptor(devHandle, LIBUSB_DT_DEVICE, 0x00, ch341DescriptorBuffer, 0x12);

    if(ret < 0) {
        printf("Failed to get device descriptor: '%s'\n", strerror(-ret));
        return NULL;
    }
    
    printf( "Device reported its revision [%d.%02d]\n",
        ch341DescriptorBuffer[12], ch341DescriptorBuffer[13]);

    for(i=0;i<0x12;i++)
        printf("%02x ", ch341DescriptorBuffer[i]);
    printf("\n");

    return devHandle;
}


// --------------------------------------------------------------------------
//  ch341setstream()  
//      set the i2c bus speed (speed: 0 = 20kHz; 1 = 100kHz, 2 = 400kHz, 3 = 750kHz)
int32_t ch341setstream( uint32_t speed) {
    int32_t ret, i;
    uint8_t ch341outBuffer[EEPROM_READ_BULKOUT_BUF_SZ], *outptr;
    int32_t actuallen = 0;

    outptr = ch341outBuffer;

    *outptr++ = mCH341A_CMD_I2C_STREAM;
    *outptr++ = mCH341A_CMD_I2C_STM_SET | (speed & 0x3);
    *outptr   = mCH341A_CMD_I2C_STM_END;

    ret = libusb_bulk_transfer(devHandle, BULK_WRITE_ENDPOINT, ch341outBuffer, 3, &actuallen, DEFAULT_TIMEOUT);

    if(ret < 0) {
      printf("ch341setstream(): Failed write %d bytes '%s'\n", 3, strerror(-ret));
      return -1;
    }

    printf("ch341setstream(): Wrote %d bytes: ", 3);
    for(i=0; i < 3; i++)
        printf("%02x ", ch341outBuffer[i]);
    printf("\n");
    return 0;
}


void ch341ReadCmdMarshall(uint8_t *buffer, uint32_t addr, struct EEPROM *eeprom_info)
{
    uint8_t *ptr = buffer;
    uint8_t msb_addr;
    uint32_t size_kb;

    *ptr++ = mCH341A_CMD_I2C_STREAM; // 0
    *ptr++ = mCH341A_CMD_I2C_STM_STA; // 1
    // Write address
    *ptr++ = mCH341A_CMD_I2C_STM_OUT | ((*eeprom_info).addr_size + 1); // 2: I2C bus adddress + EEPROM address
    if ((*eeprom_info).addr_size >= 2) {
        // 24C32 and more
        msb_addr = addr >> 16 & (*eeprom_info).i2c_addr_mask;
        *ptr++ = (EEPROM_I2C_BUS_ADDRESS | msb_addr) << 1; // 3
        *ptr++ = (addr >> 8 & 0xFF); // 4
        *ptr++ = (addr >> 0 & 0xFF); // 5
    } else {
        // 24C16 and less
        msb_addr = addr >> 8 & (*eeprom_info).i2c_addr_mask;
        *ptr++ = (EEPROM_I2C_BUS_ADDRESS | msb_addr) << 1; // 3
        *ptr++ = (addr >> 0 & 0xFF); // 4
    }
    // Read
    *ptr++ = mCH341A_CMD_I2C_STM_STA; // 6/5
    *ptr++ = mCH341A_CMD_I2C_STM_OUT | 1; // 7/6
    *ptr++ = ((EEPROM_I2C_BUS_ADDRESS | msb_addr) << 1) | 1; // 8/7: Read command

    // Configuration?
    *ptr++ = 0xE0; // 9/8
    *ptr++ = 0x00; // 10/9
    if ((*eeprom_info).addr_size < 2)
        *ptr++ = 0x10; // x/10
    memcpy(ptr, "\x00\x06\x04\x00\x00\x00\x00\x00\x00", 9);
    ptr += 9; // 10
    size_kb = (*eeprom_info).size/1024;
    *ptr++ = size_kb & 0xFF; // 19
    *ptr++ = (size_kb >> 8) & 0xFF; // 20
    memcpy(ptr, "\x00\x00\x11\x4d\x40\x77\xcd\xab\xba\xdc", 10);
    ptr += 10;

    // Frame 2
    *ptr++ = mCH341A_CMD_I2C_STREAM;
    memcpy(ptr, "\xe0\x00\x00\xc4\xf1\x12\x00\x11\x4d\x40\x77\xf0\xf1\x12\x00" \
            "\xd9\x8b\x41\x7e\x00\xe0\xfd\x7f\xf0\xf1\x12\x00\x5a\x88\x41\x7e", 31);
    ptr += 31;

    // Frame 3
    *ptr++ = mCH341A_CMD_I2C_STREAM;
    memcpy(ptr, "\xe0\x00\x00\x2a\x88\x41\x7e\x06\x04\x00\x00\x11\x4d\x40\x77" \
            "\xe8\xf3\x12\x00\x14\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00", 31);
    ptr += 31;

    // Finalize
    *ptr++ = mCH341A_CMD_I2C_STREAM; // 0xAA
    *ptr++ = 0xDF; // ???
    *ptr++ = mCH341A_CMD_I2C_STM_IN; // 0xC0
    *ptr++ = mCH341A_CMD_I2C_STM_STO; // 0x75
    *ptr++ = mCH341A_CMD_I2C_STM_END; // 0x00

    assert(ptr - buffer == CH341_EEPROM_READ_CMD_SZ);
}

// --------------------------------------------------------------------------
// ch341readEEPROM()
//      read n bytes from device (in packets of 32 bytes)
int32_t ch341readEEPROM_param(uint8_t *buffer, uint32_t offset, uint32_t bytestoread, uint32_t ic_size, uint32_t block_size, uint8_t algorithm)
{

    uint8_t ch341outBuffer[EEPROM_READ_BULKOUT_BUF_SZ];
    uint8_t ch341inBuffer[IN_BUF_SZ]; // 0x100 bytes
    int32_t ret = 0, readpktcount = 0;
    struct libusb_transfer *xferBulkIn, *xferBulkOut;
    struct timeval tv = {0, 100};     // our async polling interval
//
    struct EEPROM eeprom_info;
    eeprom_info.name = "24c01";
    eeprom_info.size = ic_size;
    eeprom_info.page_size = (uint16_t)block_size;
    eeprom_info.addr_size = 0x0f & algorithm;
    eeprom_info.i2c_addr_mask = (0xf0 & algorithm) / 16;
    //printf("addr_size=%x addr_mask=%x",eeprom_info.addr_size, eeprom_info.i2c_addr_mask);
//
    xferBulkIn  = libusb_alloc_transfer(0);
    xferBulkOut = libusb_alloc_transfer(0);

    if (!xferBulkIn || !xferBulkOut) {
        printf("Couldnt allocate USB transfer structures\n");
        return -1;
    }

    byteoffset = 0;

    memset(ch341inBuffer, 0, EEPROM_READ_BULKIN_BUF_SZ);
    ch341ReadCmdMarshall(ch341outBuffer, offset, &eeprom_info); // Fill output buffer

    libusb_fill_bulk_transfer(xferBulkIn, devHandle, BULK_READ_ENDPOINT, ch341inBuffer,
        EEPROM_READ_BULKIN_BUF_SZ, cbBulkIn, NULL, DEFAULT_TIMEOUT);

    libusb_fill_bulk_transfer(xferBulkOut, devHandle, BULK_WRITE_ENDPOINT,
        ch341outBuffer, EEPROM_READ_BULKOUT_BUF_SZ, cbBulkOut, NULL, DEFAULT_TIMEOUT);


    libusb_submit_transfer(xferBulkIn);
    libusb_submit_transfer(xferBulkOut);

    readbuf = buffer;

    while (1) {
        ret = libusb_handle_events_timeout(NULL, &tv);

        if (ret < 0 || getnextpkt == -1) {  // indicates an error
            printf("ret from libusb_handle_timeout = %d\n", ret);
            printf("getnextpkt = %d\n", getnextpkt);
            if (ret < 0)
                printf("USB read error : %s\n", strerror(-ret));
            libusb_free_transfer(xferBulkIn);
            libusb_free_transfer(xferBulkOut);
            return -1;
        }
        if (getnextpkt == 1) {   // callback function reports a new BULK IN packet received
            getnextpkt = 0; // reset the flag
            readpktcount++; // increment the read packet counter
            byteoffset += EEPROM_READ_BULKIN_BUF_SZ;
            if (byteoffset == bytestoread)
                break;

            libusb_submit_transfer(xferBulkIn); // re-submit request for next BULK IN packet of EEPROM data
            if (syncackpkt)
                syncackpkt = 0;
            // if 4th packet received, we are at end of 0x80 byte data block,
            // if it is not the last block, then resubmit request for data
            if (readpktcount == 4) {
                readpktcount = 0;

                ch341ReadCmdMarshall(ch341outBuffer, byteoffset, &eeprom_info); // Fill output buffer
                libusb_fill_bulk_transfer(xferBulkOut, devHandle, BULK_WRITE_ENDPOINT, ch341outBuffer,
                            EEPROM_READ_BULKOUT_BUF_SZ, cbBulkOut, NULL, DEFAULT_TIMEOUT);

                libusb_submit_transfer(xferBulkOut); // update transfer struct (with new EEPROM page offset)
                                     // and re-submit next transfer request to BULK OUT endpoint
            }
        }
    }

    libusb_free_transfer(xferBulkIn);
    libusb_free_transfer(xferBulkOut);
    return 0;
}


// Callback function for async bulk in comms
void cbBulkIn(struct libusb_transfer *transfer)
{
    int i;

    switch (transfer->status) {
        case LIBUSB_TRANSFER_COMPLETED:
            // display the contents of the BULK IN data buffer
/*
            for (i = 0; i < transfer->actual_length; i++) {
                if(!(i % 16))
                dprintf("%02x ", transfer->buffer[i]);
            }
*/
            // copy read data to our EEPROM buffer
            memcpy(readbuf + byteoffset, transfer->buffer, transfer->actual_length);
            getnextpkt = 1;
            break;
        default:
            printf("\ncbBulkIn: error : %d\n", transfer->status);
            getnextpkt = -1;
    }
    return;
}

// Callback function for async bulk out comms
void cbBulkOut(struct libusb_transfer *transfer)
{
    syncackpkt = 1;
    return;
}

// --------------------------------------------------------------------------
// ch341writeEEPROM()
//      write n bytes to 24c32/24c64 device (in packets of 32 bytes)
int32_t ch341writeEEPROM_param(uint8_t *buffer, uint32_t offset, uint32_t bytesum, uint32_t block_size, uint8_t algorithm)
{
    printf("ch341writeEEPROM_param\n");
    uint8_t addr_size = 0x0f & algorithm;
    uint8_t i2c_addr_mask = (0xf0 & algorithm) / 16;
    uint8_t ch341outBuffer[512/*EEPROM_WRITE_BUF_SZ*/];
    uint8_t *outptr, *bufptr;
    uint8_t i2cCmdBuffer[256];
    int32_t ret = 0;
    uint32_t payload_size, byteoffset = offset;
    uint32_t bytes = bytesum;
    uint8_t addrbytecount = 0x0f & algorithm + 1; // 24c32 and 24c64 (and other 24c??) use 3 bytes for addressing
    int32_t actuallen = 0;
    uint32_t page_size = block_size;
    uint16_t page_size_left;
    uint8_t part_no;
    uint8_t *i2cBufPtr;

    if (bytesum < page_size) page_size = bytesum;

    bufptr = buffer;

    while (bytes) {
        outptr = i2cCmdBuffer;
        if (addr_size >= 2) {
            *outptr++ = (uint8_t) (0xa0 | (byteoffset >> 16 & i2c_addr_mask) << 1); // EEPROM device address
            *outptr++ = (uint8_t) (byteoffset >> 8 & 0xff); // MSB (big-endian) byte address
        } else {
            *outptr++ = (uint8_t) (0xa0 | (byteoffset >> 8 & i2c_addr_mask) << 1); // EEPROM device address
        }
        *outptr++ = (uint8_t) (byteoffset & 0xff); // LSB of 16-bit    byte address

        memcpy(outptr, bufptr, page_size); // Copy one page

        byteoffset += page_size;
        bufptr += page_size;
        bytes  -= page_size;

        outptr = ch341outBuffer;
        page_size_left = page_size + addrbytecount;
        part_no = 0;
        i2cBufPtr = i2cCmdBuffer;
        while (page_size_left) {
            uint8_t to_write = MIN(page_size_left, 28);
            *outptr++ = mCH341A_CMD_I2C_STREAM;
            if (part_no == 0) { // Start packet
                *outptr++ = mCH341A_CMD_I2C_STM_STA;
            }
            *outptr++ = mCH341A_CMD_I2C_STM_OUT | to_write;
            memcpy(outptr, i2cBufPtr, to_write);
            outptr += to_write;
            i2cBufPtr += to_write;
            page_size_left -= to_write;

            if (page_size_left == 0) { // Stop packet
                *outptr++ = mCH341A_CMD_I2C_STM_STO;
            }
            *outptr++ = mCH341A_CMD_I2C_STM_END;
            part_no++;
        }
        payload_size = outptr - ch341outBuffer;
/*
        for (i = 0; i < payload_size; i++) {
            if(!(i % 0x10))
                dprintf("\n%04x : ", i);
            dprintf("%02x ", ch341outBuffer[i]);
        }
*/

        ret = libusb_bulk_transfer(devHandle, BULK_WRITE_ENDPOINT,
                        ch341outBuffer, payload_size, &actuallen, DEFAULT_TIMEOUT);

        if (ret < 0) {
            printf("Failed to write to EEPROM: '%s'\n", strerror(-ret));
            return -1;
        }

        outptr = ch341outBuffer;
        *outptr++ = mCH341A_CMD_I2C_STREAM;
        *outptr++ = 0x5a;  // what is this 0x5a??
        *outptr++ = mCH341A_CMD_I2C_STM_END;

        ret = libusb_bulk_transfer(devHandle, BULK_WRITE_ENDPOINT, ch341outBuffer, 3, &actuallen, DEFAULT_TIMEOUT);

        if (ret < 0) {
            printf("Failed to write to EEPROM: '%s'\n", strerror(-ret));
            return -1;
        }
    }
    return 0;
}


// --------------------------------------------------------------------------


int32_t ch341aConnect()
{
    //struct libusb_device_handle *devHandle;
    if(!(devHandle = ch341configure(USB_LOCK_VENDOR, USB_LOCK_PRODUCT)))
        {
            return -1;
        }
    else
        {
        //if(ch341setstream(CH341_I2C_STANDARD_SPEED) < 0)
        if(ch341setstream(CH341_I2C_LOW_SPEED) < 0)
              {
                 printf( "Couldnt set i2c bus speed\n");
                 return -1;
              }
            return 0;
        }

}

int32_t ch341aShutdown()
{
    if (devHandle == NULL) return -1;

    libusb_release_interface(devHandle, DEFAULT_INTERFACE);
    libusb_close(devHandle);
    libusb_exit(NULL);
    devHandle = NULL;
    return 0;
}
