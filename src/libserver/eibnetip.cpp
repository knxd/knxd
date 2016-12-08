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

#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include "eibnetip.h"
#include "config.h"
#ifdef HAVE_LINUX_NETLINK
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif
#ifdef HAVE_WINDOWS_IPHELPER
#define Array XArray
#include <windows.h>
#include <iphlpapi.h>
#undef Array
#endif
#if HAVE_BSD_SOURCEINFO
#include <net/if.h>
#include <net/route.h>
#endif

int
GetHostIP (struct sockaddr_in *sock, const char *Name)
{
  struct hostent *h;
  if (!Name)
    return 0;
  memset (sock, 0, sizeof (*sock));
  h = gethostbyname (Name);
  if (!h)
    {
      ERRORPRINTF (t, E_ERROR | 50, this, "Resolving %s failed: %s", Name, hstrerror(h_errno));

      return 0;
    }
#ifdef HAVE_SOCKADDR_IN_LEN
  sock->sin_len = sizeof (*sock);
#endif
  sock->sin_family = h->h_addrtype;
  sock->sin_addr.s_addr = (*((unsigned long *) h->h_addr_list[0]));
  return 1;
}

#ifdef HAVE_LINUX_NETLINK
typedef struct
{
  struct nlmsghdr n;
  struct rtmsg r;
  char data[1000];
} r_req;

int
GetSourceAddress (const struct sockaddr_in *dest, struct sockaddr_in *src)
{
  int s;
  int l;
  r_req req;
  struct rtattr *a;
  memset (&req, 0, sizeof (req));
  memset (src, 0, sizeof (*src));
  s = socket (PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
  if (s == -1)
    return 0;
  req.n.nlmsg_len =
    NLMSG_SPACE (sizeof (req.r)) +
    RTA_LENGTH (sizeof (dest->sin_addr.s_addr));
  req.n.nlmsg_flags = NLM_F_REQUEST;
  req.n.nlmsg_type = RTM_GETROUTE;
  req.r.rtm_family = AF_INET;
  req.r.rtm_dst_len = 32;
  a = (rtattr *) ((char *) &req + NLMSG_SPACE (sizeof (req.r)));
  a->rta_type = RTA_DST;
  a->rta_len = RTA_LENGTH (sizeof (dest->sin_addr.s_addr));
  memcpy (RTA_DATA (a), &dest->sin_addr.s_addr,
	  sizeof (dest->sin_addr.s_addr));
  if (write (s, &req, req.n.nlmsg_len) < 0)
    goto err_out;
  if (read (s, &req, sizeof (req)) < 0)
    goto err_out;
  close (s);
  if (req.n.nlmsg_type == NLMSG_ERROR)
    return 0;
  l = ((struct nlmsghdr *) &req)->nlmsg_len;
  while (RTA_OK (a, l))
    {
      if (a->rta_type == RTA_PREFSRC
	  && RTA_PAYLOAD (a) == sizeof (src->sin_addr.s_addr))
	{
	  src->sin_family = AF_INET;
	  memcpy (&src->sin_addr.s_addr, RTA_DATA (a), RTA_PAYLOAD (a));
	  return 1;
	}
      a = RTA_NEXT (a, l);
    }
  return 0;
err_out:
  close (s);
  return 0;
}
#endif

#ifdef HAVE_WINDOWS_IPHELPER
int
GetSourceAddress (const struct sockaddr_in *dest, struct sockaddr_in *src)
{
  DWORD d = 0;
  PMIB_IPADDRTABLE tab;
  DWORD s = 0;

  memset (src, 0, sizeof (*src));
  if (GetBestInterface (dest->sin_addr.s_addr, &d) != NO_ERROR)
    return 0;

  tab = (MIB_IPADDRTABLE *) malloc (sizeof (MIB_IPADDRTABLE));
  if (!tab)
    return 0;
  if (GetIpAddrTable (tab, &s, 0) == ERROR_INSUFFICIENT_BUFFER)
    {
      tab = (MIB_IPADDRTABLE *) realloc (tab, s);
      if (!tab)
	return 0;
    }
  if (GetIpAddrTable (tab, &s, 0) != NO_ERROR)
    {
      if (tab)
	free (tab);
      return 0;
    }
  for (int i = 0; i < tab->dwNumEntries; i++)
    if (tab->table[i].dwIndex == d)
      {
	src->sin_family = AF_INET;
	src->sin_addr.s_addr = tab->table[i].dwAddr;
	free (tab);
	return 1;
      }
  free (tab);
  return 0;
}
#endif

#if HAVE_BSD_SOURCEINFO
typedef struct
{
  struct rt_msghdr hdr;
  char data[1000];
} r_req;

#ifndef HAVE_SA_SIZE
static int
SA_SIZE (struct sockaddr *sa)
{
  int align = sizeof (int);
  int len = (sa->sa_len ? sa->sa_len : align);
  if (len & (align - 1))
    {
      len += align - (len & (align - 1));
    }
  return len;
}
#endif

int
GetSourceAddress (const struct sockaddr_in *dest, struct sockaddr_in *src)
{
  int s;
  r_req req;
  char *cp = req.data;
  memset (&req, 0, sizeof (req));
  memset (src, 0, sizeof (*src));
  s = socket (PF_ROUTE, SOCK_RAW, 0);
  if (s == -1)
    return 0;
  req.hdr.rtm_msglen = sizeof (req) + sizeof (*dest);
  req.hdr.rtm_version = RTM_VERSION;
  req.hdr.rtm_flags = RTF_UP;
  req.hdr.rtm_type = RTM_GET;
  req.hdr.rtm_addrs = RTA_DST | RTA_IFP;
  memcpy (cp, dest, sizeof (*dest));
  if (write (s, (char *) &req, req.hdr.rtm_msglen) < 0)
    goto err_out;
  if (read (s, (char *) &req, sizeof (req)) < 0)
    goto err_out;
  close (s);
  int i;
  cp = (char *) (&req.hdr + 1);
  for (i = 1; i; i <<= 1)
    if (i & req.hdr.rtm_addrs)
      {
	struct sockaddr *sa = (struct sockaddr *) cp;
	if (i == RTA_IFA)
	  {
	    src->sin_len = sizeof (*src);
	    src->sin_family = AF_INET;
	    src->sin_addr.s_addr =
	      ((struct sockaddr_in *) sa)->sin_addr.s_addr;
	    return 1;
	  }
	cp += SA_SIZE (sa);
      }
  return 0;
err_out:
  close (s);
  return 0;
}
#endif

bool
compareIPAddress (const struct sockaddr_in & a, const struct sockaddr_in & b)
{
  if (a.sin_family != AF_INET)
    return false;
  if (b.sin_family != AF_INET)
    return false;
  if (a.sin_port != b.sin_port)
    return false;
  if (a.sin_addr.s_addr != b.sin_addr.s_addr)
    return false;
  return true;
}

EIBNetIPPacket::EIBNetIPPacket ()
{
  service = 0;
  memset (&src, 0, sizeof (src));
}

EIBNetIPPacket *
EIBNetIPPacket::fromPacket (const CArray & c, const struct sockaddr_in src)
{
  EIBNetIPPacket *p;
  unsigned len;
  if (c () < 6)
    return 0;
  if (c[0] != 0x6 || c[1] != 0x10)
    return 0;
  len = (c[4] << 8) | c[5];
  if (len != c ())
    return 0;
  p = new EIBNetIPPacket;
  p->service = (c[2] << 8) | c[3];
  p->data.set (c.array () + 6, len - 6);
  p->src = src;
  return p;
}

CArray
EIBNetIPPacket::ToPacket ()
  CONST
{
  CArray c;
  c.resize (6 + data ());
  c[0] = 0x06;
  c[1] = 0x10;
  c[2] = (service >> 8) & 0xff;
  c[3] = (service) & 0xff;
  c[4] = ((data () + 6) >> 8) & 0xff;
  c[5] = ((data () + 6)) & 0xff;
  c.setpart (data, 6);
  return c;
}

CArray
IPtoEIBNetIP (const struct sockaddr_in * a, bool nat)
{
  CArray buf;
  buf.resize (8);
  buf[0] = 0x08;
  buf[1] = 0x01;
  if (nat)
    {
      buf[2] = 0;
      buf[3] = 0;
      buf[4] = 0;
      buf[5] = 0;
      buf[6] = 0;
      buf[7] = 0;
    }
  else
    {
      buf[2] = (ntohl (a->sin_addr.s_addr) >> 24) & 0xff;
      buf[3] = (ntohl (a->sin_addr.s_addr) >> 16) & 0xff;
      buf[4] = (ntohl (a->sin_addr.s_addr) >> 8) & 0xff;
      buf[5] = (ntohl (a->sin_addr.s_addr) >> 0) & 0xff;
      buf[6] = (ntohs (a->sin_port) >> 8) & 0xff;
      buf[7] = (ntohs (a->sin_port) >> 0) & 0xff;
    }
  return buf;
}

int
EIBnettoIP (const CArray & buf, struct sockaddr_in *a,
	    const struct sockaddr_in *src, bool & nat)
{
  int ip, port;
  memset (a, 0, sizeof (*a));
  if (buf[0] != 0x8 || buf[1] != 0x1)
    return 1;
  ip = (buf[2] << 24) | (buf[3] << 16) | (buf[4] << 8) | (buf[5]);
  port = (buf[6] << 8) | (buf[7]);
#ifdef HAVE_SOCKADDR_IN_LEN
  a->sin_len = sizeof (*a);
#endif
  a->sin_family = AF_INET;
  if (port == 0)
    a->sin_port = src->sin_port;
  else
    a->sin_port = htons (port);
  if (ip == 0)
    {
      nat = true;
      a->sin_addr.s_addr = src->sin_addr.s_addr;
    }
  else
    a->sin_addr.s_addr = htonl (ip);

  return 0;
}

EIBNetIPSocket::EIBNetIPSocket (struct sockaddr_in bindaddr, bool reuseaddr,
				Trace * tr, SockMode mode)
{
  int i;
  t = tr;
  TRACEPRINTF (t, 0, this, "Open");
  multicast = 0;
  pth_sem_init (&insignal);
  pth_sem_init (&outsignal);
  getwait = pth_event (PTH_EVENT_SEM, &outsignal);
  memset (&maddr, 0, sizeof (maddr));
  memset (&sendaddr, 0, sizeof (sendaddr));
  memset (&recvaddr, 0, sizeof (recvaddr));
  memset (&recvaddr2, 0, sizeof (recvaddr2));
  recvall = 0;

  fd = socket (AF_INET, SOCK_DGRAM, 0);
  if (fd == -1)
    return;

  if (reuseaddr)
    {
      i = 1;
      if (setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof (i)) == -1)
	{
	  close (fd);
	  fd = -1;
	  return;
	}
    }
  if (bind (fd, (struct sockaddr *) &bindaddr, sizeof (bindaddr)) == -1)
    {
      ERRORPRINTF (t, E_ERROR | 38, this, "cannot bind to address: %s", strerror(errno));
      close (fd);
      fd = -1;
      return;
    }

  // Enable loopback so processes on the same host see each other.
  {
    char loopch=1;
 
    if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP,
                   (char *)&loopch, sizeof(loopch)) < 0) {
      ERRORPRINTF (t, E_ERROR | 39, this, "cannot turn on multicast loopback: %s", strerror(errno));
      close(fd);
      fd = -1;
      return;
    }
  }

  // don't really care if this fails
  if (mode == S_RD)
    shutdown (fd, SHUT_WR);
  else if (mode == S_WR)
    shutdown (fd, SHUT_RD);

  Start ();
  TRACEPRINTF (t, 0, this, "Openend");
}

EIBNetIPSocket::~EIBNetIPSocket ()
{
  TRACEPRINTF (t, 0, this, "Close");
  Stop ();
  pth_event_free (getwait, PTH_FREE_THIS);
  if (fd != -1)
    {
      if (multicast)
	setsockopt (fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &maddr,
		    sizeof (maddr));
      close (fd);
    }
}

bool
EIBNetIPSocket::init ()
{
  return fd != -1;
}

int
EIBNetIPSocket::port ()
{
  struct sockaddr_in sa;
  socklen_t saLen = sizeof(sa);
  if (getsockname(fd, (struct sockaddr *) &sa, &saLen) < 0)
    return -1;
  if (sa.sin_family != AF_INET)
    {
      errno = ENODATA;
      return -1;
    }
  return sa.sin_port;
}

bool
EIBNetIPSocket::SetMulticast (struct ip_mreq multicastaddr)
{
  if (multicast)
    return false;
  maddr = multicastaddr;
  if (setsockopt (fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &maddr, sizeof (maddr))
      == -1)
    return false;
  multicast = 1;
  return true;
}

void
EIBNetIPSocket::Send (EIBNetIPPacket p, struct sockaddr_in addr)
{
  struct _EIBNetIP_Send s;
  t->TracePacket (1, this, "Send", p.data);
  s.data = p;
  s.addr = addr;
  inqueue.put (s);
  pth_sem_inc (&insignal, 1);
}

EIBNetIPPacket *
EIBNetIPSocket::Get (pth_event_t stop)
{
  if (stop != NULL)
    pth_event_concat (getwait, stop, NULL);

  pth_wait (getwait);

  if (stop)
    pth_event_isolate (getwait);

  if (pth_event_status (getwait) == PTH_STATUS_OCCURRED)
    {
      EIBNetIPPacket *p;
      pth_sem_dec (&outsignal);
      p = outqueue.get ();
      t->TracePacket (1, this, "Recv", p->data);
      return p;
    }
  else
    return 0;
}

void
EIBNetIPSocket::Run (pth_sem_t * stop1)
{
  int i;
  int error = 0;
  uchar buf[255];
  socklen_t rl;
  sockaddr_in r;
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t input = pth_event (PTH_EVENT_SEM, &insignal);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      pth_event_concat (stop, input, NULL);
      rl = sizeof (r);
      memset (&r, 0, sizeof (r));
      i =
	pth_recvfrom_ev (fd, buf, sizeof (buf), 0, (struct sockaddr *) &r,
			 &rl, stop);
      if (i > 0 && rl == sizeof (r))
	{
	  if (recvall == 1 || !memcmp (&r, &recvaddr, sizeof (r)) ||
	      (recvall == 2 && memcmp (&r, &localaddr, sizeof (r))) ||
	      (recvall == 3 && !memcmp (&r, &recvaddr2, sizeof (r))))
	    {
	      t->TracePacket (0, this, "Recv", i, buf);
	      EIBNetIPPacket *p =
		EIBNetIPPacket::fromPacket (CArray (buf, i), r);
	      if (p)
		{
		  outqueue.put (p);
		  pth_sem_inc (&outsignal, 1);
		}
	    }
          else
	    t->TracePacket (0, this, "Dropped", i, buf);
	}
      pth_event_isolate (stop);
      if (!inqueue.isempty ())
	{
	  const struct _EIBNetIP_Send s = inqueue.top ();
	  CArray p = s.data.ToPacket ();
	  t->TracePacket (0, this, "Send", p);
	  i =
	    pth_sendto_ev (fd, p.array (), p (), 0,
			   (const struct sockaddr *) &s.addr, sizeof (s.addr),
			   stop);
	  if (i > 0)
	    {
	      pth_sem_dec (&insignal);
	      inqueue.get ();
	      error = 0;
	    }
	  else
	    error++;
	  if (error > 5)
	    {
	      t->TracePacket (0, this, "Drop EIBnetSocket", p);
	      pth_sem_dec (&insignal);
	      inqueue.get ();
	      error = 0;
	    }
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
  pth_event_free (input, PTH_FREE_THIS);
}

EIBnet_ConnectRequest::EIBnet_ConnectRequest ()
{
  memset (&caddr, 0, sizeof (caddr));
  memset (&daddr, 0, sizeof (daddr));
  nat = false;
}

EIBNetIPPacket EIBnet_ConnectRequest::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  CArray
    ca,
    da;
  ca = IPtoEIBNetIP (&caddr, nat);
  da = IPtoEIBNetIP (&daddr, nat);
  p.service = CONNECTION_REQUEST;
  p.data.resize (ca () + da () + 1 + CRI ());
  p.data.setpart (ca, 0);
  p.data.setpart (da, ca ());
  p.data[ca () + da ()] = CRI () + 1;
  p.data.setpart (CRI, ca () + da () + 1);
  return p;
}

int
parseEIBnet_ConnectRequest (const EIBNetIPPacket & p,
			    EIBnet_ConnectRequest & r)
{
  if (p.service != CONNECTION_REQUEST)
    return 1;
  if (p.data () < 18)
    return 1;
  if (EIBnettoIP (CArray (p.data.array (), 8), &r.caddr, &p.src, r.nat))
    return 1;
  if (EIBnettoIP (CArray (p.data.array () + 8, 8), &r.daddr, &p.src, r.nat))
    return 1;
  if (p.data () - 16 != p.data[16])
    return 1;
  r.CRI = CArray (p.data.array () + 17, p.data () - 17);
  return 0;
}

EIBnet_ConnectResponse::EIBnet_ConnectResponse ()
{
  memset (&daddr, 0, sizeof (daddr));
  nat = false;
  channel = 0;
  status = 0;
}

EIBNetIPPacket EIBnet_ConnectResponse::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  CArray
    da = IPtoEIBNetIP (&daddr, nat);
  p.service = CONNECTION_RESPONSE;
  if (status != 0)
    p.data.resize (2);
  else
    p.data.resize (da () + CRD () + 3);
  p.data[0] = channel;
  p.data[1] = status;
  if (status == 0)
    {
      p.data.setpart (da, 2);
      p.data[da () + 2] = CRD () + 1;
      p.data.setpart (CRD, da () + 3);
    }
  return p;
}

int
parseEIBnet_ConnectResponse (const EIBNetIPPacket & p,
			     EIBnet_ConnectResponse & r)
{
  if (p.service != CONNECTION_RESPONSE)
    return 1;
  if (p.data () < 2)
    return 1;
  if (p.data[1] != 0)
    {
      if (p.data () != 2)
	return 1;
      r.channel = p.data[0];
      r.status = p.data[1];
      return 0;
    }
  if (p.data () < 12)
    return 1;
  if (EIBnettoIP (CArray (p.data.array () + 2, 8), &r.daddr, &p.src, r.nat))
    return 1;
  if (p.data () - 10 != p.data[10])
    return 1;
  r.channel = p.data[0];
  r.status = p.data[1];
  r.CRD = CArray (p.data.array () + 11, p.data () - 11);
  return 0;
}

EIBnet_ConnectionStateRequest::EIBnet_ConnectionStateRequest ()
{
  memset (&caddr, 0, sizeof (caddr));
  nat = false;
  channel = 0;
}

EIBNetIPPacket EIBnet_ConnectionStateRequest::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  CArray
    ca = IPtoEIBNetIP (&caddr, nat);
  p.service = CONNECTIONSTATE_REQUEST;
  p.data.resize (ca () + 2);
  p.data[0] = channel;
  p.data[1] = 0;
  p.data.setpart (ca, 2);
  return p;
}

int
parseEIBnet_ConnectionStateRequest (const EIBNetIPPacket & p,
				    EIBnet_ConnectionStateRequest & r)
{
  if (p.service != CONNECTIONSTATE_REQUEST)
    return 1;
  if (p.data () != 10)
    return 1;
  if (EIBnettoIP (CArray (p.data.array () + 2, 8), &r.caddr, &p.src, r.nat))
    return 1;
  r.channel = p.data[0];
  return 0;
}

EIBnet_ConnectionStateResponse::EIBnet_ConnectionStateResponse ()
{
  channel = 0;
  status = 0;
}

EIBNetIPPacket EIBnet_ConnectionStateResponse::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  p.service = CONNECTIONSTATE_RESPONSE;
  p.data.resize (2);
  p.data[0] = channel;
  p.data[1] = status;
  return p;
}

int
parseEIBnet_ConnectionStateResponse (const EIBNetIPPacket & p,
				     EIBnet_ConnectionStateResponse & r)
{
  if (p.service != CONNECTIONSTATE_RESPONSE)
    return 1;
  if (p.data () != 2)
    return 1;
  r.channel = p.data[0];
  r.status = p.data[1];
  return 0;
}

EIBnet_DisconnectRequest::EIBnet_DisconnectRequest ()
{
  memset (&caddr, 0, sizeof (caddr));
  nat = false;
  channel = 0;
}

EIBNetIPPacket EIBnet_DisconnectRequest::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  CArray
    ca = IPtoEIBNetIP (&caddr, nat);
  p.service = DISCONNECT_REQUEST;
  p.data.resize (ca () + 2);
  p.data[0] = channel;
  p.data[1] = 0;
  p.data.setpart (ca, 2);
  return p;
}

int
parseEIBnet_DisconnectRequest (const EIBNetIPPacket & p,
			       EIBnet_DisconnectRequest & r)
{
  if (p.service != DISCONNECT_REQUEST)
    return 1;
  if (p.data () != 10)
    return 1;
  if (EIBnettoIP (CArray (p.data.array () + 2, 8), &r.caddr, &p.src, r.nat))
    return 1;
  r.channel = p.data[0];
  return 0;
}

EIBnet_DisconnectResponse::EIBnet_DisconnectResponse ()
{
  channel = 0;
  status = 0;
}

EIBNetIPPacket EIBnet_DisconnectResponse::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  p.service = DISCONNECT_RESPONSE;
  p.data.resize (2);
  p.data[0] = channel;
  p.data[1] = status;
  return p;
}

int
parseEIBnet_DisconnectResponse (const EIBNetIPPacket & p,
				EIBnet_DisconnectResponse & r)
{
  if (p.service != DISCONNECT_RESPONSE)
    return 1;
  if (p.data () != 2)
    return 1;
  r.channel = p.data[0];
  r.status = p.data[1];
  return 0;
}

EIBnet_TunnelRequest::EIBnet_TunnelRequest ()
{
  channel = 0;
  seqno = 0;
}

EIBNetIPPacket EIBnet_TunnelRequest::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  p.service = TUNNEL_REQUEST;
  p.data.resize (CEMI () + 4);
  p.data[0] = 4;
  p.data[1] = channel;
  p.data[2] = seqno;
  p.data[3] = 0;
  p.data.setpart (CEMI, 4);
  return p;
}

int
parseEIBnet_TunnelRequest (const EIBNetIPPacket & p, EIBnet_TunnelRequest & r)
{
  if (p.service != TUNNEL_REQUEST)
    return 1;
  if (p.data () < 6)
    return 1;
  if (p.data[0] != 4)
    return 1;
  r.channel = p.data[1];
  r.seqno = p.data[2];
  r.CEMI.set (p.data.array () + 4, p.data () - 4);
  return 0;
}

EIBnet_TunnelACK::EIBnet_TunnelACK ()
{
  channel = 0;
  seqno = 0;
  status = 0;
}

EIBNetIPPacket EIBnet_TunnelACK::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  p.service = TUNNEL_RESPONSE;
  p.data.resize (4);
  p.data[0] = 4;
  p.data[1] = channel;
  p.data[2] = seqno;
  p.data[3] = status;
  return p;
}

int
parseEIBnet_TunnelACK (const EIBNetIPPacket & p, EIBnet_TunnelACK & r)
{
  if (p.service != TUNNEL_RESPONSE)
    return 1;
  if (p.data () != 4)
    return 1;
  if (p.data[0] != 4)
    return 1;
  r.channel = p.data[1];
  r.seqno = p.data[2];
  r.status = p.data[3];
  return 0;
}

EIBnet_ConfigRequest::EIBnet_ConfigRequest ()
{
  channel = 0;
  seqno = 0;
}

EIBNetIPPacket EIBnet_ConfigRequest::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  p.service = DEVICE_CONFIGURATION_REQUEST;
  p.data.resize (CEMI () + 4);
  p.data[0] = 4;
  p.data[1] = channel;
  p.data[2] = seqno;
  p.data[3] = 0;
  p.data.setpart (CEMI, 4);
  return p;
}

int
parseEIBnet_ConfigRequest (const EIBNetIPPacket & p, EIBnet_ConfigRequest & r)
{
  if (p.service != DEVICE_CONFIGURATION_REQUEST)
    return 1;
  if (p.data () < 6)
    return 1;
  if (p.data[0] != 4)
    return 1;
  r.channel = p.data[1];
  r.seqno = p.data[2];
  r.CEMI.set (p.data.array () + 4, p.data () - 4);
  return 0;
}

EIBnet_ConfigACK::EIBnet_ConfigACK ()
{
  channel = 0;
  seqno = 0;
  status = 0;
}

EIBNetIPPacket EIBnet_ConfigACK::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  p.service = DEVICE_CONFIGURATION_ACK;
  p.data.resize (4);
  p.data[0] = 4;
  p.data[1] = channel;
  p.data[2] = seqno;
  p.data[3] = status;
  return p;
}

int
parseEIBnet_ConfigACK (const EIBNetIPPacket & p, EIBnet_ConfigACK & r)
{
  if (p.service != DEVICE_CONFIGURATION_ACK)
    return 1;
  if (p.data () != 4)
    return 1;
  if (p.data[0] != 4)
    return 1;
  r.channel = p.data[1];
  r.seqno = p.data[2];
  r.status = p.data[3];
  return 0;
}

EIBnet_DescriptionRequest::EIBnet_DescriptionRequest ()
{
  memset (&caddr, 0, sizeof (caddr));
  nat = false;
}

EIBNetIPPacket EIBnet_DescriptionRequest::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  CArray
    ca = IPtoEIBNetIP (&caddr, nat);
  p.service = DESCRIPTION_REQUEST;
  p.data = ca;
  return p;
}

int
parseEIBnet_DescriptionRequest (const EIBNetIPPacket & p,
				EIBnet_DescriptionRequest & r)
{
  if (p.service != DESCRIPTION_REQUEST)
    return 1;
  if (p.data () != 8)
    return 1;
  if (EIBnettoIP (p.data, &r.caddr, &p.src, r.nat))
    return 1;
  return 0;
}


EIBnet_DescriptionResponse::EIBnet_DescriptionResponse ()
{
  KNXmedium = 0;
  devicestatus = 0;
  individual_addr = 0;
  installid = 0;
  memset (&serial, 0, sizeof (serial));
  multicastaddr.s_addr = 0;
  memset (&MAC, 0, sizeof (MAC));
  memset (&name, 0, sizeof (name));
}

EIBNetIPPacket EIBnet_DescriptionResponse::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  p.service = DESCRIPTION_RESPONSE;
  p.data.resize (56 + services () * 2);
  p.data[0] = 54;
  p.data[1] = 1;
  p.data[2] = KNXmedium;
  p.data[3] = devicestatus;
  p.data[4] = (individual_addr >> 8) & 0xff;
  p.data[5] = (individual_addr) & 0xff;
  p.data[6] = (installid >> 8) & 0xff;
  p.data[7] = (installid) & 0xff;
  memcpy (p.data.array () + 18, &serial, 6);
  memcpy (p.data.array () + 14, &multicastaddr, 4);
  memcpy (p.data.array () + 18, &MAC, 6);
  memcpy (p.data.array () + 24, &name, 30);
  p.data[53] = 0;
  p.data[54] = services () * 2 + 2;
  p.data[55] = 2;
  for (unsigned int i = 0; i < services (); i++)
    {
      p.data[56 + i * 2] = services[i].family;
      p.data[57 + i * 2] = services[i].version;
    }
  p.data.setpart (optional, 56 + services () * 2);
  return p;
}

int
parseEIBnet_DescriptionResponse (const EIBNetIPPacket & p,
				 EIBnet_DescriptionResponse & r)
{
  if (p.service != DESCRIPTION_RESPONSE)
    return 1;
  if (p.data () < 56)
    return 1;
  if (p.data[0] != 54)
    return 1;
  if (p.data[1] != 1)
    return 1;
  r.KNXmedium = p.data[2];
  r.devicestatus = p.data[3];
  r.individual_addr = (p.data[4] << 8) | p.data[5];
  r.installid = (p.data[6] << 8) | p.data[7];
  memcpy (&r.serial, p.data.array () + 8, 6);
  memcpy (&r.multicastaddr, p.data.array () + 14, 4);
  memcpy (&r.MAC, p.data.array () + 18, 6);
  memcpy (&r.name, p.data.array () + 24, 30);
  r.name[29] = 0;
  if (p.data[55] != 2)
    return 1;
  if (p.data[54] % 2)
    return 1;
  if (p.data[54] + 54U > p.data ())
    return 1;
  r.services.resize ((p.data[54] / 2) - 1);
  for (int i = 0; i < (p.data[54] / 2) - 1; i++)
    {
      r.services[i].family = p.data[56 + 2 * i];
      r.services[i].version = p.data[57 + 2 * i];
    }
  r.optional.set (p.data.array () + p.data[54] + 54,
		  p.data () - p.data[54] - 54);
  return 0;
}

EIBnet_SearchRequest::EIBnet_SearchRequest ()
{
  memset (&caddr, 0, sizeof (caddr));
  nat = false;
}

EIBNetIPPacket EIBnet_SearchRequest::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  CArray
    ca = IPtoEIBNetIP (&caddr, nat);
  p.service = SEARCH_REQUEST;
  p.data = ca;
  return p;
}

int
parseEIBnet_SearchRequest (const EIBNetIPPacket & p, EIBnet_SearchRequest & r)
{
  if (p.service != SEARCH_REQUEST)
    return 1;
  if (p.data () != 8)
    return 1;
  if (EIBnettoIP (p.data, &r.caddr, &p.src, r.nat))
    return 1;
  return 0;
}


EIBnet_SearchResponse::EIBnet_SearchResponse ()
{
  KNXmedium = 0;
  devicestatus = 0;
  individual_addr = 0;
  installid = 0;
  memset (&serial, 0, sizeof (serial));
  multicastaddr.s_addr = 0;
  memset (&MAC, 0, sizeof (MAC));
  memset (&name, 0, sizeof (name));
}

EIBNetIPPacket EIBnet_SearchResponse::ToPacket ()CONST
{
  EIBNetIPPacket
    p;
  CArray
    ca = IPtoEIBNetIP (&caddr, nat);
  p.service = SEARCH_RESPONSE;
  p.data.resize (64 + services () * 2);
  p.data.setpart (ca, 0);
  p.data[8] = 54;
  p.data[9] = 1;
  p.data[10] = KNXmedium;
  p.data[11] = devicestatus;
  p.data[12] = (individual_addr >> 8) & 0xff;
  p.data[13] = (individual_addr) & 0xff;
  p.data[14] = (installid >> 8) & 0xff;
  p.data[15] = (installid) & 0xff;
  memcpy (p.data.array () + 16, &serial, 6);
  memcpy (p.data.array () + 22, &multicastaddr, 4);
  memcpy (p.data.array () + 26, &MAC, 6);
  memcpy (p.data.array () + 32, &name, 30);
  p.data[61] = 0;
  p.data[62] = services () * 2 + 2;
  p.data[63] = 2;
  for (unsigned int i = 0; i < services (); i++)
    {
      p.data[64 + i * 2] = services[i].family;
      p.data[65 + i * 2] = services[i].version;
    }
  return p;
}

int
parseEIBnet_SearchResponse (const EIBNetIPPacket & p,
			    EIBnet_SearchResponse & r)
{
  if (p.service != SEARCH_RESPONSE)
    return 1;
  if (p.data () < 64)
    return 1;
  if (EIBnettoIP (CArray (p.data.array () + 0, 8), &r.caddr, &p.src, r.nat))
    return 1;
  if (p.data[8] != 54)
    return 1;
  if (p.data[9] != 1)
    return 1;
  r.KNXmedium = p.data[10];
  r.devicestatus = p.data[11];
  r.individual_addr = (p.data[12] << 8) | p.data[13];
  r.installid = (p.data[14] << 8) | p.data[15];
  memcpy (&r.serial, p.data.array () + 16, 6);
  memcpy (&r.multicastaddr, p.data.array () + 22, 4);
  memcpy (&r.MAC, p.data.array () + 26, 6);
  memcpy (&r.name, p.data.array () + 32, 30);
  r.name[29] = 0;
  if (p.data[63] != 2)
    return 1;
  if (p.data[62] % 2)
    return 1;
  if (p.data[62] + 62U > p.data ())
    return 1;
  r.services.resize ((p.data[62] / 2) - 1);
  for (int i = 0; i < (p.data[62] / 2) - 1; i++)
    {
      r.services[i].family = p.data[64 + 2 * i];
      r.services[i].version = p.data[65 + 2 * i];
    }
  return 0;
}
