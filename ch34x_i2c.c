//  Programming tool for the 24Cxx serial EEPROMs using the Winchiphead ch34x IC
//
// (c) December 2011 asbokid <ballymunboy@gmail.com>
// (c) December 2023 aystarik <aystarik@gmail.com>
// (c) February 2026 Mikhail Medvedev <e-ink-reader@yandex.ru>
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

#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "ch34x_i2c.h"

struct libusb_device_handle *devHandle;
extern struct ch347_priv *priv;

struct xxx {
  uint8_t ibuf[512];
  uint8_t obuf[512];
} i2c_buf;


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

int32_t ch341setstream( uint32_t speed) {
    int32_t ret, i;
    uint8_t ch341outBuffer[EEPROM_READ_BULKOUT_BUF_SZ], *outptr;
    int32_t actuallen = 0;

    outptr = ch341outBuffer;

    *outptr++ = ch34x_CMD_I2C_STREAM;
    *outptr++ = ch34x_CMD_I2C_STM_SET | (speed & 0x3);
    *outptr   = ch34x_CMD_I2C_STM_END;

    ret = libusb_bulk_transfer(devHandle, ch341_BULK_WRITE_ENDPOINT, ch341outBuffer, 3, &actuallen, DEFAULT_TIMEOUT);

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


int ch34xdelay_ms(unsigned ms) {
    i2c_buf.obuf[0] = ch34x_CMD_I2C_STREAM;
    i2c_buf.obuf[1] = ch34x_CMD_I2C_STM_MS | (ms & 0xf);        // Wait up to 15ms
    i2c_buf.obuf[2] = ch34x_CMD_I2C_STM_END;
    int actuallen = 0;
    libusb_bulk_transfer(devHandle, ch341_BULK_WRITE_ENDPOINT, i2c_buf.obuf, 3, &actuallen, DEFAULT_TIMEOUT);
    return 0;
}

int ch34xi2cBlockRead(uint8_t *buf, uint32_t address, uint32_t blockSize, uint8_t algorithm)
{
    int ret;
    uint32_t step, maxstep;
    int32_t actuallen = 0;
    uint32_t size = blockSize;

    uint8_t deviceAddress = 0;
    uint8_t wordAddressLo = 0;
    uint8_t wordAddressHi = 0;

        if (size > 16) size = 16;
        maxstep = blockSize / size;

        for (step = 0; step < maxstep; step++)
        {
            uint8_t *ptr = i2c_buf.obuf;
            *ptr++ = ch34x_CMD_I2C_STREAM;
            *ptr++ = ch34x_CMD_I2C_STM_STA;

            if ((algorithm & 0x0f) == 0x01) //1 byte address
            {
                *ptr++ = ch34x_CMD_I2C_STM_OUT | 2;
                deviceAddress = (uint8_t) ( ((((address & 0xff00) >> 8) & ((algorithm & 0xf0) >> 4)) << 1) | 0xa0);
                wordAddressLo = (uint8_t) (address & 0x00ff);
                *ptr++ = deviceAddress;
                *ptr++ = wordAddressLo;
            }
            if ((algorithm & 0x0f) == 0x02) //2 byte address
            {
                *ptr++ = ch34x_CMD_I2C_STM_OUT | 3;
                deviceAddress = (uint8_t) ( ((((address & 0xff0000) >> 16) & ((algorithm & 0xf0) >> 4)) << 1) | 0xa0);
                wordAddressLo = (uint8_t) (address & 0x00ff);
                wordAddressHi = (uint8_t) ((address & 0xff00) >> 8);
                *ptr++ = deviceAddress;
                *ptr++ = wordAddressHi;
                *ptr++ = wordAddressLo;
            }
                //device addr + read bit
                *ptr++ = ch34x_CMD_I2C_STM_STA;
                *ptr++ = ch34x_CMD_I2C_STM_OUT | 1;
                *ptr++ = deviceAddress | 0x01;
                *ptr++ = ch34x_CMD_I2C_STM_IN | ((uint8_t)(size - 1));
                *ptr++ = ch34x_CMD_I2C_STM_IN; //last byte and end of packet
                *ptr++ = ch34x_CMD_I2C_STM_STO;
                *ptr++ = ch34x_CMD_I2C_STM_END;


            ret = libusb_bulk_transfer(devHandle, ch341_BULK_WRITE_ENDPOINT, i2c_buf.obuf, 15 , &actuallen, DEFAULT_TIMEOUT);
            if (ret < 0)
            {
                fprintf(stderr, "USB write error : %s\n", strerror(-ret));
                return ret;
            }
            ret = libusb_bulk_transfer(devHandle, ch341_BULK_READ_ENDPOINT, i2c_buf.ibuf, blockSize + 4, &actuallen, DEFAULT_TIMEOUT);
            if (ret < 0)
            {
                fprintf(stderr, "USB read error : %s\n", strerror(-ret));
                return ret;
            }

            memcpy(&buf[step * size], &i2c_buf.ibuf[0], size);

            address = address + size;
            ret = ch34xdelay_ms(10);
            if (ret < 0)
            {
                fprintf(stderr, "Failed to set timeout: '%s'\n", strerror(-ret));
                return -1;
            }
        }
    return 0;
}

int ch34xi2cBlockWrite(uint8_t *buf, uint32_t address, uint32_t blockSize, uint32_t sectorSize, uint8_t algorithm)
{
    int ret;
    int32_t actuallen = 0;
    uint32_t size = blockSize;
    uint32_t step, maxstep;
    uint8_t deviceAddress = 0;
    uint8_t wordAddressLo = 0;
    uint8_t wordAddressHi = 0;

        if (size > sectorSize) size = sectorSize;
        if (size > 16) size = 16;
        maxstep = blockSize / size;

        for (step = 0; step < maxstep; step++)
        {
            uint8_t *ptr = i2c_buf.obuf;
            *ptr++ = ch34x_CMD_I2C_STREAM;
            *ptr++ = ch34x_CMD_I2C_STM_STA;

            if ((algorithm & 0x0f) == 0x01) //1 byte address
            {
                *ptr++ = ch34x_CMD_I2C_STM_OUT | 2;
                deviceAddress = (uint8_t) ( ((((address & 0xff00) >> 8) & ((algorithm & 0xf0) >> 4)) << 1) | 0xa0);
                wordAddressLo = (uint8_t) (address & 0x00ff);
                *ptr++ = deviceAddress;
                *ptr++ = wordAddressLo;
            }
            if ((algorithm & 0x0f) == 0x02) //2 byte address
            {
                *ptr++ = ch34x_CMD_I2C_STM_OUT | 3;
                deviceAddress = (uint8_t) ( ((((address & 0xff0000) >> 16) & ((algorithm & 0xf0) >> 4)) << 1) | 0xa0);
                wordAddressLo = (uint8_t) (address & 0x00ff);
                wordAddressHi = (uint8_t) ((address & 0xff00) >> 8);
                *ptr++ = deviceAddress;
                *ptr++ = wordAddressHi;
                *ptr++ = wordAddressLo;
            }
                *ptr++ = ch34x_CMD_I2C_STM_OUT | ((uint8_t)(size ));

                 for (uint32_t i = 0; i < size; i++) *ptr++ = buf[i + step * size];

                *ptr++ = ch34x_CMD_I2C_STM_STO;
                *ptr++ = ch34x_CMD_I2C_STM_END;


            ret = libusb_bulk_transfer(devHandle, ch341_BULK_WRITE_ENDPOINT, i2c_buf.obuf, size + 12 , &actuallen, DEFAULT_TIMEOUT);
            if (ret < 0)
            {
                fprintf(stderr, "Failed to write to I2C: '%s'\n", strerror(-ret));
                return ret;
            }

            ret = ch34xdelay_ms(10);
            if (ret < 0)
            {
                fprintf(stderr, "Failed to set timeout: '%s'\n", strerror(-ret));
                return -1;
            }
            address = address + size;
        }

    return 0;
}
