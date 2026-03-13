/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_TRACING_TRACING_H_
#define ZEPHYR_INCLUDE_TRACING_TRACING_H_

#include <kernel.h>

/* Below IDs are used with systemview, not final to the zephyr tracing API */
#define SYS_TRACE_ID_OFFSET                  (32u)

#define SYS_TRACE_ID_MUTEX_INIT              (1u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_MUTEX_UNLOCK            (2u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_MUTEX_LOCK              (3u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SEMA_INIT               (4u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SEMA_GIVE               (5u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SEMA_TAKE               (6u + SYS_TRACE_ID_OFFSET)
#define SYS_TRACE_ID_SLEEP                   (7u + SYS_TRACE_ID_OFFSET)

/* Below IDs are user defined IDs */
#define SYS_TRACE_ID_USR_OFFSET              (100u)

#define SYS_TRACE_ID_TP_IRQ                  (1u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_TP_READ                 (2u + SYS_TRACE_ID_USR_OFFSET)

#define SYS_TRACE_ID_KEY_READ                (3u + SYS_TRACE_ID_USR_OFFSET)

#define SYS_TRACE_ID_VSYNC                   (4u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_LCD_POST                (5u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_DE_DRAW                 (6u + SYS_TRACE_ID_USR_OFFSET)

#define SYS_TRACE_ID_GUI_INIT                (20u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_DEINIT              (21u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_CLEAR               (22u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_TASK                (23u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_INDEV_TASK          (24u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_REFR_TASK           (25u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_DISPLAY_CREATE      (26u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_DISPLAY_DELETE      (27u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_WIDGET_DRAW         (28u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_WIDGET_CREATE       (29u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_WIDGET_DELETE       (30u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_DRAW_COMPLEX        (31u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_UPDATE_LAYOUT       (32u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_EVENT               (33u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GUI_WAIT                (34u + SYS_TRACE_ID_USR_OFFSET)

#define SYS_TRACE_ID_VIEW_CREATE             (40u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_VIEW_PRELOAD            (41u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_VIEW_LAYOUT             (42u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_VIEW_PAUSE              (43u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_VIEW_RESUME             (44u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_VIEW_DELETE             (45u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_VIEW_SETPROP            (46u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_VIEW_DRAW               (47u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_VIEW_POST               (48u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_VIEW_PROC_CB            (49u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_VIEW_NOTIFY_CB          (50u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_VIEW_COMPRESS           (51u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_VIEW_DECOMPRESS         (52u + SYS_TRACE_ID_USR_OFFSET)

#define SYS_TRACE_ID_GRAPHICBUF_ALLOC        (53u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_GRAPHICBUF_FREE         (54u + SYS_TRACE_ID_USR_OFFSET)

#define SYS_TRACE_ID_VIEW_SWAPBUF            (55u + SYS_TRACE_ID_USR_OFFSET)

#define SYS_TRACE_ID_RES_SCENE_LOAD          (60u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_RES_PICS_LOAD           (61u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_RES_STRS_LOAD           (62u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_RES_BMP_LOAD_0          (63u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_RES_BMP_LOAD_1          (64u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_RES_BMP_LOAD_2          (65u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_RES_CHECK_PRELOAD       (66u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_RES_PICS_PRELOAD        (67u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_RES_UNLOAD              (68u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_RES_SCENE_PRELOAD_0     (69u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_RES_SCENE_PRELOAD_1     (70u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_RES_SCENE_PRELOAD_2     (71u + SYS_TRACE_ID_USR_OFFSET)
#define SYS_TRACE_ID_RES_SCENE_PRELOAD_3     (72u + SYS_TRACE_ID_USR_OFFSET)



#ifdef CONFIG_SEGGER_SYSTEMVIEW
#include "tracing_sysview.h"

#elif defined CONFIG_TRACING_CPU_STATS
#include "tracing_cpu_stats.h"

#define sys_trace_end_call_u32(id, retv)
#define sys_trace_u32(id, p1)
#define sys_trace_u32x2(id, p1, p2)
#define sys_trace_u32x3(id, p1, p2, p3)
#define sys_trace_u32x4(id, p1, p2, p3, p4)
#define sys_trace_u32x5(id, p1, p2, p3, p4, p5)
#define sys_trace_u32x6(id, p1, p2, p3, p4, p5, p6)
#define sys_trace_u32x7(id, p1, p2, p3, p4, p5, p6, p7)
#define sys_trace_u32x8(id, p1, p2, p3, p4, p5, p6, p7, p8)
#define sys_trace_u32x9(id, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define sys_trace_u32x10(id, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
#define sys_trace_string(id, string)
#define sys_trace_string_u32x5(id, string, p1, p2, p3, p4, p5)

#elif defined CONFIG_TRACING_CTF
#include "tracing_ctf.h"

#define sys_trace_end_call_u32(id, retv)
#define sys_trace_u32(id, p1)
#define sys_trace_u32x2(id, p1, p2)
#define sys_trace_u32x3(id, p1, p2, p3)
#define sys_trace_u32x4(id, p1, p2, p3, p4)
#define sys_trace_u32x5(id, p1, p2, p3, p4, p5)
#define sys_trace_u32x6(id, p1, p2, p3, p4, p5, p6)
#define sys_trace_u32x7(id, p1, p2, p3, p4, p5, p6, p7)
#define sys_trace_u32x8(id, p1, p2, p3, p4, p5, p6, p7, p8)
#define sys_trace_u32x9(id, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define sys_trace_u32x10(id, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
#define sys_trace_string(id, string)
#define sys_trace_string_u32x5(id, string, p1, p2, p3, p4, p5)

#elif defined CONFIG_TRACING_TEST
#include "tracing_test.h"

#define sys_trace_end_call_u32(id, retv)
#define sys_trace_u32(id, p1)
#define sys_trace_u32x2(id, p1, p2)
#define sys_trace_u32x3(id, p1, p2, p3)
#define sys_trace_u32x4(id, p1, p2, p3, p4)
#define sys_trace_u32x5(id, p1, p2, p3, p4, p5)
#define sys_trace_u32x6(id, p1, p2, p3, p4, p5, p6)
#define sys_trace_u32x7(id, p1, p2, p3, p4, p5, p6, p7)
#define sys_trace_u32x8(id, p1, p2, p3, p4, p5, p6, p7, p8)
#define sys_trace_u32x9(id, p1, p2, p3, p4, p5, p6, p7, p8, p9)
#define sys_trace_u32x10(id, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
#define sys_trace_string(id, string)
#define sys_trace_string_u32x5(id, string, p1, p2, p3, p4, p5)

#else

/**
 * @brief Tracing APIs
 * @defgroup tracing_apis Tracing APIs
 * @{
 */
/**
 * @brief Called before a thread has been selected to run
 */
#define sys_trace_thread_switched_out()

/**
 * @brief Called after a thread has been selected to run
 */
#define sys_trace_thread_switched_in()

/**
 * @brief Called when setting priority of a thread
 * @param thread Thread structure
 */
#define sys_trace_thread_priority_set(thread)

/**
 * @brief Called when a thread is being created
 * @param thread Thread structure
 */
#define sys_trace_thread_create(thread)

/**
 * @brief Called when a thread is being aborted
 * @param thread Thread structure
 *
 */
#define sys_trace_thread_abort(thread)

/**
 * @brief Called when a thread is being suspended
 * @param thread Thread structure
 */
#define sys_trace_thread_suspend(thread)

/**
 * @brief Called when a thread is being resumed from suspension
 * @param thread Thread structure
 */
#define sys_trace_thread_resume(thread)

/**
 * @brief Called when a thread is ready to run
 * @param thread Thread structure
 */
#define sys_trace_thread_ready(thread)

/**
 * @brief Called when a thread is pending
 * @param thread Thread structure
 */
#define sys_trace_thread_pend(thread)

/**
 * @brief Provide information about specific thread
 * @param thread Thread structure
 */
#define sys_trace_thread_info(thread)

/**
 * @brief Called when a thread name is set
 * @param thread Thread structure
 */
#define sys_trace_thread_name_set(thread)

/**
 * @brief Called when entering an ISR
 */
#define sys_trace_isr_enter()

/**
 * @brief Called when exiting an ISR
 */
#define sys_trace_isr_exit()

/**
 * @brief Called when exiting an ISR and switching to scheduler
 */
#define sys_trace_isr_exit_to_scheduler()

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 */
#define sys_trace_void(id)

/**
 * @brief Can be called with any id signifying ending a call
 * @param id ID of the operation that was completed
 */
#define sys_trace_end_call(id)

/**
 * @brief Can be called with any id signifying ending a call
 * @param id ID of the operation that was completed
 * @param retv return value of the operation that was completed
 */
#define sys_trace_end_call_u32(id, retv)

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 * @param p1 1st integer parameter
 */
#define sys_trace_u32(id, p1)

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 * @param p1 1st integer parameter
 * @param p2 2nd integer parameter
 */
#define sys_trace_u32x2(id, p1, p2)

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 * @param p1 1st integer parameter
 * @param p2 2nd integer parameter
 * @param p3 3rd integer parameter
 */
#define sys_trace_u32x3(id, p1, p2, p3)

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 * @param p1 1st integer parameter
 * @param p2 2nd integer parameter
 * @param p3 3rd integer parameter
 * @param p4 4th integer parameter
 */
#define sys_trace_u32x4(id, p1, p2, p3, p4)

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 * @param p1 1st integer parameter
 * @param p2 2nd integer parameter
 * @param p3 3rd integer parameter
 * @param p4 4th integer parameter
 * @param p5 5th integer parameter
 */
#define sys_trace_u32x5(id, p1, p2, p3, p4, p5)

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 * @param p1 1st integer parameter
 * @param p2 2nd integer parameter
 * @param p3 3rd integer parameter
 * @param p4 4th integer parameter
 * @param p5 5th integer parameter
 * @param p6 6th integer parameter
 */
#define sys_trace_u32x6(id, p1, p2, p3, p4, p5, p6)

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 * @param p1 1st integer parameter
 * @param p2 2nd integer parameter
 * @param p3 3rd integer parameter
 * @param p4 4th integer parameter
 * @param p5 5th integer parameter
 * @param p6 6th integer parameter
 * @param p7 7th integer parameter
 */
#define sys_trace_u32x7(id, p1, p2, p3, p4, p5, p6, p7)

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 * @param p1 1st integer parameter
 * @param p2 2nd integer parameter
 * @param p3 3rd integer parameter
 * @param p4 4th integer parameter
 * @param p5 5th integer parameter
 * @param p6 6th integer parameter
 * @param p7 7th integer parameter
 * @param p8 8th integer parameter
 */
#define sys_trace_u32x8(id, p1, p2, p3, p4, p5, p6, p7, p8)

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 * @param p1 1st integer parameter
 * @param p2 2nd integer parameter
 * @param p3 3rd integer parameter
 * @param p4 4th integer parameter
 * @param p5 5th integer parameter
 * @param p6 6th integer parameter
 * @param p7 7th integer parameter
 * @param p8 8th integer parameter
 * @param p9 9th integer parameter
 */
#define sys_trace_u32x9(id, p1, p2, p3, p4, p5, p6, p7, p8, p9)

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 * @param p1 1st integer parameter
 * @param p2 2nd integer parameter
 * @param p3 3rd integer parameter
 * @param p4 4th integer parameter
 * @param p5 5th integer parameter
 * @param p6 6th integer parameter
 * @param p7 7th integer parameter
 * @param p8 8th integer parameter
 * @param p9 9th integer parameter
 * @param p10 10th integer parameter
 */
#define sys_trace_u32x10(id, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 * @param string string parameter
 */
#define sys_trace_string(id, string)

/**
 * @brief Can be called with any id signifying a new call
 * @param id ID of the operation that was started
 * @param string string parameter
 * @param p1 1st integer parameter
 * @param p2 2nd integer parameter
 * @param p3 3rd integer parameter
 * @param p4 4th integer parameter
 * @param p5 5th integer parameter
 */
#define sys_trace_string_u32x5(id, string, p1, p2, p3, p4, p5)

/**
 * @brief Called when the cpu enters the idle state
 */
#define sys_trace_idle()

/**
 * @brief Trace initialisation of a Semaphore
 * @param sem Semaphore object
 */
#define sys_trace_semaphore_init(sem)

/**
 * @brief Trace taking a Semaphore
 * @param sem Semaphore object
 */
#define sys_trace_semaphore_take(sem)

/**
 * @brief Trace giving a Semaphore
 * @param sem Semaphore object
 */
#define sys_trace_semaphore_give(sem)

/**
 * @brief Trace initialisation of a Mutex
 * @param mutex  Mutex object
 */
#define sys_trace_mutex_init(mutex)

/**
 * @brief Trace locking a Mutex
 * @param mutex Mutex object
 */
#define sys_trace_mutex_lock(mutex)

/**
 * @brief Trace unlocking a Mutex
 * @param mutex Mutex object
 */
#define sys_trace_mutex_unlock(mutex)
/**
 * @}
 */

#endif
#endif
