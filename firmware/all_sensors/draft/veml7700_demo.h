#ifndef VEML7700_DEMO_H
#define VEML7700_DEMO_H

#include "driver/i2c_master.h"

void veml7700_demo_start(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t oled_handle);
void veml7700_demo_stop(void);

#endif // VEML7700_DEMO_H
