#!/usr/bin/env python3
#
# Build Actions NVRAM config binary file
#
# Copyright (c) 2017 Actions Semiconductor Co., Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import sys
import struct
import array
import argparse
import zlib
import configparser

from ctypes import *;

BINARY_BLOCK_SIZE_ALIGN = 512

class BOOT_PARAMETERS(Structure): # import from ctypes
    _pack_ = 1
    _fields_ = [
        ("magic",           c_uint32),
        ("uart_baudrate",   c_uint32),
        ("uart_id",         c_uint8),
        ("uart_mfp",        c_uint8),
        ("jtag_groud",      c_uint8),
        ("psram_mfp",       c_uint8),
        ("adfu_txrx",       c_uint8),
        ("reserved",        c_uint8 * 15),
        ("checksum",        c_uint32),
    ] # 32 bytes
SIZEOF_BOOT_PARAMETERS_VERSION   = 0x20
BOOT_PARAMETERS_MAGIC = 0x52415042   #'BPAR'
BOOT_PARAMETERS_OFFSET = 0x30

def c_struct_crc(c_struct, length):
    crc_buf = (c_byte * length)()
    memmove(addressof(crc_buf), addressof(c_struct), length)
    return zlib.crc32(crc_buf, 0) & 0xffffffff

def boot_padding(filename, align = BINARY_BLOCK_SIZE_ALIGN):
    fsize = os.path.getsize(filename)
    if fsize % align:
        padding_size = align - (fsize % align)
        print('fsize %d, padding_size %d' %(fsize, padding_size))
        with open(filename, 'rb+') as f:
            f.seek(fsize, 0)
            buf = (c_byte * padding_size)();
            f.write(buf)
            f.close()

def boot_calc_checksum(data):
        s = sum(array.array('H',data))
        s = s & 0xffff
        return s

IMAGE_TLV_INFO_MAGIC=0x5935
IMAGE_TLV_PROT_INFO_MAGIC=0x593a

def img_header_check(img_bin_f):
    """
    check img header, and add h_img_size
    """    
    img_len = os.path.getsize(img_bin_f)
    print('img  origin length %d' %(img_len))
    if img_len % 4:
         boot_padding(img_bin_f, 4)
         img_len = os.path.getsize(img_bin_f)
         print('img  align 4 length %d' %(img_len))

    with open(img_bin_f, 'rb+') as f:
        f.seek(0, 0)
        data = f.read(48)
        h_magic0,h_magic1,h_load_addr,h_name,h_run_addr,h_img_size,h_img_chksum, h_hdr_chksum, \
        h_header_size,h_ptlv_size,h_tlv_size,h_version,h_flags \
        = struct.unpack('III8sIIIIHHHHI',data)
        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            print('magic header check fail.')
            sys.exit(1)
        print('img h_header_size %d.' %h_header_size)
        h_img_size = img_len - h_header_size
        f.seek(0x18, 0)
        f.write(struct.pack('<I', h_img_size))
        print('img h_img_size %d.' %h_img_size)

        f.seek(0x26, 0)
        f.write(struct.pack('<h', 0))
        f.close()


def Add_data_to_tlv(img_bin_f, tlv_data_bin_f, tlv_type):
    """
    add data tlv
    """
    if not os.path.exists(tlv_data_bin_f):
        return 0
    img_len = os.path.getsize(img_bin_f)
    tlv_len = os.path.getsize(tlv_data_bin_f)
    print('tlv data length %d, img_len=%d' %(tlv_len,img_len))

    with open(tlv_data_bin_f, 'rb') as f:
        tlv_data = f.read()
        f.close()

    with open(img_bin_f, 'rb+') as f:
        f.seek(0, 0)
        data = f.read(48)
        h_magic0,h_magic1,h_load_addr,h_name,h_run_addr,h_img_size,h_img_chksum, h_hdr_chksum, \
        h_header_size,h_ptlv_size,h_tlv_size,h_version,h_flags \
        = struct.unpack('III8sIIIIHHHHI',data)
        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            print('magic header check fail.')
            sys.exit(1)
        plv_start = h_header_size + h_img_size + h_ptlv_size
        print('plv_start %d,header_size=%d, img_len=%d  ptlv_size=%d' %(plv_start, h_header_size, h_img_size, h_ptlv_size))

        f.seek(plv_start, 0)
        tlv_all_size = tlv_len + 4
        tlv_tol_size = 0
        if(plv_start == img_len):#not tlv header
            print('add tlv magic')
            tlv_tol_size = tlv_all_size
            f.write(struct.pack('<h', IMAGE_TLV_INFO_MAGIC))
            f.write(struct.pack('<h', tlv_tol_size))
        else :
            data1 = f.read(4)
            tlv_magic, tlv_tol_size= struct.unpack('HH',data1)
            if(tlv_magic != IMAGE_TLV_INFO_MAGIC):
                print('tlv magic  check fail.')
                sys.exit(1)
            print('old tlv_tol_size %d' %(tlv_tol_size))
            new_tlv_start = plv_start + tlv_tol_size + 4
            tlv_tol_size = tlv_tol_size + tlv_all_size
            f.seek(plv_start+2, 0)
            f.write(struct.pack('<h', tlv_tol_size))
            f.seek(new_tlv_start, 0)

        print('tlv_tol_size %d' %(tlv_tol_size))
        f.write(struct.pack('<h', tlv_type))
        f.write(struct.pack('<h', tlv_len))
        f.write(tlv_data)
        f.close()
        tlv_tol_size = tlv_tol_size + 4
        #if(tlv_tol_size > h_tlv_size):
        #    print('tlv_tol_size %d > header tlv size %d'%(tlv_tol_size, h_tlv_size))
         #   sys.exit(1)
        return tlv_tol_size
    return 0


def image_calc_checksum(data):
        s = sum(array.array('I',data))
        s = s & 0xffffffff
        return s

def image_add_cksum(filename, tlv_size):
    with open(filename, 'rb+') as f:
        f.seek(0, 0)
        data = f.read(48)
        h_magic0,h_magic1,h_load_addr,h_name,h_run_addr,h_img_size,h_img_chksum, h_hdr_chksum, \
        h_header_size,h_ptlv_size,h_tlv_size,h_version,h_flags \
        = struct.unpack('III8sIIIIHHHHI',data)
        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            print('magic header check fail.')
            sys.exit(1)

        print('img header_size %d, img size=%d, ptlv_size=%d' %(h_header_size,h_img_size, h_ptlv_size))
        f.seek(h_header_size, 0)
        data = f.read(h_img_size)
        checksum = image_calc_checksum(data)
        checksum = 0xffffffff - checksum
        f.seek(0x1c, 0)
        f.write(struct.pack('<I', checksum))
        print('img checksum 0x%x.' %checksum)

        f.seek(0x28, 0)
        f.write(struct.pack('<H', tlv_size))

        f.seek(0x0, 0)
        data = f.read(h_header_size)
        checksum = image_calc_checksum(data)
        checksum = 0xffffffff - checksum
        f.seek(0x20, 0)
        f.write(struct.pack('<I', checksum))
        print('header checksum 0x%x.' %checksum)
        f.close()
        print('boot loader add cksum pass.')


def tai_boot_post_build(boot_name):
    print('tai boot add ini')
    img_header_check(boot_name)
    image_add_cksum(boot_name, 0)
    boot_padding(boot_name)

def boot_post_build(boot_name):

    with open(boot_name, 'rb+') as f:
        f.seek(0, 0)
        data = f.read(16)
        f.close()
        h_magic0,h_magic1,h_load_addr,h_name = struct.unpack('III4s',data)
        if(h_magic0 != 0x48544341 or h_magic1 != 0x41435448):
            print('boot loader header check fail.')
            return
        print('boot name =%s'% h_name)
        tai_boot_post_build(boot_name)

def main(argv):
    boot_post_build(argv[1])

if __name__ == "__main__":
    main(sys.argv)
