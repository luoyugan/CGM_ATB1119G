/*
 * Compatibility unit.
 *
 * The CGMS implementation has been split into:
 * - cgms_db.c/.h
 * - cgms_meas.c/.h
 * - cgms_racp.c/.h
 * - cgms_socp.c/.h
 * - cgms_sst.c/.h
 * - atb_ble_cgms.c/.h
 */

#include "ble_super_service.h"

/*
 * Legacy Keil project currently compiles ble_super_service.c from a fixed
 * source list. To keep backward compatibility without editing project files,
 * include split implementation units here.
 */
#include "cgms_db.c"
#include "cgms_sst.c"
#include "cgms_meas.c"
#include "cgms_racp.c"
#include "cgms_socp.c"
#include "atb_ble_cgms.c"
