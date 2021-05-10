#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#include "web.h"
#include "DUTCHess.h"

#define SLEEP_TIME_MS 500

#define LED0_NODE DT_ALIAS(led0)
#define LED0 DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS DT_GPIO_FLAGS(LED0_NODE, gpios)


bool relay=false; // off
bool relaystate(void)
{
	return relay;
}

void relayon(void)
{
	// turn the relay on
	relay=true;
	// TODO set the gpio output
}
void relayoff(void)
{
	// turn the relay off
	relay=false;
	// TODO set the gpio output
}

storeData_t globalStore=
{
	.ip=IP(192,168,50,2),
	.gateway=IP(192,168,50,1),
	.netmask=IP(255,255,255,0),
	.defaultRelayState=true,
	.controllerIP=IP(192,168,50,50),
	.controllerPort=12345,
};

storeData_t *getStoreData(void)
{
	return &globalStore;
}
unsigned long getIP(char *value)
{
	unsigned long a,b,c,d;
	a=b=c=d=0;
	sscanf(value,"%ld.%ld.%ld.%ld",&a,&b,&c,&d);
	return  IP(a,b,c,d);
}

void updateStore(char *var, char *value, int valueLen)
{
	// process the variables
	if ( valueLen <= 0 )
		return; //either an error or the value is blank
	if ( strcmp(var,"ip") == 0 )
	{
		globalStore.ip=getIP(value);
	}
	if ( strcmp(var,"gateway") == 0 )
	{
		globalStore.gateway=getIP(value);
	}
	if ( strcmp(var,"mask") == 0 )
	{
		globalStore.netmask=getIP(value);
	}
	if ( strcmp(var,"relaydefault") == 0 )
	{
		if ( value[0]=='0')
			globalStore.defaultRelayState=false;
		else
			globalStore.defaultRelayState=true;// any other value will enable this...
		
	}
	if ( strcmp(var,"controller") == 0 )
	{
		globalStore.controllerIP=getIP(value);
	}
	if ( strcmp(var,"port") == 0 )
	{
		globalStore.controllerPort=atoi(value);
	}
}
void saveStore(void)
{
	// save to flash...
}

void main (void)
{
    const struct device *dev;

    if ((dev = device_get_binding(LED0)) == NULL)
    {
        return;
    }

    if (gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS) < 0)
    {
        return;
    }

    webStart();

    while (true)
    {
        gpio_pin_set(dev, PIN, !gpio_pin_get(dev, PIN));
        k_msleep(SLEEP_TIME_MS);
    }
}
