/*
    EIBD client library
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    In addition to the permissions in the GNU General Public License, 
    you may link the compiled version of this file into combinations
    with other programs, and distribute those combinations without any 
    restriction coming from the use of this file. (The General Public 
    License restrictions do apply in other respects; for example, they 
    cover modification of the file, and distribution when not linked into 
    a combine executable.)

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef EIB_LOAD_RESULT_H
#define EIB_LOAD_RESULT_H

#define IMG_UNKNOWN_ERROR             0
#define IMG_UNRECOG_FORMAT            1
#define IMG_INVALID_FORMAT            2
#define IMG_NO_BCUTYPE                3
#define IMG_UNKNOWN_BCUTYPE           4
#define IMG_NO_CODE                   5
#define IMG_NO_SIZE                   6
#define IMG_LODATA_OVERFLOW           7
#define IMG_HIDATA_OVERFLOW           8
#define IMG_TEXT_OVERFLOW             9
#define IMG_NO_ADDRESS               10
#define IMG_WRONG_SIZE               11
#define IMG_IMAGE_LOADABLE           12
#define IMG_NO_DEVICE_CONNECTION     13
#define IMG_MASK_READ_FAILED         14
#define IMG_WRONG_MASK_VERSION       15
#define IMG_CLEAR_ERROR              16
#define IMG_RESET_ADDR_TAB           17
#define IMG_LOAD_HEADER              18
#define IMG_LOAD_MAIN                19
#define IMG_ZERO_RAM                 20
#define IMG_FINALIZE_ADDR_TAB        21
#define IMG_PREPARE_RUN              22
#define IMG_RESTART                  23
#define IMG_LOADED                   24
#define IMG_NO_START                 25
#define IMG_WRONG_ADDRTAB            26
#define IMG_ADDRTAB_OVERFLOW         27
#define IMG_OVERLAP_ASSOCTAB         28
#define IMG_OVERLAP_TEXT             29
#define IMG_NEGATIV_TEXT_SIZE        30
#define IMG_OVERLAP_PARAM            31
#define IMG_OVERLAP_EEPROM           32
#define IMG_OBJTAB_OVERFLOW          33
#define IMG_WRONG_LOADCTL            34
#define IMG_UNLOAD_ADDR              35
#define IMG_UNLOAD_ASSOC             36
#define IMG_UNLOAD_PROG              37
#define IMG_LOAD_ADDR                38
#define IMG_WRITE_ADDR               39
#define IMG_SET_ADDR                 40
#define IMG_FINISH_ADDR              41
#define IMG_LOAD_ASSOC               42
#define IMG_WRITE_ASSOC              43
#define IMG_SET_ASSOC                44
#define IMG_FINISH_ASSOC             45
#define IMG_LOAD_PROG                46
#define IMG_ALLOC_LORAM              47
#define IMG_ALLOC_HIRAM              48
#define IMG_ALLOC_INIT               49
#define IMG_ALLOC_RO                 50
#define IMG_ALLOC_EEPROM             51
#define IMG_ALLOC_PARAM              52
#define IMG_SET_PROG                 53
#define IMG_SET_TASK_PTR             54
#define IMG_SET_OBJ                  55
#define IMG_SET_TASK2                56
#define IMG_FINISH_PROC              57
#define IMG_WRONG_CHECKLIM           58
#define IMG_INVALID_KEY              59
#define IMG_AUTHORIZATION_FAILED     60
#define IMG_KEY_WRITE                61

typedef int BCU_LOAD_RESULT;

#endif
