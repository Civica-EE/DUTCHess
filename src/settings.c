#include <logging/log.h>
#include <settings/settings.h>

#include "settings.h"

LOG_MODULE_REGISTER(dutchess_settings);

void dutchess_settings_init ()
{
    if (settings_subsys_init())
    {
        LOG_ERR("Error during settings subsystem initialization\n");
        return;
    }

    settings_load();
}
