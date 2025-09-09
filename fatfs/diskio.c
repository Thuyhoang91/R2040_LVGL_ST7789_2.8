#include "pico.h"
#include "string.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "ff.h"
#include "diskio.h"
#include "storage.h"

DSTATUS disk_initialize(BYTE pdrv) {
    return 0; // Flash always initialized
}

DSTATUS disk_status(BYTE pdrv) {
    return 0; // Always ready
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count) {
    uint32_t addr = FLASH_TARGET_OFFSET + (sector * FLASH_SECTOR_SIZE);
    memcpy(buff, (const void *)(XIP_BASE + addr), count * FLASH_SECTOR_SIZE);
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count) {
    uint32_t addr = FLASH_TARGET_OFFSET + (sector * FLASH_SECTOR_SIZE);
    uint32_t ints = save_and_disable_interrupts();
    for (UINT i = 0; i < count; i++) {
        flash_range_erase(addr + (i * FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
        flash_range_program(addr + (i * FLASH_SECTOR_SIZE), buff + (i * FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE);
    }
    restore_interrupts(ints);
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;
        case GET_SECTOR_COUNT:
            *(DWORD*)buff = FS_SIZE / FLASH_SECTOR_SIZE;
            return RES_OK;
        case GET_SECTOR_SIZE:
            *(WORD*)buff = FLASH_SECTOR_SIZE;
            return RES_OK;
        case GET_BLOCK_SIZE:
            *(DWORD*)buff = 1; // Erase block size in sectors
            return RES_OK;
        default:
            return RES_PARERR;
    }
}

DWORD get_fattime(void) {
    // Fixed timestamp: January 1, 2025, 00:00:00
    // Format: (year-1980) << 25 | month << 21 | day << 16 | hour << 11 | minute << 5 | second/2
    return ((2025 - 1980) << 25) | (1 << 21) | (1 << 16) | (0 << 11) | (0 << 5) | (0 / 2);
}