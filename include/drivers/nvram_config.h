/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NVRAM config driver interface
 */

#ifndef __INCLUDE_NVRAM_CONFIG_H__
#define __INCLUDE_NVRAM_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <shell/shell.h>

int nvram_config_get(const char *name, void *data, int max_len);
int nvram_config_set(const char *name, const void *data, int len);
int nvram_config_clear(int len);
int nvram_config_clear_all(void);
void nvram_config_dump(const struct shell *shell);

int nvram_config_get_factory(const char *name, void *data, int max_len);
int nvram_config_set_factory(const char *name, const void *data, int len);

typedef int (*nvram_config_load_cb)(char *name, char *val, uint16_t val_len, void *arg);
int nvram_config_load(char *name, char *val,
		nvram_config_load_cb cb, void *cb_arg);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* __INCLUDE_NVRAM_CONFIG_H__ */
