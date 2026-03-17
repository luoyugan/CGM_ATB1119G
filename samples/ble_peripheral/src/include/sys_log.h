/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SYS_LOG_H
#define __SYS_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <logging/log.h>

#define SYS_LOG_DBG LOG_DBG
#define SYS_LOG_INF LOG_INF
#define SYS_LOG_WRN LOG_WRN
#define SYS_LOG_ERR LOG_ERR

#ifdef __cplusplus
}
#endif

#endif
