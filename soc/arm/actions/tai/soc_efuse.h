/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPICACHE profile interface for Actions SoC
 */

#ifndef __EFUSE_H__
#define __EFUSE_H__

#include <zephyr/types.h>

/**
 *  @brief check whether ft has been done
 *  @return returns FT_DONE (0)
 *          returns NO_FT (-1)
 */
int check_efuse_ft_flag(void);

/**
 *  @brief get hosc trim from efuse
 *  @return returns SUCCESS (0)
 *          returns FAIL (-1)
 * @param val IN -- hosc trim val (6bits)
 */
int get_efuse_hosc_trim(uint32_t *val);

/**
 *  @brief get vddret_cal07 from efuse
 *  @return returns SUCCESS (0)
 *          returns FAIL (-1)
 * @param val IN -- vddret_cal07 val (4bits)
 */
int get_efuse_vddret_cal07(uint8_t *val);

/**
 *  @brief get sensor trim from efuse
 *  @return returns SUCCESS (0)
 *          returns FAIL (-1)
 * @param vref_val IN -- sensor v6ref val (5bits)
          offset_val IN -- sensor offset val (5bits) 
 */
int get_efuse_sensor_trim(uint8_t *vref_val, uint8_t *offset_val);


#endif	/* __EFUSE_H__ */
