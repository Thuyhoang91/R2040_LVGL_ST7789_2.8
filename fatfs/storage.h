#ifndef STORAGE_H
#define STORAGE_H

#include "pico/stdlib.h"
#include "hardware/flash.h"

#define FLASH_TARGET_OFFSET (2 * 1024 * 1024) // 2MB offset (0x10200000)
//#define FLASH_SECTOR_SIZE FLASH_SECTOR_SIZE // 4096 bytes from hardware/flash.h
#define FLASH_TOTAL_SIZE (16 * 1024 * 1024) // 16MB
#define FS_SIZE (FLASH_TOTAL_SIZE - FLASH_TARGET_OFFSET) // ~14MB

#endif