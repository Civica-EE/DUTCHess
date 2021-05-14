#include <storage/disk_access.h>
#include <fs/fs.h>
#include <ff.h>
#include <logging/log.h>

#include "fs.h"

LOG_MODULE_REGISTER(dutchess_fs);

static FATFS fat_fs;

static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};

static const char *disk_mount_pt = "/SD:";

void dutchess_fs_init ()
{
    static const char *disk_pdrv = "SD";
    uint64_t memory_size_mb;
    uint32_t block_count;
    uint32_t block_size;

    if (disk_access_init(disk_pdrv) != 0)
    {
        LOG_ERR("Storage init ERROR!");
        return;
    }

    if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &block_count))
    {
        LOG_ERR("Unable to get sector count");
        return;
    }

    LOG_INF("Block count %u", block_count);

    if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &block_size))
    {
        LOG_ERR("Unable to get sector size");
        return;
    }

    LOG_INF("Sector size %u\n", block_size);

    memory_size_mb = (uint64_t)block_count * block_size;

    LOG_INF("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));

    mp.mnt_point = disk_mount_pt;

    if (fs_mount(&mp) == FR_OK)
    {
        LOG_INF("Disk mounted.\n");
    }
    else
    {
        LOG_ERR("Error mounting disk.\n");
    }
}
