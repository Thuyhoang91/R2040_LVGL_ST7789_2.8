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

extern lv_group_t *group;

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

void load_text_to_label(lv_obj_t *label, const char *filename) {
    lv_fs_file_t file;
    lv_fs_res_t res;
    char buf[1024];  // Buffer size; adjust based on file size (LVGL handles long text)
    uint32_t bytes_read = 0;
    uint32_t total_bytes = 0;
    char *full_text = NULL;  // Will allocate dynamically for large files

    // Open file (e.g., "Q:/text.txt" where 'Q' is the drive)
    res = lv_fs_open(&file, filename, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK) {
        //LV_LOG_ERROR("Failed to open file: %s", filename);
        //lv_label_set_text(label, "Error: Cannot open file");
        lv_textarea_set_text(label, "Error: Cannot open file");
        return;
    }

    // Read file in chunks (for large files)
    do {
        res = lv_fs_read(&file, buf, sizeof(buf) - 1, &bytes_read);
        if (res != LV_FS_RES_OK || bytes_read == 0) break;

        buf[bytes_read] = '\0';  // Null-terminate chunk
        total_bytes += bytes_read;

        // Reallocate and append to full_text
        char *new_text = lv_realloc(full_text, total_bytes + 1);
        if (!new_text) {
            //LV_LOG_ERROR("Out of memory");
            break;
        }
        full_text = new_text;
        lv_strcat(full_text + (total_bytes - bytes_read), buf);
    } while (bytes_read == sizeof(buf) - 1);

    lv_fs_close(&file);

    if (full_text) {
        //lv_label_set_text(label, full_text);  // Set the full text
        lv_textarea_set_text(label, full_text);  // Set the full text
        lv_free(full_text);  // Free after setting (LVGL copies it internally)
    }
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

    // LVGL file write example
    // lv_fs_file_t file;
    // lv_fs_res_t res = lv_fs_open(&file, "F:/test1.txt", LV_FS_MODE_WR);
    // if (res == LV_FS_RES_OK) {
    //     const char *data = "Hello, Flash!";
    //     uint32_t written_bytes;
    //     lv_fs_write(&file, data, strlen(data), &written_bytes);
    //     lv_fs_close(&file);
    // }

    // Example: Load an image from QSPI flash
    // lv_obj_t *img = lv_img_create(lv_scr_act());
    // lv_img_set_src(img, "F:/image.bin");
    // lv_obj_center(img);


    // Example: Load a text file from QSPI flash
    // lv_obj_t *label = lv_label_create(lv_scr_act());
    // // lv_label_set_text(label, "F:/test.txt");
    // load_text_to_label(label, "F:/test.txt");
    // lv_obj_center(label);

    //example textarea load file test.txt
    lv_obj_t *ta = lv_textarea_create(lv_scr_act());
    // lv_textarea_set_text(ta, "F:/test.txt");
    lv_textarea_set_one_line(ta, false);
    // add size to textarea
    lv_obj_set_size(ta, 200, 150);
    lv_textarea_set_placeholder_text(ta, "No file loaded");
    load_text_to_label(ta, "F:/test.txt");
    lv_obj_center(ta);
    // add textarea to group
    lv_group_add_obj(group, ta);

    
    while (true) {
        tud_task();
        lv_task_handler();
        tight_loop_contents();
    }
    return 0;
}