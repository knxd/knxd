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

package eibd

import (
  "fmt"
  "net"
  "strconv"
  "strings"
  "time"
)

type CompleteFunc func() (int,error)

type EIBAddr uint16

type EIBConnection struct {
    head       []byte
    data       []byte
    readlen    int
    datalen    int
    sendlen    int
    con        net.Conn
    buffer     []byte
    __complete CompleteFunc
    ptr1       *uint16
    ptr2       *uint8
    ptr3       *uint8
    ptr4       *uint16
    ptr5       *uint16
    ptr6       *uint16
    ptr7       *uint32
}

func NewEIBConnection() (*EIBConnection) {
    return &EIBConnection{nil,nil,0,0,0,nil,nil,nil,nil,nil,nil,nil,nil,nil,nil}
}

func (this *EIBConnection) EIBSocketLocal(path string) (error) {

    if this.con != nil {
      return fmt.Errorf("Connection already open")
    }
    var err error
    this.con,err = net.Dial("unix", path)
    if err != nil {
      return err
    }
    this.data = nil
    this.readlen = 0
    return nil
}

func (this *EIBConnection) EIBSocketRemote(host string, port uint16) (error) {

    if this.con != nil {
      return fmt.Errorf("Connection already open")
    }
    var hostport string
    fmt.Sprintf(hostport,"%s:%d",host,port)
    var err error
    this.con,err = net.Dial("tcp", hostport)
    if err != nil {
      return err
    }
    this.data = nil
    this.readlen = 0
    return nil
}

func (this *EIBConnection) EIBSocketURL(url string) (error) {

    if url[0:6] == "local:" {
      return this.EIBSocketLocal(url[6:])
    }
    if url[0:3] == "ip:" {
         parts := strings.Split(url,":")
         if len(parts) == 2 {
             parts = append(parts,"6720")
         }
         port,err := strconv.Atoi(parts[2])
         if err != nil || port<0 || port>65535 {
            return fmt.Errorf("Port invalid")
         }
         return this.EIBSocketRemote(parts[1], uint16(port))
    }
    return fmt.Errorf("Protocol invalid")
}

func (this *EIBConnection) EIBComplete() (int,error) {
    if this.__complete == nil {
        return 0,fmt.Errorf("Already completed")
    }
    return this.__complete()
}

func (this *EIBConnection) EIBClose() (error) {
    if this.con == nil {
      return fmt.Errorf("Close failed, not connected")
    }
    this.con.Close()
    this.con = nil
    return nil
}

func (this *EIBConnection) EIBClose_sync() (error) {
    this.EIBReset()
    return this.EIBClose()
}

func (this *EIBConnection) __EIB_SendRequest(data []byte) (error) {
    if this.con == nil {
      return fmt.Errorf("Not connected")
    }
    if len(data) < 2 || len(data) > 0xffff {
      return fmt.Errorf("Invalid data")
    }
    headdata := []byte{ byte((len(data)>>8)&0xff), byte((len(data))&0xff) }
    data = append(headdata,data...)
    _,err := this.con.Write(data) // TODO evtl evaluate n
    return err
}

func (this *EIBConnection) EIB_Poll_FD() (int, error) {
    if this.con == nil {
      return 0,fmt.Errorf("Not connected")
    }
    return 0,fmt.Errorf("Not yet implemented") // TODO
}

func (this *EIBConnection) EIB_Poll_Complete() (bool,error) {
    if this.__EIB_CheckRequest(false) != nil {
      return false,fmt.Errorf("Request failed")
    }
    if this.readlen < 2 || (this.readlen >= 2 && this.readlen < this.datalen + 2) {
      return false,nil
    }
    return true,nil
}

func (this *EIBConnection) __EIB_GetRequest() (error) {
    for {
        if this.__EIB_CheckRequest(true) != nil {
            return fmt.Errorf("Request failed")
        }
        if this.readlen >= 2 && this.readlen >= this.datalen + 2 {
            this.readlen = 0
            return nil
        }
    }
}

func (this *EIBConnection) __EIB_CheckRequest(block bool) (error) {

    if this.con == nil {
      return fmt.Errorf("Not connected")
    }

    if this.readlen == 0 {
      this.head = []byte{}
      this.data = []byte{}
    }
    if this.readlen < 2 {
      if block == true {
        this.con.SetDeadline(time.Time{})
      } else {
        this.con.SetDeadline(time.Now().Add(1*time.Millisecond))
      }
      buf := make([]byte, 2-this.readlen)
      n,err := this.con.Read (buf)
      this.con.SetDeadline(time.Time{}) // reset the absolute deadline
      if err != nil {
        return err
      }
      this.head = append(this.head,buf...)
      this.readlen += n
    }
    if this.readlen < 2 {
      return nil
    }
    this.datalen = int( (this.head[0] << 8) | this.head[1] )
    if this.readlen < this.datalen + 2 {
      if block == true {
        this.con.SetDeadline(time.Time{})
      } else {
        this.con.SetDeadline(time.Now().Add(1*time.Millisecond))
      }
      buf := make([]byte, this.datalen + 2-this.readlen)
      n,err := this.con.Read (buf)
      this.con.SetDeadline(time.Time{}) // reset the absolute deadline
      if err != nil {
        return err
      }
      this.data = append(this.data,buf...)
      this.readlen += n
    }
    return nil
}


