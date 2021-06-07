#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/eeprom.h>
#include <device.h>
#include "tftp.h"
const struct device *dev = NULL;
size_t eeprom_size;

int eepromOpen(char *filename)
{
    unsigned char buf[1];
    return eeprom_read( dev, 0 , buf, 1);
}
int eepromRead(unsigned int *addr, unsigned char *buf, unsigned int len)
{
    if ( *addr > eeprom_size )
        return 0;
    if (eeprom_read( dev, *addr, buf, len)<0 )
        return 0;
    *addr+=len;
    return len;
}
int eepromWrite(unsigned int *addr, unsigned char *buf, unsigned int len)
{
    if (eeprom_write( dev, *addr, buf, len)<0)
        return 0;
    *addr+=len;
    return len;
}

tftpHandler eepromHandler=
{
    .id="eeprom",
    .open=eepromOpen,
    .read=eepromRead,
    .write=eepromWrite,
};


void dut_tftp_eeprom(void)
{
    dev = DEVICE_DT_GET(DT_ALIAS(eeprom_0));
    if (dev == NULL) {
        printk("\nError: EEPROM with alias eeprom_0 not found.\n");
        return ;
    }

    if (!device_is_ready(dev)) {
        printk("\nError: Device \"%s\" is not ready; "
                "check the driver initialization logs for errors.\n",
                dev->name);
        return ;
    }
    eeprom_size = eeprom_get_size( dev);

    printk("Found EEPROM device \"%s\"\n", dev->name);
    registerHandler(&eepromHandler);

}

