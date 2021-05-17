#define DT_DRV_COMPAT nxp_pct2075

#include <errno.h>

#include <kernel.h>
#include <drivers/i2c.h>
#include <init.h>

#include <drivers/sensor.h>

#include <init.h>
#include <sys/byteorder.h>
#include <sys/__assert.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(pct2075, CONFIG_SENSOR_LOG_LEVEL);
struct pct2075_data {
	const struct device *i2c_master;

	uint16_t reg_val;

};

struct pct2075_config {
	const char *i2c_bus;
	uint16_t i2c_addr;
};

#define PCT2075_TEMP_SCALE_CEL 		8
#define PCT2075_TEMP_SIGN_BIT		BIT(11)
#define PCT2075_TEMP_ABS_MASK		((uint16_t)(PCT2075_TEMP_SIGN_BIT - 1U))

int pct2075_reg_read(const struct device *dev, uint8_t reg, uint16_t *val)
{
	struct pct2075_data *data = dev->data;
	const struct pct2075_config *cfg = dev->config;
	int rc = i2c_write_read(data->i2c_master, cfg->i2c_addr,
				&reg, sizeof(reg),
				val, sizeof(*val));

	if (rc == 0) {
		*val = sys_be16_to_cpu(*val);
	}
	// 16 bits are d10,d9,d..d0,0,0,0,0,0
	*val >>=5; // shift down to lsb
	return rc;
}

static int pct2075_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct pct2075_data *data = dev->data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	return pct2075_reg_read(dev, 0, &data->reg_val);
}

static inline int pct2075_temp_signed_from_reg(uint16_t reg)
{
	int rv = reg & PCT2075_TEMP_ABS_MASK;

	if (reg & PCT2075_TEMP_SIGN_BIT) {
		/* Convert 12-bit 2s complement to signed negative
		 * value.
		 */
		rv = -(1U + (rv ^ PCT2075_TEMP_ABS_MASK));
	}
	return rv;
}

static int pct2075_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct pct2075_data *data = dev->data;
	int temp = pct2075_temp_signed_from_reg(data->reg_val);

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP);

	val->val1 = temp / PCT2075_TEMP_SCALE_CEL;
	temp -= val->val1 * PCT2075_TEMP_SCALE_CEL;
	val->val2 = (temp * 1000000) / PCT2075_TEMP_SCALE_CEL;

	return 0;
}

static const struct sensor_driver_api pct2075_api_funcs = {
	.sample_fetch = pct2075_sample_fetch,
	.channel_get = pct2075_channel_get,
};

int pct2075_init(const struct device *dev)
{
	struct pct2075_data *data = dev->data;
	const struct pct2075_config *cfg = dev->config;
	int rc = 0;

	data->i2c_master = device_get_binding(cfg->i2c_bus);
	if (!data->i2c_master) {
		LOG_ERR("pct2075: i2c master not found: %s", cfg->i2c_bus);
		return -EINVAL;
	}

	return rc;
}

static struct pct2075_data pct2075_data;
static const struct pct2075_config pct2075_cfg = {
	.i2c_bus = DT_INST_BUS_LABEL(0),
	.i2c_addr = DT_INST_REG_ADDR(0)
};

DEVICE_DT_INST_DEFINE(0, pct2075_init, NULL,
		      &pct2075_data, &pct2075_cfg, POST_KERNEL,
		      CONFIG_SENSOR_INIT_PRIORITY, &pct2075_api_funcs);
