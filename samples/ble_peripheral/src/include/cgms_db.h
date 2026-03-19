#ifndef CGMS_DB_H
#define CGMS_DB_H

#include <stdbool.h>
#include <stdint.h>

#include "atb_ble_cgms.h"

#define CGMS_DB_MAX_RECORDS 100

uint8_t cgms_encode_feature(uint8_t *buf);
uint8_t cgms_encode_status(uint8_t *buf);
uint8_t cgms_encode_sst(uint8_t *buf);
// uint8_t cgms_encode_measurement(const struct cgms_record *rec, uint8_t *buf);

void put_le16(uint8_t *dst, uint16_t value);
uint16_t get_le16(const uint8_t *src);
bool cgms_feature_present(uint32_t feature);
void cgms_racp_reset_state(void);
uint8_t cgms_normalize_comm_interval(uint8_t interval);
// int cgms_record_index_offset_less_or_equal_get(uint16_t offset, uint16_t *record_num);
// int cgms_record_index_offset_greater_or_equal_get(uint16_t offset, uint16_t *record_num);
// void cgms_record_add(const struct cgms_record *rec);

uint16_t cgms_db_num_records_get(void);
int32_t cgms_db_init(void);
int32_t cgms_db_record_get(uint8_t record_num, ble_cgms_rec_t * p_rec);
int32_t cgms_db_record_add(ble_cgms_rec_t * p_rec);
int32_t cgms_db_record_delete(uint8_t record_num);

#endif