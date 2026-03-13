/******************** (C) COPYRIGHT 2020 Actions-semi ******************** 
 * File Name          : ksdfs.s 
 * Author             : Tai Team 
 * Version            : V1.0 
 * Date               : 2022-02-15 
 * Description        : Include ksdfs bin. 
 *************************************************************************/ 
                .section .ksdfs,"a" 
                                .align 5 
                                .global __ksdfs_start 
                                .global __ksdfs__end 
__ksdfs_start: 
                                .incbin "ksdfs.bin" 
__ksdfs__end: 
                                .align 5 
                                .end 
                PRESERVE8 
                END
