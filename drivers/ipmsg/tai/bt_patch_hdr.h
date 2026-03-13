/*
 * Copyright (c) 2021 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef __BT_PTACH_HDR_H_
#define __BT_PTACH_HDR_H_

#include <string.h>             // used for memset.
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define L1_TABLE_CNT_MAX 16
struct bt_patch_hdr_t {
    uint8_t MAGIC[4];
    uint8_t version;
    uint8_t size;   //hdr size
    uint8_t rsv[2];

    uint32_t len;
    uint32_t addr;
    uint32_t entry;
    uint32_t crc32;

    uint32_t L1_table_addr;
    uint16_t L1_table_bitmap;
    uint16_t rsv_1;
    uint32_t L1_table[L1_TABLE_CNT_MAX];
};

#endif // __BT_PTACH_HDR_H_
