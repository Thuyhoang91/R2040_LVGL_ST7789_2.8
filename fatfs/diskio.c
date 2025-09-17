/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

// #include "ff.h"			/* Obtains integer types */
// #include "diskio.h"		/* Declarations of disk functions */
// #include "pico/stdlib.h"
// #include "hardware/flash.h"
// #include "hardware/sync.h"
// #include <string.h>
#include "header.h"

DSTATUS disk_initialize(BYTE pdrv) { return 0; }
DSTATUS disk_status(BYTE pdrv) { return 0; }
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
        case CTRL_SYNC: return RES_OK;
        case GET_SECTOR_COUNT: *(DWORD*)buff = FS_SIZE / FLASH_SECTOR_SIZE; return RES_OK;
        case GET_SECTOR_SIZE: *(WORD*)buff = FLASH_SECTOR_SIZE; return RES_OK;
        case GET_BLOCK_SIZE: *(DWORD*)buff = 1; return RES_OK;
        default: return RES_PARERR;
    }
}