./build/prep_c.o: ..\..\..\arch\arm\core\aarch32\prep_c.c linker_conf.h \
  ..\src\include\autoconf.h ..\src\include\autoconf_app.h \
  ..\src\include\atb1113_46k.h \
  ..\..\..\include\toolchain\zephyr_stdint.h ..\..\..\include\kernel.h \
  ..\..\..\include\kernel_includes.h ..\..\..\include\zephyr\types.h \
  ..\..\..\lib\libc\minimal\include\stdint.h \
  ..\..\..\lib\libc\minimal\include\limits.h \
  ..\..\..\include\toolchain.h ..\..\..\include\toolchain\gcc.h \
  ..\..\..\include\toolchain\common.h \
  ..\..\..\lib\libc\minimal\include\stdbool.h \
  ..\..\..\include\linker\sections.h \
  ..\..\..\include\linker\section_tags.h ..\..\..\include\sys\atomic.h \
  ..\..\..\include\sys\__assert.h ..\..\..\include\sched_priq.h \
  ..\..\..\include\sys\util.h ..\..\..\include\sys\util_macro.h \
  ..\..\..\include\sys\util_internal.h ..\..\..\include\sys\dlist.h \
  ..\..\..\include\sys\rb.h ..\..\..\include\sys\slist.h \
  ..\..\..\include\sys\list_gen.h ..\..\..\include\sys\sflist.h \
  ..\..\..\include\kernel_structs.h ..\..\..\include\sys\sys_heap.h \
  ..\..\..\include\mempool_heap.h ..\..\..\include\kernel_version.h \
  ..\..\..\include\syscall.h ..\..\..\include\generated\syscall_list.h \
  ..\..\..\include\arch\syscall.h \
  ..\..\..\include\arch\arm\aarch32\syscall.h \
  ..\..\..\include\sys\printk.h \
  ..\..\..\lib\libc\minimal\include\inttypes.h \
  ..\..\..\include\arch\cpu.h ..\..\..\include\sys\arch_interface.h \
  ..\..\..\include\irq_offload.h ..\..\..\include\arch\arch_inlines.h \
  ..\..\..\include\arch\arm\aarch32\arch.h \
  ..\..\..\include\arch\arm\aarch32\thread.h \
  ..\..\..\include\arch\arm\aarch32\exc.h \
  ..\..\..\include\arch\arm\aarch32\cortex_m\nvic.h \
  ..\..\..\include\arch\arm\aarch32\irq.h ..\..\..\include\irq.h \
  ..\..\..\include\sw_isr_table.h \
  ..\..\..\include\arch\arm\aarch32\error.h \
  ..\..\..\include\arch\arm\aarch32\misc.h \
  ..\..\..\include\arch\common\addr_types.h \
  ..\..\..\include\arch\common\ffs.h \
  ..\..\..\include\arch\arm\aarch32\nmi.h \
  ..\..\..\include\arch\arm\aarch32\asm_inline.h \
  ..\..\..\include\arch\arm\aarch32\asm_inline_gcc.h \
  ..\..\..\include\arch\common\sys_bitops.h \
  ..\..\..\include\sys\sys_io.h \
  ..\..\..\include\arch\arm\aarch32\cortex_m\cpu.h \
  ..\..\..\include\arch\arm\aarch32\cortex_m\memory_map.h \
  ..\..\..\include\arch\common\sys_io.h ..\..\..\include\sys_clock.h \
  ..\..\..\include\sys\time_units.h ..\..\..\include\spinlock.h \
  ..\..\..\include\fatal.h ..\..\..\include\sys\thread_stack.h \
  ..\..\..\include\app_memory\mem_domain.h \
  ..\..\..\include\kernel\thread.h ..\..\..\include\sys\kobject.h \
  ..\..\..\include\generated\kobj-types-enum.h \
  ..\..\..\include\generated\syscalls\kobject.h \
  ..\..\..\lib\libc\minimal\include\errno.h \
  ..\..\..\include\sys\errno_private.h \
  ..\..\..\include\generated\syscalls\errno_private.h \
  ..\..\..\include\tracing\tracing.h \
  ..\..\..\include\generated\syscalls\kernel.h \
  ..\..\..\kernel\include\kernel_internal.h \
  ..\..\..\kernel\include\kernel_arch_interface.h \
  ..\..\..\arch\arm\include\kernel_arch_func.h \
  ..\..\..\arch\arm\include\aarch32\kernel_arch_func.h \
  ..\..\..\arch\arm\include\kernel_arch_data.h \
  ..\..\..\arch\arm\include\aarch32\cortex_m\stack.h \
  ..\..\..\include\arch\arm\aarch32\cortex_m\cmsis.h \
  ..\..\..\soc\arm\actions\tai\soc.h \
  ..\..\..\soc\arm\actions\tai\soc_regs.h \
  ..\..\..\soc\arm\actions\tai\brom_interface.h \
  ..\..\..\lib\libc\minimal\include\sys\types.h \
  ..\..\..\lib\libc\minimal\include\sys\_types.h \
  ..\..\..\soc\arm\actions\tai\soc_clock.h \
  ..\..\..\soc\arm\actions\tai\soc_reset.h \
  ..\..\..\soc\arm\actions\tai\soc_irq.h \
  ..\..\..\soc\arm\actions\tai\soc_gpio.h \
  ..\..\..\soc\arm\actions\tai\soc_pinmux.h \
  ..\..\..\soc\arm\actions\tai\soc_pm.h \
  ..\..\..\soc\arm\actions\tai\soc_boot.h \
  ..\..\..\soc\arm\actions\tai\soc_memctrl.h \
  ..\..\..\soc\arm\actions\tai\soc_se.h ..\..\..\include\device.h \
  ..\..\..\include\init.h ..\..\..\include\sys\device_mmio.h \
  ..\..\..\include\sys\mem_manage.h \
  ..\..\..\include\generated\syscalls\device.h \
  ..\..\..\soc\arm\actions\tai\soc_timer.h \
  ..\..\..\soc\arm\actions\tai\soc_saradc.h \
  ..\..\..\soc\arm\actions\tai\soc_efuse.h \
  ..\..\..\ext\hal\cmsis\CMSIS\Core\Include\core_cm4.h \
  ..\..\..\arch\arm\include\aarch32\cortex_m\exc.h \
  ..\..\..\lib\libc\minimal\include\string.h \
  ..\..\..\lib\libc\minimal\include\bits\restrict.h \
  ..\..\..\include\linker\linker-defs.h \
  ..\..\..\include\generated\offsets.h
