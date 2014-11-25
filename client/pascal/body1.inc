    Private complete : Complete_Function;
    Private recvdata : TByteArray;
    Private recvhead : TByteArray;
    Private ptr1 : PUINT16;
    Private ptr2 : PUINT8;
    Private ptr3 : PUINT8;
    Private ptr4 : PUINT16;
    Private ptr5 : PEIBAddr;
    Private ptr6 : PEIBAddr;
    Private ptr7 : PUINT32;
    Private buffer : PByteArray;
    Private sendlen : Integer;
    Private readlen : Integer;
    Private datalen : Integer;
    Private fd : LongInt;

    Public Function EIBComplete : Integer;
    Public Function EIBClose : Integer;
    Public Function EIBClose_sync : Integer;
    Public Function EIB_Poll_Complete : Integer;
    Public Function EIB_Poll_FD : Integer;
    Private Function EIB_SendRequest (data: TByteArray): Integer;
    Private Function EIB_GetRequest: Integer;
    Private Function EIB_CheckRequest (block: Boolean): Integer;

    Public constructor Create;
    Public Function EIBSocketLocal (path: String): Integer;
    Public Function EIBSocketRemote (host: String): Integer;
    Public Function EIBSocketRemote (host: String; port: Integer): Integer;
    Public Function EIBSocketURL (url: String): Integer;
 
  End;
Implementation
{
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
}
Uses BaseUnix, Sockets, Strings, NetDB;

Function GetHostAddr(Var addr: sockaddr_in; host: String): boolean;
Var he : THostEntry;
Begin
  If gethostbyname(host, he) Then
    Begin
      Result := true;
      addr.sin_addr.s_addr := htonl(he.addr.s_addr);
      exit;
    End;
  Result := false;
End;

Function EIBConnection.EIBComplete : Integer;
Begin
  If complete = Nil Then
    Begin
      fpseterrno (ESysEINVAL);
      Result := -1;
      exit;
    End;
  Result := complete();
End;

Function EIBConnection.EIBClose_sync : Integer;
Begin
  EIBReset();
  Result := EIBClose();
End;

Function EIBConnection.EIB_SendRequest(data : TByteArray): Integer;
Var head: Array[0..1] Of byte;
Begin
  If fd = -1 Then
    Begin;
      fpseterrno(ESysECONNRESET);
      Result := -1;
      exit;
    End;
  If (Length(data) < 2) Or (Length(data) >= $ffff) Then
    Begin
      fpseterrno(ESysEINVAL);
      Result := -1;
      exit;
    End;
  head[0] := (length(data) shr 8) And $ff;
  head[1] := (length(data)) And $ff;
  If fpsend(fd, @head[0], length(head), 0) = -1 Then
    Begin
      Result := -1;
      exit;
    End;
  If fpsend(fd, @data[0], length(data), 0) = -1 Then
    Begin
      Result := -1;
      exit;
    End;
  Result := 0;
End;

Function EIBConnection.EIBClose : Integer;
Begin
  If fd = -1 Then
    Begin;
      fpseterrno(ESysEINVAL);
      Result := -1;
      exit;
    End;
  CloseSocket(fd);
  fd := -1;
  Result := 0;
End;

Function EIBConnection.EIB_Poll_Complete: Integer;
Begin
  If EIB_CheckRequest(false) = -1 Then
    Begin;
      Result := -1;
      exit;
    End;
  If (readlen < 2) Or ((readlen >= 2) And (readlen < datalen + 2)) Then
    Result := 0
  Else
    Result := 1;
End;

Function EIBConnection.EIB_Poll_FD: Integer;
Begin
  result := fd;
End;

Function EIBConnection.EIB_GetRequest: Integer;
Begin
  Repeat
    If EIB_CheckRequest(true) = -1 Then
      Begin;
        Result := -1;
        exit;
      End;
    If (readlen >= 2) And (readlen >= datalen + 2) Then
      Begin
        readlen := 0;
        result := 0;
        exit;
      End;
  Until false;
End;

Function EIBConnection.EIB_CheckRequest(block: boolean): Integer;
Var i : Integer;
Begin
  If fd = -1 Then
    Begin;
      fpseterrno(ESysECONNRESET);
      Result := -1;
      exit;
    End;
  If readlen = 0 Then
    Begin
      setLength(recvdata, 0);
      setLength(recvhead, 2);
    End;
  If readlen < 2 Then
    Begin
      If block Then
        fpfcntl(fd, F_SETFL, fpfcntl(fd, F_GETFL, 0) And Not O_NONBLOCK)
      Else
        fpfcntl(fd, F_SETFL, fpfcntl(fd, F_GETFL, 0) Or O_NONBLOCK);
      i := fprecv(fd, @recvhead[readlen], 2-readlen, 0);
      If i = -1 Then
        Begin
          Result := -1;
          exit;
        End;
      If i = 0 Then
        Begin
          fpseterrno(ESysECONNRESET);
          Result := -1;
          exit;
        End;
      readlen := readlen + i;
    End;
  If readlen < 2 Then
    Begin
      Result := 0;
      exit;
    End;
  datalen := (recvhead[0] shr 8) Or recvhead[1];
  setLength (recvdata, datalen);
  If readlen < datalen + 2 Then
    Begin
      If block Then
        fpfcntl(fd, F_SETFL, fpfcntl(fd, F_GETFL, 0) And Not O_NONBLOCK)
      Else
        fpfcntl(fd, F_SETFL, fpfcntl(fd, F_GETFL, 0) Or O_NONBLOCK);
      i := fprecv(fd, @recvdata[readlen-2], datalen+2-readlen, 0);
      If i = -1 Then
        Begin
          Result := -1;
          exit;
        End;
      If i = 0 Then
        Begin
          fpseterrno(ESysECONNRESET);
          Result := -1;
          exit;
        End;
      readlen := readlen + i;
    End;
  Result := 0;
End;

Constructor EIBConnection.Create;
Begin
  readlen := 0;
  datalen := 0;
  fd := -1;
  complete := Nil;
  setLength(recvdata, 0);
End;

Function EIBConnection.EIBSocketLocal (path:String): Integer;
Var sock : LongInt;
  addr : sockaddr_un;
Begin
  If fd <> -1 Then
    Begin;
      fpseterrno(ESysEUSERS);
      Result := -1;
      exit;
    End;
  sock := fpsocket(AF_UNIX, SOCK_STREAM, 0);
  If sock = -1 Then
    Begin
      Result := -1;
      exit;
    End;
  addr.sun_family := AF_UNIX;
  strpcopy (addr.sun_path, path);
  If fpconnect(sock, @addr, length(path)+3) = -1 Then
    Begin
      Result := -1;
      exit;
    End;
  fd := sock;
End;

Function EIBConnection.EIBSocketRemote (host: String) : Integer;
Begin
  Result := EIBSocketRemote (host, 6720);
End;

Function EIBConnection.EIBSocketURL (url:String) : Integer;
var host, portstr: String;
  port,code : Integer;
Begin
  If copy(url, 1, 6) = 'local:' Then
    Begin
      Result := EIBSocketLocal (copy (url, 7, length(url)));
      exit;
    End;
  If copy(url, 1, 3) = 'ip:' Then
    Begin
      host := copy (url, 4, length(url));
      If pos(':', host) <> 0 Then
        Begin
          portstr := copy (host, pos(':', host)+1, length(url));
          host := copy (host, 1, pos(':', host)-1);
          val (portstr, port, code);
          If code <> 0 Then
            Begin
              fpseterrno(ESysEINVAL);
              Result := -1;
              exit;
            End;
          Result := EIBSocketRemote (host, port);
        End
      Else
        Begin
          Result := EIBSocketRemote (host);
        End;
      exit;
    End;
  fpseterrno(ESysEINVAL);
  Result := -1;
End;

Function EIBConnection.EIBSocketRemote (host: String; port: Integer) : Integer;
var sock : LongInt;
  addr : sockaddr_in; 
  arg : Integer;
Begin
  If fd <> -1 Then
    Begin;
      fpseterrno(ESysEUSERS);
      Result := -1;
      exit;
    End;
  If Not GetHostAddr(addr, host) Then
    Begin
      fpseterrno(ESysECONNREFUSED);
      Result := -1;
      exit;
    End;
  sock := fpsocket(AF_INET, SOCK_STREAM, 0);
  If sock = -1 Then
    Begin
      Result := -1;
      exit;
    End;
  addr.sin_family := AF_INET;
  addr.sin_port := htons (port);
  If fpconnect(sock, @addr, sizeof(addr)) = -1 Then
    Begin
      Result := -1;
      exit;
    End;
  arg := 1;
  fpsetsockopt(sock, IPPROTO_TCP, TCP_NODELAY, @arg, sizeof(arg));
  fd := sock;
  Result := 0;
End;


