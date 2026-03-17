/* Fix for AC-6.9 */
#ifndef __GNUC__
#define __GNUC__	9
#endif

/* ROM layout */
#define _image_ksdfs_start				__ksdfs_start
#define _image_rom_start					Image$$ER_VECTOR$$Base

#define _vector_start							Image$$ER_VECTOR$$Base
#define _vector_end								Image$$ER_VECTOR$$Limit

#define __app_entry_table					Image$$ER_APP_ENTRY$$Base
#define __app_entry_end						Image$$ER_APP_ENTRY$$Limit
#define __service_entry_table				Image$$ER_SRV_ENTRY$$Base
#define __service_entry_end					Image$$ER_SRV_ENTRY$$Limit
#define __view_entry_table					Image$$ER_VIEW_ENTRY$$Base
#define __view_entry_end					Image$$ER_VIEW_ENTRY$$Limit

#define _image_text_start					Image$$ER_TEXT$$Base
#define _image_text_end						Image$$ER_TEXT$$Limit

#define __start_unwind_idx						Image$$ER_ARM_EXIDX$$Base
#define __stop_unwind_idx						Image$$ER_ARM_EXIDX$$Limit

#define __exidx_start							Image$$ER_ARM_EXIDX$$Base
#define __exidx_end								Image$$ER_ARM_EXIDX$$Limit

#define _image_rodata_start				Image$$ER_INIT_PRE_KERNEL_1$$Base

#define __init_start							Image$$ER_INIT_PRE_KERNEL_1$$Base
#define __init_PRE_KERNEL_1_start	Image$$ER_INIT_PRE_KERNEL_1$$Base
#define __init_PRE_KERNEL_2_start	Image$$ER_INIT_PRE_KERNEL_2$$Base
#define __init_POST_KERNEL_start	Image$$ER_INIT_POST_KERNEL$$Base
#define __init_APPLICATION_start	Image$$ER_INIT_APPLICATION$$Base
#define __init_SMP_start					Image$$ER_INIT_SMP$$Base
#define __init_end								Image$$ER_INIT_SMP$$Limit

#define __app_shmem_regions_start	Image$$ER_APP_SHMEM$$Base
#define __app_shmem_regions_end		Image$$ER_APP_SHMEM$$Limit

#define __net_l2_start						Image$$ER_NET_12$$Base
#define __net_l2_end							Image$$ER_NET_12$$Limit

#define _bt_l2cap_fixed_chan_list_start			Image$$ER_BT_CHANNELS$$Base
#define _bt_l2cap_fixed_chan_list_end				Image$$ER_BT_CHANNELS$$Limit

#define _bt_l2cap_br_fixed_chan_list_start			Image$$ER_BT_BR_CHANNELS$$Base
#define _bt_l2cap_br_fixed_chan_list_end			Image$$ER_BT_BR_CHANNELS$$Limit

#define _bt_gatt_service_static_list_start	Image$$ER_BT_SERVICES$$Base
#define _bt_gatt_service_static_list_end		Image$$ER_BT_SERVICES$$Limit

#define _settings_handler_static_list_start		Image$$ER_SETTINGS_HANDLERS$$Base
#define _settings_handler_static_list_end		Image$$ER_SETTINGS_HANDLERS$$Limit

#define __log_const_start					Image$$ER_LOG_CONST$$Base
#define __log_const_end						Image$$ER_LOG_CONST$$Limit

#define __log_backends_start			Image$$ER_LOG_BACKENDS$$Base
#define __log_backends_end				Image$$ER_LOG_BACKENDS$$Limit

#define _shell_list_start					Image$$ER_SHELL$$Base
#define _shell_list_end						Image$$ER_SHELL$$Limit

#define __shell_root_cmds_start		Image$$ER_SHELL_ROOT_CMDS$$Base
#define __shell_root_cmds_end			Image$$ER_SHELL_ROOT_CMDS$$Limit

#define __font_entry_start				Image$$ER_FONT_ENTRY$$Base
#define __font_entry_end					Image$$ER_FONT_ENTRY$$Limit

#define _tracing_backend_list_start					Image$$ER_TRACING_BACKENDS$$Base
#define _tracing_backend_list_end						Image$$ER_TRACING_BACKENDS$$Limit

#define _image_rodata_end					Image$$ER_RODATA$$Limit
#define _image_rom_end						Image$$ER_RODATA$$Limit

#define _flash_used							Load$$LR$$LR1$$Length

/* RAM layout */
#define _image_ram_start					Image$$ER_SW_ISR_TABLE$$Base

#define _ramfunc_ram_start				Image$$ER_RAMFUNC$$Base
#define _ramfunc_ram_end					Image$$ER_RAMFUNC$$Limit
#define _ramfunc_ram_size					Image$$ER_RAMFUNC$$Length
#define _ramfunc_rom_start				Load$$ER_RAMFUNC$$Base

#define __data_ram_start					Image$$ER_DATAS$$Base
#define __data_rom_start					Load$$ER_DATAS$$Base

#define __device_start											Image$$ER_DEVICE_PRE_KERNEL_1$$Base
#define __device_PRE_KERNEL_1_start					Image$$ER_DEVICE_PRE_KERNEL_1$$Base
#define __device_PRE_KERNEL_2_start					Image$$ER_DEVICE_PRE_KERNEL_2$$Base
#define __device_POST_KERNEL_start					Image$$ER_DEVICE_POST_KERNEL$$Base
#define __device_APPLICATION_start					Image$$ER_DEVICE_APPLICATION$$Base
#define __device_SMP_start									Image$$ER_DEVICE_SMP$$Base
#define __device_end												Image$$ER_DEVICE_SMPX$$Limit
#define __device_init_status_start					Image$$ER_DEVICE_INIT_STATUS$$Base
#define __device_init_status_end						Image$$ER_DEVICE_INIT_STATUS$$Limit
#define __device_busy_start				Image$$ER_DEVICE_BUSY$$Base
#define __device_busy_end					Image$$ER_DEVICE_BUSY$$Limit

#define __shell_module_start			Image$$ER_INITSHELL$$Base

#define __log_dynamic_start				Image$$ER_LOG_DYNAMIC$$Base
#define __log_dynamic_end					Image$$ER_LOG_DYNAMIC$$Limit

#define __static_thread_data_list_start			Image$$ER_STATIC_THREAD$$Base
#define __static_thread_data_list_end				Image$$ER_STATIC_THREAD$$Limit

#define _k_timer_list_start				Image$$ER_K_TIMER$$Base
#define _k_timer_list_end					Image$$ER_K_TIMER$$Limit

#define _k_mem_slab_list_start		Image$$ER_K_MEM_SLAB$$Base
#define _k_mem_slab_list_end			Image$$ER_K_MEM_SLAB$$Limit

#define _k_mem_pool_list_start		Image$$ER_K_MEM_POOL$$Base
#define _k_mem_pool_list_end			Image$$ER_K_MEM_POOL$$Limit

#define _k_heap_list_start				Image$$ER_K_HEAP$$Base
#define _k_heap_list_end					Image$$ER_K_HEAP$$Limit

#define _k_sem_list_start					Image$$ER_K_SEM$$Base
#define _k_sem_list_end						Image$$ER_K_SEM$$Limit

#define _k_mutex_list_start				Image$$ER_K_MUTEX$$Base
#define _k_mutex_list_end					Image$$ER_K_MUTEX$$Limit

#define _k_queue_list_start				Image$$ER_K_QUEUE$$Base
#define _k_queue_list_end					Image$$ER_K_QUEUE$$Limit

#define _k_stack_list_start				Image$$ER_K_STACK$$Base
#define _k_stack_list_end					Image$$ER_K_STACK$$Limit

#define _k_msgq_list_start				Image$$ER_K_MSGQ$$Base
#define _k_msgq_list_end					Image$$ER_K_MSGQ$$Limit

#define _k_mbox_list_start				Image$$ER_K_MBOX$$Base
#define _k_mbox_list_end					Image$$ER_K_MBOX$$Limit

#define _k_pipe_list_start				Image$$ER_K_PIPE$$Base
#define _k_pipe_list_end					Image$$ER_K_PIPE$$Limit

#define _net_buf_pool_list				Image$$ER_NET_BUF_POOL$$Base
#define _net_buf_pool_list_end			Image$$ER_NET_BUF_POOL$$Limit

#define __net_if_start						Image$$ER_NET_IF$$Base
#define __net_if_end							Image$$ER_NET_IF$$Limit

#define __net_if_dev_start				Image$$ER_NET_IF_DEV$$Base
#define __net_if_dev_end					Image$$ER_NET_IF_DEV$$Limit

#define __net_l2_data_start				Image$$ER_NET_L2_DATA$$Base
#define __net_l2_data_end					Image$$ER_NET_L2_DATA$$Limit

#define __data_ram_end						Image$$ER_NET_L2_DATA$$Limit

#define __bss_start								Image$$ER_BSS$$Base
#define __bss_end									Image$$ER_BSS$$ZI$$Limit

#define __stack_start								Image$$ER_STACK$$Base
#define __stack_end									Image$$ER_STACK$$Limit

#define __ret_ram2_end								Image$$ER_RAM_END$$Base

#define __psram_bss_start								Image$$ER_BSS$$Base
#define __psram_bss_end									Image$$ER_BSS$$ZI$$Limit

#define __kernel_ram_start								Image$$ER_SRAM_BSS$$Base

#define _image_ram_end						Image$$ER_NOINIT$$Limit

#define __kernel_ram_end					Image$$ER_RAM_END$$Base
#define _end								__kernel_ram_end

#define __share_ram_start					Image$$ER_SHARE_RAM_BSS$$Base
#define __share_ram_end						Image$$ER_SHARE_RAM_BSS$$ZI$$Limit

