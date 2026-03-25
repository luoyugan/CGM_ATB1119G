#ifndef ATB_BLE_STD_SERVICES_H
#define ATB_BLE_STD_SERVICES_H

#include <stdint.h>

void atb_std_services_init(void);
uint8_t atb_bas_get_level(void);
int atb_bas_set_level(uint8_t level);

#endif
