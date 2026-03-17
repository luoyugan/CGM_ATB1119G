/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file sensor device driver interface
 */

#ifndef __INCLUDE_SENSOR_H__
#define __INCLUDE_SENSOR_H__

#include <stdint.h>
#include <device.h>



#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup sensor_apis Sensor-temperature APIs
 * @ingroup driver_apis
 * @{
 */

/** sensor property enum */
enum sensor_property {
	/** get sensor temperature */
	SENSOR_PROP_TEMPERATURE,
};

/** sensor property value */
union sensor_propval {
	/** property value */
	int intval;
	/** property string */
	const char *strval;
};


/** sensor driver api */
struct sensor_driver_api {
	/**< get property from sensor driver */
	int (*get_property)(struct device *dev, enum sensor_property psp,
				union sensor_propval *val);
	/**< enable sensor */
	void (*enable)(struct device *dev);
	/**< disable sensor */
	void (*disable)(struct device *dev);
};


/**
 * @brief get property from power supply driver
 *
 * This routine calls to get information from power supply driver
 *
 * Example:
 *
 * @code
 *	dev = device_get_binding(CONFIG_ACTS_BATTERY_DEV_NAME);
 *	if (!dev) {
 *		SYS_LOG_ERR("cannot found battery device");
 *		return -ENODEV;
 *	} 
 *  psp = POWER_SUPPLY_PROP_CAPACITY;
 *	ret = power_supply_get_property(dev, psp, &val);
 *	if (ret < 0) {
 *		SYS_LOG_ERR("cannot get property, ret %d", ret);
 *		return -EINVAL;
 *	} 
 * @endcode
 *
 * @param dev pointer to the battery device.
 * @param psp the property need to get from driver
 * @param val pointer to the inf getting from driver
 *
 * @return 0 : succsess.  others: fail
 */

static inline int sensor_get_property(struct device *dev, enum sensor_property psp,
				union sensor_propval *val)
{
	const struct sensor_driver_api *api = dev->api;

	return api->get_property(dev, psp, val);
}


/**
 * @brief enable battery charge
 *
 * This routine calls to enable battery charge.
 *
 * Example:
 *
 * @code
 *	dev = device_get_binding(CONFIG_ACTS_BATTERY_DEV_NAME);
 *	if (!dev) {
 *		SYS_LOG_ERR("cannot found battery device");
 *		return -ENODEV;
 *	} 
 *  
 *	power_supply_enable(dev);
 *
 * @endcode
 *
 * @param dev pointer to the battery device.
 */

static inline void sensor_enable(struct device *dev)
{
	const struct sensor_driver_api *api = dev->api;

	api->enable(dev);
}


/**
 * @brief disable battery charge
 *
 * This routine calls to disable battery charge.
 *
 * Example:
 *
 * @code
 *	dev = device_get_binding(CONFIG_ACTS_BATTERY_DEV_NAME);
 *	if (!dev) {
 *		SYS_LOG_ERR("cannot found battery device");
 *		return -ENODEV;
 *	} 
 *  
 *	power_supply_disable(dev);
 *
 * @endcode
 *
 * @param dev pointer to the battery device.
 *
 */

static inline void sensor_disable(struct device *dev)
{
	const struct sensor_driver_api *api = dev->api;

	api->disable(dev);
}

/**
 * @} end defgroup sensor_apis
 */

#ifdef __cplusplus
}
#endif

#endif  /* __INCLUDE_SENSOR_H__ */
