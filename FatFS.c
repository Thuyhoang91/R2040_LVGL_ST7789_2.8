#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "ff.h"
#include "diskio.h"
#include "storage.h"
#include "tusb.h"

int main() {
    stdio_init_all();
    tusb_init(); // Initialize TinyUSB
    sleep_ms(2000); // Wait for UART serial

    FATFS fs;
    FIL file;
    FRESULT fr;

    // Mount FatFS
    fr = f_mount(&fs, "", 1);
    if (fr == FR_NO_FILESYSTEM) {
        printf("No filesystem, formatting...\n");
        BYTE work[FLASH_SECTOR_SIZE];
        MKFS_PARM opt = {
            .fmt = FM_FAT, // FAT16
            .n_fat = 1,
            .align = 0,
            .n_root = 0,
            .au_size = FLASH_SECTOR_SIZE // Cluster size = 4KB
        };
        fr = f_mkfs("", &opt, work, sizeof(work));
        if (fr == FR_OK) {
            printf("Filesystem formatted\n");
            fr = f_mount(&fs, "", 1);
        }
    }
    if (fr != FR_OK) {
        printf("Mount failed: %d\n", fr);
        while (1);
    }
    printf("FatFS mounted\n");

    // Write text file
    const char *text = "Hello from RP2040! USB mass storage test.";
    fr = f_open(&file, "hello.txt", FA_WRITE | FA_CREATE_ALWAYS);
    if (fr == FR_OK) {
        UINT bw;
        f_write(&file, text, strlen(text), &bw);
        f_close(&file);
        printf("Wrote hello.txt (%u bytes)\n", bw);
    } else {
        printf("Text file open failed: %d\n", fr);
    }

    // Write binary image file (4KB)
    uint8_t image_data[4096];
    for (int i = 0; i < 4096; i++) {
        image_data[i] = i % 256; // Dummy pixel data
    }
    fr = f_open(&file, "image.bin", FA_WRITE | FA_CREATE_ALWAYS);
    if (fr == FR_OK) {
        UINT bw;
        f_write(&file, image_data, sizeof(image_data), &bw);
        f_close(&file);
        printf("Wrote image.bin (%u bytes)\n", bw);
    } else {
        printf("Image file open failed: %d\n", fr);
    }

    while (true) {
        tud_task(); // Handle USB events

        // Periodically check for PC-written files
        static uint32_t last_check = 0;
        if (to_ms_since_boot(get_absolute_time()) - last_check > 5000) {
            fr = f_open(&file, "pc_file.txt", FA_READ);
            if (fr == FR_OK) {
                char buffer[64];
                UINT br;
                f_read(&file, buffer, sizeof(buffer) - 1, &br);
                buffer[br] = '\0';
                f_close(&file);
                printf("Read pc_file.txt: %s\n", buffer);
            }
            last_check = to_ms_since_boot(get_absolute_time());
        }
    }
    return 0;
}

// TinyUSB MSC callbacks
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
    return -1; // Unsupported SCSI commands
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