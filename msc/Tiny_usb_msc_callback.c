#include "header.h"

void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    const char vid[] = "xAI";
    const char pid[] = "RP2040 USB Drive";
    const char rev[] = "1.0";
    memcpy(vendor_id, vid, strlen(vid));
    memcpy(product_id, pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    return true; // Drive always ready
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size) {
    *block_count = FS_SIZE / FLASH_SECTOR_SIZE;
    *block_size = FLASH_SECTOR_SIZE;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    return true; // No-op
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    disk_read(lun, buffer, lba, bufsize / FLASH_SECTOR_SIZE);
    return bufsize;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    disk_write(lun, buffer, lba, bufsize / FLASH_SECTOR_SIZE);
    return bufsize;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
    return -1; // Unsupported commands
}
// Handle USB control requests
bool tud_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request) {
    if (request->bRequest == TUSB_REQ_SET_DESCRIPTOR) {
        // Reject SET_DESCRIPTOR requests (not supported for MSC)
        return false;
    }
    // Let TinyUSB handle other standard requests
    return true;
}