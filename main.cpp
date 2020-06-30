/*
busb_get_device_descriptor* libusb example program to list devices on the bus
* Copyright © 2007 Daniel Drake <dsd@gentoo.org>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <iostream>
// #include <stdio.h>
#include <vector>
#include <libusb.h>

// From the HID spec:

static const int HID_GET_REPORT = 0x01;
static const int HID_SET_REPORT = 0x09;
static const int HID_REPORT_TYPE_INPUT = 0x01;
static const int HID_REPORT_TYPE_OUTPUT = 0x02;
static const int HID_REPORT_TYPE_FEATURE = 0x03;

using std::vector;
const uint16_t vendor_id = 0x43e;
const uint16_t product_id = 0x9a40;
const uint16_t max_brightness = 0xd2f0;
const uint16_t min_brightness = 0x0000;

static libusb_device *get_lg_ultrafine_usb_device(libusb_device **devs, int dev_num)
{
    libusb_device *dev, *lgdev = NULL;
    int i = 0, j = 0, k = 0;
    uint8_t path[8];

    while ((dev = devs[i++]) != NULL)
    {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0)
        {
            printf("failed to get device descriptor");
            return NULL;
        }

        if (desc.idVendor == vendor_id && desc.idProduct == product_id)
        {
            if(dev_num == k++) {
                lgdev = dev;
                break;
            }
        }
    }

    return lgdev;
}

uint16_t get_brightness(libusb_device_handle *handle)
{
    u_char data[8] = {0x00};
    // int res = hid_get_feature_report(handle, data, sizeof(data));
    int res = libusb_control_transfer(handle,
                                        LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                        HID_GET_REPORT, (HID_REPORT_TYPE_FEATURE<<8)|0, 1, data, sizeof(data), 0);

    if (res < 0)
    {
        printf("Unable to get brightness.\n");
        printf("libusb_control_transfer error: %s (%d)\n", libusb_error_name(res), res);
    } 
    else {
        // for (int i = 0; i < sizeof(data); i++) {
        //     printf("0x%x  ", data[i]);
        // }
        // printf("\n");
    }

    uint16_t val = data[0] + (data[1] << 8);
    // printf("val=%d (0x%x 0x%x 0x%x)\n", val, data[0], data[1], data[2]);

    return int((float(val) / 54000) * 100.0);
}

void set_brightness(libusb_device_handle *handle, uint16_t val)
{
    if(val > max_brightness) {
        val = max_brightness;
    }

    if(val < min_brightness) {
        val = min_brightness;
    }

    u_char data[6] = {
        u_char(val & 0x00ff),
        u_char((val >> 8) & 0x00ff), 0x00, 0x00, 0x00, 0x00 };
    // int res = hid_send_feature_report(handle, data, sizeof(data));
    int res = libusb_control_transfer(handle,
                                      LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
                                      HID_SET_REPORT, (HID_REPORT_TYPE_FEATURE<<8)|0, 1, data, sizeof(data), 0);

    if (res < 0)
    {
        printf("Unable to set brightness.\n");
        printf("libusb_control_transfer error: %s\n", libusb_error_name(res));
    } 
    // else {
    //     get_brightness(handle);
    // }
}

void adjust_brightness(libusb_device_handle *handle, int brightness_pct)
{
    auto brightness = get_brightness(handle);
    printf("Current brightness = %d%4s\r", int((float(brightness) / 54000) * 100.0), " ");
    set_brightness(handle, int(float(max_brightness) * (float(brightness_pct) / 100.0)));
}

int main(int argc, char *argv[])
{
    if(argc < 3) {
        printf("Expected 2 arguments to be passed, the display index and the brightness as a percentage.\n");
        return -1;
    }
    
    int lg_dev_idx = -1;

    if(std::string(argv[1]) != "all") {
        lg_dev_idx = std::stoi(argv[1]);
    }

    int brightness_pct = std::stoi(argv[2]);

    printf("Awesome! lg_dv_idx=%i, brightness_pct=%i\n", lg_dev_idx, brightness_pct);

    libusb_device **devs, *lgdev;
    int r, openCode, iface = 1;
    ssize_t cnt;
    libusb_device_handle *handle;

    r = libusb_init(NULL);
    libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_WARNING);       // LIBUSB_LOG_LEVEL_DEBUG  
    if (r < 0)
    {
        printf("Unable to initialize libusb.\n");
        return r;
    }

    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0)
    {
        printf("Unable to get USB device list (%ld).\n", cnt);
        return (int)cnt;
    }

    int idx = lg_dev_idx >= 0 ? lg_dev_idx : 0;
    while(true) {
        lgdev = get_lg_ultrafine_usb_device(devs, idx);

        if (lgdev == NULL)
        {
            break;
        }

        openCode = libusb_open(lgdev, &handle);
        if (openCode == 0)
        {
            libusb_set_auto_detach_kernel_driver(handle, 1);
            // r = libusb_detach_kernel_driver(handle, iface);
            // if (r == LIBUSB_SUCCESS) {
            r = libusb_claim_interface(handle, iface);
            if (r == LIBUSB_SUCCESS) {
                adjust_brightness(handle, brightness_pct);
                libusb_release_interface(handle, iface);
                libusb_attach_kernel_driver(handle, iface);
            } else {
                printf("Failed to claim interface %d. Error: %d\n", iface, r);
                printf("Error: %s\n", libusb_error_name(r));
            }

            // } else {
            //     printf("Failed to detach interface %d. Error: %d\n", iface, r);
            //     printf("Error: %s\n", libusb_error_name(r));
            // }
            libusb_close(handle);
        }
        else
        {
            printf("libusb_open failed and returned %d\n", openCode);
        }

        if(idx == lg_dev_idx) {
            break;
        }
        idx++;
    }

    libusb_free_device_list(devs, 1);

    libusb_exit(NULL);

    return 0;
}
