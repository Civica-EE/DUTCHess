#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <shell/shell.h>
#include <drivers/sensor.h>
#include <libc_extensions.h>

#include "web.h"
#include "DUTCHess.h"
#include "dut_serial.h"
#include "tftp.h"

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
static int cmd_dut_power(const struct shell *shell, size_t argc, char **argv)
{
	if ( argc != 1 )
	{
		shell_print(shell,"Usage: Please supply on/off as a parameter\n");
	}
	else
	{
		if ( strcmp(argv[1], "on"))
		{
			relayon();
			shell_print(shell,"Power ON\n");
		}
		else if ( strcmp(argv[1], "off"))
		{
			relayoff();
			shell_print(shell,"Power OFF\n");
		}
		else
		{
			shell_print(shell,"Please supply on/off\n");
		}
	}
        return 0;
}

double getTemperature(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(nxp_pct2075);
	struct sensor_value temp;
	if (!device_is_ready(dev)) {
		//shell_print(shell,"Device %s is not ready.\n", dev->name);
		return 0;
	}
	int	rc = sensor_sample_fetch(dev);
	if (rc != 0) {
		//shell_print(shell,"sensor_sample_fetch error: %d\n", rc);
		return 0;
	}


	rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
	if (rc != 0) {
		//shell_print(shell,"sensor_channel_get error: %d\n", rc);
		return 0;
	}

	return sensor_value_to_double(&temp);
}
static int cmd_dut_temperature(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell,"Temp %g C\n", getTemperature());

	return 0;
}

/* Creating subcommands (level 1 command) array for command "demo". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_dut_power,
        SHELL_CMD(power,   NULL, "Power command.", cmd_dut_power),
        SHELL_CMD(temperature,   NULL, "Temperature command.", cmd_dut_temperature),
        SHELL_SUBCMD_SET_END
);

/* Creating root (level 0) command "dut" */
SHELL_CMD_REGISTER(dut, &sub_dut_power, "DUTCHess commands", NULL);

// This should come from nvram (see Zephyr:settings)
static struct dut_serial_cfg dut_serial_cfg[] = {
    {
        .dev_name = "UART_2",
        .cfg = {
            .baudrate = 115200,
            .parity = UART_CFG_PARITY_NONE,
            .stop_bits = 1,
            .data_bits = 8,
            .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
        },
       .tcp_port = 21500
    },
        
    {
        .dev_name = "UART_2",
        .cfg = {
            .baudrate = 57600,
            .parity = UART_CFG_PARITY_NONE,
            .stop_bits = 1,
            .data_bits = 8,
            .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
        },
        .tcp_port = 25700
    },
        
    DUT_SERIAL_LAST
};


void main (void)
{
#ifdef CONFIG_BOARD_MIMXRT1020_EVK    
    const struct device *dev;

    if ((dev = device_get_binding(LED0)) == NULL)
    {
        return;
    }

    if (gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS) < 0)
    {
        return;
    }
#endif

    webStart();

    dut_serial_start(dut_serial_cfg);
    dut_start_tftpServer();
#ifdef CONFIG_BOARD_MIMXRT1020_EVK
    while (true)
    {
        gpio_pin_set(dev, PIN, !gpio_pin_get(dev, PIN));
        k_msleep(SLEEP_TIME_MS);
    }
#endif
}
