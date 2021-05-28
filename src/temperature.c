#include <drivers/sensor.h>
#include <logging/log.h>



#include "temperature.h"

LOG_MODULE_REGISTER(dutchess_temperature);

static const struct device *dev = NULL;

double dutchess_temperature_read ()
{
    int rc = 0;
    struct sensor_value temp;

    if (!device_is_ready(dev))
    {
        LOG_ERR("Device not ready!");
        return 0;
    }

    if ((rc = sensor_sample_fetch(dev)) != 0)
    {
        LOG_ERR("Error fetching sample!");
        return 0;
    }


    if ((rc = sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp)) != 0)
    {
        LOG_ERR("Error getting channel!");
        return 0;
    }

    return sensor_value_to_double(&temp);
}

void dutchess_temperature_init ()
{
    dev = DEVICE_DT_GET_ANY(nxp_pct2075);
}
