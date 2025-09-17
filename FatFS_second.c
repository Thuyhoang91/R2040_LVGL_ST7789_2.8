// #include <stdio.h>
// #include <string.h>
// #include "pico/stdlib.h"
// #include "hardware/flash.h"
// // #include "hardware/sync.h"
// #include "ff.h"
// #include "diskio.h"
#include "header.h"

FATFS fs;
FIL file;
FRESULT fr;

void fatfs_format() {    

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
            .au_size = FLASH_SECTOR_SIZE // Cluster size = sector size
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
}

int main() {
    stdio_init_all();
    tusb_init();
    sleep_ms(2000);
    st7789_init();
    lv_port_init();
    fatfs_format(); 
    lv_fs_fatfs_init();

    // // Write text file
    // const char *text = "Hello, RP2040! FatFS test.";
    // fr = f_open(&file, "test.txt", FA_WRITE | FA_CREATE_ALWAYS);
    // if (fr == FR_OK) {
    //     UINT bw;
    //     f_write(&file, text, strlen(text), &bw);
    //     f_close(&file);
    //     printf("Wrote test.txt (%u bytes)\n", bw);
    // } else {
    //     printf("Text file open failed: %d\n", fr);
    // }

    // // Read text file
    // char buffer[64];
    // fr = f_open(&file, "test.txt", FA_READ);
    // if (fr == FR_OK) {
    //     UINT br;
    //     f_read(&file, buffer, sizeof(buffer) - 1, &br);
    //     buffer[br] = '\0';
    //     f_close(&file);
    //     printf("Read test.txt: %s\n", buffer);
    // } else {
    //     printf("Text file read failed: %d\n", fr);
    // }
    // // Write binary image file (e.g., 4KB raw pixel data)
    // uint8_t image_data[4096];
    // for (int i = 0; i < 4096; i++) {
    //     image_data[i] = i % 256; // Dummy pixel values
    // }
    // fr = f_open(&file, "image.bin", FA_WRITE | FA_CREATE_ALWAYS);
    // if (fr == FR_OK) {
    //     UINT bw;
    //     f_write(&file, image_data, sizeof(image_data), &bw);
    //     f_close(&file);
    //     printf("Image file written (%u bytes)\n", bw);
    // } else {
    //     printf("Image file open failed: %d\n", fr);
    // }

    // // Read binary image file
    // uint8_t loaded_image[4096];
    // fr = f_open(&file, "image.bin", FA_READ);
    // if (fr == FR_OK) {
    //     UINT br;
    //     f_read(&file, loaded_image, sizeof(loaded_image), &br);
    //     f_close(&file);
    //     printf("Image file read, size: %u bytes\n", br);
    //     // Process loaded_image (e.g., send to OLED display)
    // } else {
    //     printf("Image file read failed: %d\n", fr);
    // }

    // List directory
    // DIR dir;
    // FILINFO fno;
    // fr = f_opendir(&dir, "/");
    // if (fr == FR_OK) {
    //     printf("Directory contents:\n");
    //     while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
    //         printf("File: %s, size: %lu bytes\n", fno.fname, fno.fsize);
    //     }
    //     f_closedir(&dir);
    // }

    // Unmount
    //f_mount(NULL, "", 0);
    while (true) {
        tud_task();
        lv_task_handler();
        tight_loop_contents();
    }
    return 0;
}