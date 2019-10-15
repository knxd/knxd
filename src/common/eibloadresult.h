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

typedef enum
{
  IMG_UNKNOWN_ERROR           =  0,
  IMG_UNRECOG_FORMAT          =  1,
  IMG_INVALID_FORMAT          =  2,
  IMG_NO_BCUTYPE              =  3,
  IMG_UNKNOWN_BCUTYPE         =  4,
  IMG_NO_CODE                 =  5,
  IMG_NO_SIZE                 =  6,
  IMG_LODATA_OVERFLOW         =  7,
  IMG_HIDATA_OVERFLOW         =  8,
  IMG_TEXT_OVERFLOW           =  9,
  IMG_NO_ADDRESS              = 10,
  IMG_WRONG_SIZE              = 11,
  IMG_IMAGE_LOADABLE          = 12,
  IMG_NO_DEVICE_CONNECTION    = 13,
  IMG_MASK_READ_FAILED        = 14,
  IMG_WRONG_MASK_VERSION      = 15,
  IMG_CLEAR_ERROR             = 16,
  IMG_RESET_ADDR_TAB          = 17,
  IMG_LOAD_HEADER             = 18,
  IMG_LOAD_MAIN               = 19,
  IMG_ZERO_RAM                = 20,
  IMG_FINALIZE_ADDR_TAB       = 21,
  IMG_PREPARE_RUN             = 22,
  IMG_RESTART                 = 23,
  IMG_LOADED                  = 24,
  IMG_NO_START                = 25,
  IMG_WRONG_ADDRTAB           = 26,
  IMG_ADDRTAB_OVERFLOW        = 27,
  IMG_OVERLAP_ASSOCTAB        = 28,
  IMG_OVERLAP_TEXT            = 29,
  IMG_NEGATIV_TEXT_SIZE       = 30,
  IMG_OVERLAP_PARAM           = 31,
  IMG_OVERLAP_EEPROM          = 32,
  IMG_OBJTAB_OVERFLOW         = 33,
  IMG_WRONG_LOADCTL           = 34,
  IMG_UNLOAD_ADDR             = 35,
  IMG_UNLOAD_ASSOC            = 36,
  IMG_UNLOAD_PROG             = 37,
  IMG_LOAD_ADDR               = 38,
  IMG_WRITE_ADDR              = 39,
  IMG_SET_ADDR                = 40,
  IMG_FINISH_ADDR             = 41,
  IMG_LOAD_ASSOC              = 42,
  IMG_WRITE_ASSOC             = 43,
  IMG_SET_ASSOC               = 44,
  IMG_FINISH_ASSOC            = 45,
  IMG_LOAD_PROG               = 46,
  IMG_ALLOC_LORAM             = 47,
  IMG_ALLOC_HIRAM             = 48,
  IMG_ALLOC_INIT              = 49,
  IMG_ALLOC_RO                = 50,
  IMG_ALLOC_EEPROM            = 51,
  IMG_ALLOC_PARAM             = 52,
  IMG_SET_PROG                = 53,
  IMG_SET_TASK_PTR            = 54,
  IMG_SET_OBJ                 = 55,
  IMG_SET_TASK2               = 56,
  IMG_FINISH_PROC             = 57,
  IMG_WRONG_CHECKLIM          = 58,
  IMG_INVALID_KEY             = 59,
  IMG_AUTHORIZATION_FAILED    = 60,
  IMG_KEY_WRITE               = 61,
}
BCU_LOAD_RESULT;

#endif
