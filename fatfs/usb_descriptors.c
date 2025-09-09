#include "tusb.h"

// Device descriptor
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MSC,
    .bDeviceSubClass    = 0x06, // SCSI
    .bDeviceProtocol    = 0x50, // Bulk-Only
    .bMaxPacketSize0    = 64,
    .idVendor           = 0x1234, // Example VID
    .idProduct          = 0x5678, // Example PID
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// String descriptors
char const* string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 }, // 0: English
    "xAI",                         // 1: Manufacturer
    "RP2040 USB Drive",            // 2: Product
    "12345678",                    // 3: Serial
};

static uint16_t _desc_str[32];
uint8_t const* tud_descriptor_device_cb(void) {
    return (uint8_t const*)&desc_device;
}

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    static uint8_t desc_config[] = {
        TUD_CONFIG_DESCRIPTOR(1, 1, 0, 32, TUSB_DESC_CONFIG_ATT_SELF_POWERED, 100),
        TUD_MSC_DESCRIPTOR(0, 0, 0x01, 0x81, 64), // Interface 0, no string, EP1 OUT, EP1 IN, 64 bytes
    };
    return desc_config;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    uint8_t chr_count;
    if (index == 0) {
        memcpy(_desc_str, string_desc_arr[0], 4);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;
        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return _desc_str;
}