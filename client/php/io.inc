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

   private $data;
   private $head;
   private $readlen;
   private $datalen;
   private $socket;

   function __construct ($host, $port = 6720)
   {
     $this->readlen = 0;
     $this->socket = stream_socket_client ("tcp://".$host.":".$port);
     if ($this->socket === FALSE)
       throw new Exception ("connect failed");
   }

   private function _EIB_SendRequest ($data)
   {
     if ($this->socket === FALSE)
       {
	 $this->errno = self::ECONNRESET;
	 return -1;
       }
    if (strlen($data) > 0xffff || strlen($data) < 2)
      {
        $this->errno = self::EINVAL;
        return -1;
      }
    $len=pack("n", strlen($data));
    if (fwrite($this->socket, $len.$data) != strlen($data)+2)
      {
	 $this->errno = self::ECONNRESET;
	 return -1;
      }
    return 0;
   }

   private function _EIB_CheckRequest($block)
   {
     if ($this->socket === FALSE || feof ($this->socket))
       {
	 $this->errno = self::ECONNRESET;
	 return -1;
       }
     if ($this->readlen == 0)
       {
	 $this->head = array (" ", " ");
	 $this->data = array ();
       }

     if ($this->readlen < 2)
       {
	 stream_set_blocking ($this->socket, $block ? 1 : 0);
	 $read = fread ($this->socket, 2 - $this->readlen);
	 if ($read === FALSE)
	   {
	     $this->errno = self::ECONNRESET;
	     return -1;
	   }
	 for ($i = 0; $i < strlen ($read); $i++)
	   $this->head [$this->readlen++] = substr($read, $i, 1);
       }
     if ($this->readlen < 2)
       return 0;
     $this->datalen = EIBConnection::upack (implode ("", $this->head), "n");
     if (feof ($this->socket))
       {
	 $this->errno = self::ECONNRESET;
	 return -1;
       }
     if ($this->readlen < $this->datalen + 2)
       {
	 stream_set_blocking ($this->socket, $block ? 1 : 0);
	 $read = fread ($this->socket, $this->datalen + 2 - $this->readlen);
	 if ($read === FALSE)
	   {
	     $this->errno = self::ECONNRESET;
	     return -1;
	   }
	 for ($i = 0; $i < strlen ($read); $i++)
	   $this->data [($this->readlen++) - 2] = substr($read, $i, 1);
       }
     return 0;
   }

   private function _EIB_GetRequest()
   {
     do
       {
	 if ($this->_EIB_CheckRequest (true) == -1)
	   return -1;
       }
     while ($this->readlen < 2 || ($this->readlen >= 2 && $this->readlen < $this->datalen + 2));
     $this->data = implode ("", $this->data);
     $this->readlen = 0;
     return 0;
   }

   public function EIB_Poll_Complete()
   {
     if ($this->_EIB_CheckRequest (false) == -1)
       return -1;
     if ($this->readlen < 2 || ($this->readlen >= 2 && $this->readlen < $this->datalen + 2))
       return 0;
     return 1;
   }

   public function EIBClose()
   {
     if ($socket === FALSE)
       {
	 $this->errno = self::EBADF;
	 return -1;
       }
     fclose ($this->socket);
     $this->socket = FALSE;
   }

   public function EIBClose_sync()
   {
     $this->EIBReset();
     return $this->EIBClose();
   }
