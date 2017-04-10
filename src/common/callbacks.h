/*
    EIBD eib bus access and management daemon
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/**
 * This 
 */

#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "types.h"
#include <ev++.h>

typedef size_t (*data_cb_t)(void *data, uint8_t *buf, size_t len);
typedef void (*info_cb_t)(void *data);
typedef void (*state_cb_t)(void *data, bool success);

class InfoCallback {
    info_cb_t cb_code = 0;
    void *cb_data = 0;

    void set_ (const void *data, info_cb_t cb)
    {
      this->cb_data = (void *)data;
      this->cb_code = cb;
    }

public:
    // method callback
    template<class K, void (K::*method)()>
    void set (K *object)
    {
      set_ (object, method_thunk<K, method>);
    }

    template<class K, void (K::*method)()>
    static void method_thunk (void *arg)
    {
      (static_cast<K *>(arg)->*method) ();
    }

    void operator()() {
        (*cb_code)(cb_data);
    }
};

class StateCallback {
    state_cb_t cb_code = 0;
    void *cb_data = 0;

    void set_ (const void *data, state_cb_t cb)
    {
      this->cb_data = (void *)data;
      this->cb_code = cb;
    }

public:
    // method callback
    template<class K, void (K::*method)(bool success)>
    void set (K *object)
    {
      set_ (object, method_thunk<K, method>);
    }

    template<class K, void (K::*method)(bool success)>
    static void method_thunk (void *arg, bool success)
    {
      (static_cast<K *>(arg)->*method) (success);
    }

    void operator()(bool success) {
        (*cb_code)(cb_data, success);
    }
};

class DataCallback {
    data_cb_t cb_code = 0;
    void *cb_data = 0;

    void set_ (const void *data, data_cb_t cb)
    {
      this->cb_data = (void *)data;
      this->cb_code = cb;
    }

public:
    // method callback
    template<class K, size_t (K::*method)(uint8_t *buf, size_t len)>
    void set (K *object)
    {
      set_ (object, method_thunk<K, method>);
    }

    template<class K, size_t (K::*method)(uint8_t *buf, size_t len)>
    static size_t method_thunk (void *arg, uint8_t *buf, size_t len)
    {
      return (static_cast<K *>(arg)->*method) (buf,len);
    }

    size_t operator()(uint8_t *buf, size_t len) {
        return (*cb_code)(cb_data, buf,len);
    }
};

#endif
