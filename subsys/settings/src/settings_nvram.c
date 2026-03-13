/*
 * Copyright (c) 2019 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <drivers/nvram_config.h>
#include <string.h>
#include <sys/printk.h>

#include "settings/settings.h"
#include "settings_priv.h"

static int settings_nvram_load(struct settings_store *cs, const struct settings_load_arg *arg);
static int settings_nvram_save(struct settings_store *cs, const char *name,
			     const char *value, size_t val_len);

static const struct settings_store_itf settings_nvram_itf = {
	.csi_load = settings_nvram_load,
	.csi_save = settings_nvram_save,
};

static struct settings_store settings_nvram_store = {
    .cs_itf = &settings_nvram_itf
};

static ssize_t settings_nvram_read_cb(void *cb_arg, void *data, size_t len)
{
    /* just copy nvram config data */
    memcpy(data, cb_arg, len);
    return len;
}

static int settings_nvram_load_cb(char *name, char *val, uint16_t val_len, void *arg)
{
	struct settings_load_arg *argp;
	int ret;

	argp = (struct settings_load_arg *)arg;

	printk("nvram load: name %s, len %u\n", name, val_len);
	ret = settings_call_set_handler(name, val_len,
		settings_nvram_read_cb, val, (void *)arg);

	return ret;
}

static int settings_nvram_load(struct settings_store *cs,
			    const struct settings_load_arg *arg)
{
	char name[SETTINGS_MAX_NAME_LEN];
    char val[SETTINGS_MAX_VAL_LEN];
	int rc;

	rc = nvram_config_load(name, val,
	    settings_nvram_load_cb, (void *)arg);
	if (rc) {
		return -EINVAL;
	}

	return 0;
}

static int settings_nvram_save(struct settings_store *cs, const char *name,
			     const char *value, size_t val_len)
{
	ARG_UNUSED(cs);

	if (!name) {
		return -EINVAL;
	}

	printk("nvram set: name %s, len %u\n", name, val_len);

	if (!value) {
		return nvram_config_set(name, value, 0);
	}

	return nvram_config_set(name, value, val_len);
}

int settings_backend_init(void)
{
    settings_dst_register((struct settings_store *)&settings_nvram_store);
    settings_src_register((struct settings_store *)&settings_nvram_store);
    return 0;
}
