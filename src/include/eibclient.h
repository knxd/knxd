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
#ifndef EIBCLIENT_H
#define EIBCLIENT_H

#include <sys/cdefs.h>
#include <stdint.h>

__BEGIN_DECLS;

#include "eibloadresult.h"

/** type represents a connection to eibd */
typedef struct _EIBConnection EIBConnection;

/** type for storing a EIB address */
typedef uint16_t eibaddr_t;

/** Opens a connection to eibd.
 *   url can either be <code>ip:host:[port]</code> or <code>local:/path/to/socket</code>
 * \param url contains the url to connect to
 * \return connection handle or NULL
 */
EIBConnection *EIBSocketURL (const char *url);
/** Opens a connection to eibd over a socket.
 * \param path path to the socket
 * \return connection handle or NULL
 */
EIBConnection *EIBSocketLocal (const char *path);
/** Opens a connection to eibd over TCP/IP.
 * \param host hostname running eibd
 * \param port portnumber
 * \return connection handle or NULL
 */
EIBConnection *EIBSocketRemote (const char *host, int port);

/** Closes and frees a connection.
 * \param con eibd connection
 */
int EIBClose (EIBConnection * con);

/** Closes and frees a connection and wait, until all resources are freed.
 * \param con eibd connection
 */
int EIBClose_sync (EIBConnection * con);

/** Finish an asynchronous request (and block until then).
 * \param con eibd connection
 * \return return value, as returned by the synchronous function call
 */
int EIBComplete (EIBConnection * con);

/** Checks if an asynchronous request is completed (non-blocking).
 * EIBComplete must be still be used for asynchronous functions to retrieve the return value.
 * For connections where packets are returned (Busmonitor, T_*), EIB_Poll_Complete can be used to check if new data is available.
 * If this function returns an error, the eibd connection should be considered as broken (and therefore be closed).
 * \param con eibd connection
 * \return -1 if any error, 0 if not finished, 1 if finished
 */
int EIB_Poll_Complete (EIBConnection * con);

/** Returns FD to wait for the next event.
 * The returned file descriptor may only be used to select/poll for read data available.
 * As EIBComplete (and functions, which return packets) block if only a part of the data is
 * available, EIB_Poll_Complete can be used to check whether blocking will occur.
 * \param con eibd connection
 * \return -1 if any error, else file descriptor
 */
int EIB_Poll_FD (EIBConnection * con);

/** Switches the connection to pristine state
 * \param con eibd connection
 * \return 0 if successful, -1 if error
 */
int EIBReset (EIBConnection * con);

/** Switches the connection to pristine state - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIBReset_async (EIBConnection * con);

/** Switches the connection to binary busmonitor mode.
 * \param con eibd connection
 * \return 0 if successful, -1 if error
 */
int EIBOpenBusmonitor (EIBConnection * con);

/** Switches the connection to binary busmonitor mode - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIBOpenBusmonitor_async (EIBConnection * con);

/** Switches the connection to text busmonitor mode.
 * \param con eibd connection
 * \return 0 if successful, -1 if error
 */
int EIBOpenBusmonitorText (EIBConnection * con);

/** Switches the connection to text busmonitor mode - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIBOpenBusmonitorText_async (EIBConnection * con);

/** Switches the connection to binary busmonitor mode.
 * \param con eibd connection
 * \param timebase if not null, tick length in ns is stored at this location - it is 0, if not provided by the interface
 * \return 0 if successful, -1 if error
 */
int EIBOpenBusmonitorTS (EIBConnection * con, uint32_t * timebase);

/** Switches the connection to binary busmonitor mode - asynchronous.
 * \param con eibd connection
 * \param timebase if not null, tick length in ns is stored at this location - it is 0, if not provided by the interface
 * \return 0 if started, -1 if error
 */
int EIBOpenBusmonitorTS_async (EIBConnection * con, uint32_t * timebase);

/** Switches the connection to binary vbusmonitor mode.
 * \param con eibd connection
 * \return 0 if successful, -1 if error
 */
int EIBOpenVBusmonitor (EIBConnection * con);

/** Switches the connection to binary vbusmonitor mode - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIBOpenVBusmonitor_async (EIBConnection * con);

/** Switches the connection to text vbusmonitor mode.
 * \param con eibd connection
 * \return 0 if successful, -1 if error
 */
int EIBOpenVBusmonitorText (EIBConnection * con);

/** Switches the connection to text vbusmonitor mode - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIBOpenVBusmonitorText_async (EIBConnection * con);

/** Switches the connection to binary busmonitor mode.
 * \param con eibd connection
 * \param timebase if not null, tick length in ns is stored at this location - it is 0, if not provided by the interface
 * \return 0 if successful, -1 if error
 */
int EIBOpenVBusmonitorTS (EIBConnection * con, uint32_t * timebase);

/** Switches the connection to binary busmonitor mode - asynchronous.
 * \param con eibd connection
 * \param timebase if not null, tick length in ns is stored at this location - it is 0, if not provided by the interface
 * \return 0 if started, -1 if error
 */
int EIBOpenVBusmonitorTS_async (EIBConnection * con, uint32_t * timebase);

/** Receives a packet on a busmonitor connection.
 * \param con eibd connection
 * \param maxlen size of the buffer
 * \param buf buffer
 * \return -1 if error, else length of the packet
 */
int EIBGetBusmonitorPacket (EIBConnection * con, int maxlen, uint8_t * buf);

/** Receives a packet on a busmonitor connection.
 * \param con eibd connection
 * \param status stores KNX busmonitor status field
 * \param timestamp stores relative timestamp
 * \param maxlen size of the buffer
 * \param buf buffer
 * \return -1 if error, else length of the packet
 */
int EIBGetBusmonitorPacketTS (EIBConnection * con, uint8_t * status,
			      uint32_t * timestamp, int maxlen,
			      uint8_t * buf);

/** Opens a connection of type T_Connection.
 * \param con eibd connection
 * \param dest destination address
 * \return 0 if successful, -1 if error
 */
int EIBOpenT_Connection (EIBConnection * con, eibaddr_t dest);

/** Opens a connection of type T_Connection - asynchronous.
 * \param con eibd connection
 * \param dest destination address
 * \return 0 if started, -1 if error
 */
int EIBOpenT_Connection_async (EIBConnection * con, eibaddr_t dest);

/** Opens a connection of type T_Individual.
 * \param con eibd connection
 * \param dest destination address
 * \param write_only if not null, no packets from the bus will be delivered
 * \return 0 if successful, -1 if error
 */
int EIBOpenT_Individual (EIBConnection * con, eibaddr_t dest, int write_only);

/** Opens a connection of type T_Individual - asynchronous.
 * \param con eibd connection
 * \param dest destination address
 * \param write_only if not null, no packets from the bus will be delivered
 * \return 0 if started, -1 if error
 */
int EIBOpenT_Individual_async (EIBConnection * con, eibaddr_t dest,
			       int write_only);

/** Opens a connection of type T_Group.
 * \param con eibd connection
 * \param dest group address
 * \param write_only if not null, no packets from the bus will be delivered
 * \return 0 if successful, -1 if error
 */
int EIBOpenT_Group (EIBConnection * con, eibaddr_t dest, int write_only);

/** Opens a connection of type T_Group - asynchronous.
 * \param con eibd connection
 * \param dest group address
 * \param write_only if not null, no packets from the bus will be delivered
 * \return 0 if started, -1 if error
 */
int EIBOpenT_Group_async (EIBConnection * con, eibaddr_t dest,
			  int write_only);

/** Opens a connection of type T_Broadcast.
 * \param con eibd connection
 * \param write_only if not null, no packets from the bus will be delivered
 * \return 0 if successful, -1 if error
 */
int EIBOpenT_Broadcast (EIBConnection * con, int write_only);

/** Opens a connection of type T_Broadcast - asynchronous.
 * \param con eibd connection
 * \param write_only if not null, no packets from the bus will be delivered
 * \return 0 if started, -1 if error
 */
int EIBOpenT_Broadcast_async (EIBConnection * con, int write_only);

/** Opens a raw Layer 4 connection.
 * \param con eibd connection
 * \param src my source address (0 means default)
 * \return 0 if successful, -1 if error
 */
int EIBOpenT_TPDU (EIBConnection * con, eibaddr_t src);

/** Opens a raw Layer 4 connection - asynchronous.
 * \param con eibd connection
 * \param src my source address (0 means default)
 * \return 0 if started, -1 if error
 */
int EIBOpenT_TPDU_async (EIBConnection * con, eibaddr_t src);

/** Sends an APDU.
 * \param con eibd connection
 * \param len length of the APDU
 * \param data buffer with APDU
 * \return tranmited length or -1 if error
 */
int EIBSendAPDU (EIBConnection * con, int len, const uint8_t * data);

/** Receive an APDU (blocking).
 * \param con eibd connection
 * \param maxlen buffer size
 * \param buf buffer
 * \return received length or -1 if error
 */
int EIBGetAPDU (EIBConnection * con, int maxlen, uint8_t * buf);

/** Receive a APDU with source address (blocking).
 * \param con eibd connection
 * \param maxlen buffer size
 * \param buf buffer
 * \param src pointer, where the source address should be stored
 * \return received length or -1 if error
 */
int EIBGetAPDU_Src (EIBConnection * con, int maxlen, uint8_t * buf,
		    eibaddr_t * src);

/** Sends a TPDU with destination address.
 * \param con eibd connection
 * \param dest destination address
 * \param len length of the APDU
 * \param data buffer with APDU
 * \return tranmited length or -1 if error
 */
int EIBSendTPDU (EIBConnection * con, eibaddr_t dest, int len,
		 const uint8_t * data);

/** Receive a TPDU with source address.
 * \param con eibd connection
 * \param maxlen buffer size
 * \param buf buffer
 * \param src pointer to where the source address should be stored
 * \return received length or -1 if error
 */
int EIBGetTPDU (EIBConnection * con, int maxlen, uint8_t * buf,
		eibaddr_t * src);

/** Opens a Group communication interface.
 * \param con eibd connection
 * \param write_only if not null, no packets from the bus will be delivered
 * \return 0 if successful, -1 if error
 */
int EIBOpen_GroupSocket (EIBConnection * con, int write_only);

/** Opens a Group communication interface - asynchronous.
 * \param con eibd connection
 * \param write_only if not null, no packets from the bus will be delivered
 * \return 0 if started, -1 if error
 */
int EIBOpen_GroupSocket_async (EIBConnection * con, int write_only);

/** Sends a group APDU.
 * \param con eibd connection
 * \param dest destination address
 * \param len length of the APDU
 * \param data buffer with APDU
 * \return tranmited length or -1 if error
 */
int EIBSendGroup (EIBConnection * con, eibaddr_t dest, int len,
		  const uint8_t * data);

/** Receive a group APDU with source address (blocking).
 * \param con eibd connection
 * \param maxlen buffer size
 * \param buf buffer
 * \param src pointer to where the source address should be stored
 * \param dest pointer to where the destination address should be stored
 * \return received length or -1 if error
 */
int EIBGetGroup_Src (EIBConnection * con, int maxlen, uint8_t * buf,
		     eibaddr_t * src, eibaddr_t * dest);

/** List devices in programming mode.
 * \param con eibd connection
 * \param maxlen buffer size
 * \param buf buffer
 * \return number of used bytes in the buffer or -1 if error
 */
int EIB_M_ReadIndividualAddresses (EIBConnection * con, int maxlen,
				   uint8_t * buf);

/** List devices in programming mode - asynchronous.
 * \param con eibd connection
 * \param maxlen buffer size
 * \param buf buffer
 * \return 0 if started, -1 if error
 */
int EIB_M_ReadIndividualAddresses_async (EIBConnection * con, int maxlen,
					 uint8_t * buf);

/** Turn on programming mode (connectionless).
 * \param con eibd connection
 * \param dest address of EIB device
 * \return 0 if successful, -1 if error
 */
int EIB_M_Progmode_On (EIBConnection * con, eibaddr_t dest);

/** Turns on programming mode (connectionless) - asynchronous.
 * \param con eibd connection
 * \param dest address of EIB device
 * \return 0 if started, -1 if error
 */
int EIB_M_Progmode_On_async (EIBConnection * con, eibaddr_t dest);

/** Turns off programming mode (connectionless).
 * \param con eibd connection
 * \param dest address of EIB device
 * \return 0 if successful, -1 if error
 */
int EIB_M_Progmode_Off (EIBConnection * con, eibaddr_t dest);

/** Turns off programming mode (connectionless) - asynchronous.
 * \param con eibd connection
 * \param dest address of EIB device
 * \return 0 if started, -1 if error
 */
int EIB_M_Progmode_Off_async (EIBConnection * con, eibaddr_t dest);

/** Toggle programming mode (connectionless).
 * \param con eibd connection
 * \param dest address of EIB device
 * \return 0 if successful, -1 if error
 */
int EIB_M_Progmode_Toggle (EIBConnection * con, eibaddr_t dest);

/** Toggle programming mode (connectionless) - asynchronous.
 * \param con eibd connection
 * \param dest address of EIB device
 * \return 0 if started, -1 if error
 */
int EIB_M_Progmode_Toggle_async (EIBConnection * con, eibaddr_t dest);

/** Check if a device is in programming mode (connectionless).
 * \param con eibd connection
 * \param dest address of EIB device
 * \return 0 if not in programming mode, -1 if error, else programming mode
 */
int EIB_M_Progmode_Status (EIBConnection * con, eibaddr_t dest);

/** Check if a device is in programming mode (connectionless) - asynchronous.
 * \param con eibd connection
 * \param dest address of EIB device
 * \return 0 if started, -1 if error
 */
int EIB_M_Progmode_Status_async (EIBConnection * con, eibaddr_t dest);

/** Retrieve the mask version (connectionless).
 * \param con eibd connection
 * \param dest address of EIB device
 * \return -1 if error, else mask version
 */
int EIB_M_GetMaskVersion (EIBConnection * con, eibaddr_t dest);

/** Retrieve the mask version (connectionless) - asynchronous.
 * \param con eibd connection
 * \param dest address of EIB device
 * \return 0 if started, -1 if error
 */
int EIB_M_GetMaskVersion_async (EIBConnection * con, eibaddr_t dest);

/** Set individual address for device currently in programming mode.
 * \param con eibd connection
 * \param dest new address of EIB device
 * \return -1 if error, 0 if successful
 */
int EIB_M_WriteIndividualAddress (EIBConnection * con, eibaddr_t dest);

/** Set individual address for device currently in programming mode - asynchronous.
 * \param con eibd connection
 * \param dest new address of EIB device
 * \return 0 if started, -1 if error
 */
int EIB_M_WriteIndividualAddress_async (EIBConnection * con, eibaddr_t dest);

/** Opens a management connection.
 * \param con eibd connection
 * \param dest destination address
 * \return 0 if successful, -1 if error
 */
int EIB_MC_Connect (EIBConnection * con, eibaddr_t dest);

/** Opens a management connection - asynchronous.
 * \param con eibd connection
 * \param dest destination address
 * \return 0 if started, -1 if error
 */
int EIB_MC_Connect_async (EIBConnection * con, eibaddr_t dest);

/** Opens a connectionless management connection.
 * \param con eibd connection
 * \param dest destination address
 * \return 0 if successful, -1 if error
 */
int EIB_MC_Individual_Open (EIBConnection * con, eibaddr_t dest);

/** Opens a connectionless management connection - asynchronous.
 * \param con eibd connection
 * \param dest destination address
 * \return 0 if started, -1 if error
 */
int EIB_MC_Individual_Open_async (EIBConnection * con, eibaddr_t dest);

/** Read BAU memory (over a management connection).
 * \param con eibd connection
 * \param addr memory address
 * \param len size to read
 * \param buf buffer
 * \return -1 if error, else number of bytes read
 */
int EIB_MC_Read (EIBConnection * con, uint16_t addr, int len, uint8_t * buf);

/** Read BAU memory (over a management connection) - asynchronous.
 * \param con eibd connection
 * \param addr memory address
 * \param len size to read
 * \param buf buffer
 * \return 0 if started, -1 if error
 */
int EIB_MC_Read_async (EIBConnection * con, uint16_t addr, int len,
		       uint8_t * buf);

/** Write BAU memory (over a management connection).
 * \param con eibd connection
 * \param addr memory address
 * \param len size to read
 * \param buf buffer
 * \return -1 if error, else number of bytes read
 */
int EIB_MC_Write (EIBConnection * con, uint16_t addr, int len,
		  const uint8_t * buf);

/** Write BAU memory (over a management connection) - asynchronous.
 * \param con eibd connection
 * \param addr Memory address
 * \param len size to read
 * \param buf buffer
 * \return 0 if started, -1 if error
 */
int EIB_MC_Write_async (EIBConnection * con, uint16_t addr, int len,
			const uint8_t * buf);

/** Turns programming mode on (over a management connection).
 * \param con eibd connection
 * \return 0 if successful, -1 if error
 */
int EIB_MC_Progmode_On (EIBConnection * con);

/** Turns programming mode on (over a management connection) - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIB_MC_Progmode_On_async (EIBConnection * con);

/** Turns programming mode off (over a management connection).
 * \param con eibd connection
 * \return 0 if successful, -1 if error
 */
int EIB_MC_Progmode_Off (EIBConnection * con);

/** Turns programming mode off (over a management connection) - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIB_MC_Progmode_Off_async (EIBConnection * con);

/** Toggles programming mode (over a management connection) - asynchronous. 
 * \param con eibd connection
 * \return 0 if successful, -1 if error
 */
int EIB_MC_Progmode_Toggle (EIBConnection * con);

/** Toggles programming mode (over a management connection) - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIB_MC_Progmode_Toggle_async (EIBConnection * con);

/** Check if a device is in programming mode (over a management connection).
 * \param con eibd connection
 * \return 0 if not in programming mode, -1 if error, else programming mode
 */
int EIB_MC_Progmode_Status (EIBConnection * con);

/** Check if a device is in programming mode (over a management connection) - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIB_MC_Progmode_Status_async (EIBConnection * con);

/** Retrieve the mask version (over a management connection).
 * \param con eibd connection
 * \return -1 if error, else mask version
 */
int EIB_MC_GetMaskVersion (EIBConnection * con);

/** Retrieve the mask version (over a management connection) - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIB_MC_GetMaskVersion_async (EIBConnection * con);

/** Read a property (over a management connection).
 * \param con eibd connection
 * \param obj object index
 * \param property property ID
 * \param start start element
 * \param nr_of_elem number of elements
 * \param max_len buffer size
 * \param buf buffer
 * \return -1 if error, else number of bytes read
 */
int EIB_MC_PropertyRead (EIBConnection * con, uint8_t obj, uint8_t property,
			 uint16_t start, uint8_t nr_of_elem, int max_len,
			 uint8_t * buf);

/** Read a property (over a management connection) - asynchronous.
 * \param con eibd connection
 * \param obj object index
 * \param property property ID
 * \param start start element
 * \param nr_of_elem number of elements
 * \param max_len buffer size
 * \param buf buffer
 * \return 0 if started, -1 if error
 */
int EIB_MC_PropertyRead_async (EIBConnection * con, uint8_t obj,
			       uint8_t property, uint16_t start,
			       uint8_t nr_of_elem, int max_len,
			       uint8_t * buf);

/** Write a property (over a management connection).
 * \param con eibd connection
 * \param obj object index
 * \param property property ID
 * \param start start element
 * \param nr_of_elem number of elements
 * \param len buffer size
 * \param buf buffer
 * \param max_len length of the result buffer
 * \param res buffer for the result
 * \return -1 if error, else length of the returned result
 */
int EIB_MC_PropertyWrite (EIBConnection * con, uint8_t obj, uint8_t property,
			  uint16_t start, uint8_t nr_of_elem, int len,
			  const uint8_t * buf, int max_len, uint8_t * res);

/** Write a property (over a management connection) - asynchronous.
 * \param con eibd connection
 * \param obj object index
 * \param property property ID
 * \param start start element
 * \param nr_of_elem number of elements
 * \param len buffer size
 * \param buf buffer
 * \param max_len length of the result buffer
 * \param res buffer for the result
 * \return 0 if started, -1 if error
 */
int EIB_MC_PropertyWrite_async (EIBConnection * con, uint8_t obj,
				uint8_t property, uint16_t start,
				uint8_t nr_of_elem, int len,
				const uint8_t * buf, int max_len,
				uint8_t * res);

/** Read a property description (over a management connection)
 * \param con eibd connection
 * \param obj object index
 * \param property property ID
 * \param type pointer to store type
 * \param max_nr_of_elem pointer to store element count
 * \param access pointer to access level
 * \return -1 if error, else 0
 */
int EIB_MC_PropertyDesc (EIBConnection * con, uint8_t obj, uint8_t property,
			 uint8_t * type, uint16_t * max_nr_of_elem,
			 uint8_t * access);

/** Read a property description (over a mangement connection) - asynchronous.
 * \param con eibd connection
 * \param obj object index
 * \param property property ID
 * \param type pointer to store type
 * \param max_nr_of_elem pointer to store element count
 * \param access pointer to access level
 * \return 0 if started, -1 if error
 */
int EIB_MC_PropertyDesc_async (EIBConnection * con, uint8_t obj,
			       uint8_t property, uint8_t * type,
			       uint16_t * max_nr_of_elem, uint8_t * access);

/** List properties (over a management connection).
 * \param con eibd connection
 * \param maxlen buffer size
 * \param buf buffer
 * \return number of used bytes in the buffer or -1 if error
 */
int EIB_MC_PropertyScan (EIBConnection * con, int maxlen, uint8_t * buf);

/** List properties (over a management connection) - asynchronous.
 * \param con eibd connection
 * \param maxlen buffer size
 * \param buf buffer
 * \return 0 if started, -1 if error
 */
int EIB_MC_PropertyScan_async (EIBConnection * con, int maxlen,
			       uint8_t * buf);

/** Read PEI type (over a management connection).
 * \param con eibd connection
 * \return PEI type or -1 if error
 */
int EIB_MC_GetPEIType (EIBConnection * con);

/** Read PEI type (over a management connection) - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIB_MC_GetPEIType_async (EIBConnection * con);

/** Read ADC value (over a management connection).
 * \param con eibd connection
 * \param channel ADC channel
 * \param count repeat count
 * \param val pointer to store result
 * \return 0, if successful or -1 if error
 */
int EIB_MC_ReadADC (EIBConnection * con, uint8_t channel, uint8_t count,
		    int16_t * val);

/** Read ADC value (over a management connection) - asynchronous.
 * \param con eibd connection
 * \param channel ADC channel
 * \param count repeat count
 * \param val pointer to store result
 * \return 0 if started, -1 if error
 */
int EIB_MC_ReadADC_async (EIBConnection * con, uint8_t channel, uint8_t count,
			  int16_t * val);

/** Authorize (over a management connection).
 * \param con eibd connection
 * \param key key
 * \return -1 if error, else access level
 */
int EIB_MC_Authorize (EIBConnection * con, uint8_t key[4]);

/** Authorize (over a management connection) - asynchronous.
 * \param con eibd connection
 * \param key key
 * \return 0 if started, -1 if error
 */
int EIB_MC_Authorize_async (EIBConnection * con, uint8_t key[4]);

/** Sets a key (over a management connection).
 * \param con eibd connection
 * \param level level to set
 * \param key key
 * \return -1 if error, else 0
 */
int EIB_MC_SetKey (EIBConnection * con, uint8_t key[4], uint8_t level);

/** Sets a key (over a management connection) - asynchronous.
 * \param con eibd connection
 * \param level level to set
 * \param key key
 * \return 0 if started, -1 if error
 */
int EIB_MC_SetKey_async (EIBConnection * con, uint8_t key[4], uint8_t level);

/** Restarts EIB device (management connection will not be useable after this any more) - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIB_MC_Restart_async (EIBConnection * con);

/** Restarts EIB device (management connection will not be useable after this any more).
 * \param con eibd connection
 * \return 0 if successful, -1 if error
 */
int EIB_MC_Restart (EIBConnection * con);

/** Write BAU memory without doing verify (over a management connection).
 * \param con eibd connection
 * \param addr memory address
 * \param len size to read
 * \param buf buffer
 * \return -1 if error, else number of bytes read
 */
int EIB_MC_Write_Plain (EIBConnection * con, uint16_t addr, int len,
			const uint8_t * buf);

/** Write BAU memory without doing verify (over a management connection) - asynchronous.
 * \param con eibd connection
 * \param addr Memory address
 * \param len size to read
 * \param buf buffer
 * \return 0 if started, -1 if error
 */
int EIB_MC_Write_Plain_async (EIBConnection * con, uint16_t addr, int len,
			      const uint8_t * buf);


/** Loads a BCU SDK program image (over a management connection).
 * \param con eibd connection
 * \param image pointer to image
 * \param len legth of the image
 * \return result (for values see IMG_* constants), -1 if communication error with eibd
 */
int EIB_LoadImage (EIBConnection * con, int len, const uint8_t * image);

/** Loads a BCU SDK program image (over a management connection) - asynchronous.
 * \param con eibd connection
 * \param image pointer to image
 * \param len legth of the image
 * \return 0 if started, -1 if error
 */
int EIB_LoadImage_async (EIBConnection * con, int len, const uint8_t * image);


/** Enable Group Cache
 * \param con eibd connection
 * \return 0 if successful, -1 if error (EBUSY if KNX/EIB transmissions are blocked,  e.g., because a busmonitor is active)
 */
int EIB_Cache_Enable (EIBConnection * con);

/** Disable Group Cache
 * \param con eibd connection
 * \return 0 if successful, -1 if error
 */
int EIB_Cache_Disable (EIBConnection * con);

/** Clear Group Cache (all group addresses)
 * \param con eibd connection
 * \return 0 if successful, -1 if error
 */
int EIB_Cache_Clear (EIBConnection * con);

/** Remove all cache data for a group address
 * \param con eibd connection
 * \param dest group address
 * \return 0 if successful, -1 if error
 */
int EIB_Cache_Remove (EIBConnection * con, eibaddr_t dest);

/** Query the last value sent to a group address; if not in cache, try to get it via A_GroupValue_Read
 * \param con eibd connection
 * \param dest group address
 * \param src source address of the last APDU
 * \param max_len buffer size
 * \param buf buffer for last APDU
 * \param age if non-zero, send a A_GroupValue_Read, if the cached telegram is older than age seconds.
 * \return -1 if error (ENODEV=group cache not enabled, ENOENT=A_GroupValue_Read request was not answered), else length of APDU
 */
int EIB_Cache_Read_Sync (EIBConnection * con, eibaddr_t dest, eibaddr_t * src,
			 int max_len, uint8_t * buf, uint16_t age);

/** Query the last value sent to a group address
 * \param con eibd connection
 * \param dest group address
 * \param src source address of the last APDU
 * \param max_len buffer size
 * \param buf buffer for last APDU
 * \return -1 if error (ENODEV=group cache not enabled, ENOENT=no telegram in cache), else length of APDU
 */
int EIB_Cache_Read (EIBConnection * con, eibaddr_t dest,
		    eibaddr_t * src, int max_len, uint8_t * buf);

/** Returns a list of the last updates in the groupcache
 * \param con eibd connection
 * \param start start position (use 0 for first request)
 * \param timeout maximum time to wait in seconds, if there is no data
 * \param max_len buffer size
 * \param buf buffer for the returned group addresses (2 bytes per address) 
 * \param end position for the next request
 * \return -1 if error, else number of bytes read
 */
int EIB_Cache_LastUpdates (EIBConnection * con, uint16_t start,
			   uint8_t timeout, int max_len, uint8_t * buf,
			   uint16_t * end);
int EIB_Cache_LastUpdates2 (EIBConnection * con, uint32_t start,
			    uint8_t timeout, int max_len, uint8_t * buf,
			    uint32_t * end);

/** Enable Group Cache - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIB_Cache_Enable_async (EIBConnection * con);

/** Disable Group Cache - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIB_Cache_Disable_async (EIBConnection * con);

/** Clear Group Cache (all group addresses) - asynchronous.
 * \param con eibd connection
 * \return 0 if started, -1 if error
 */
int EIB_Cache_Clear_async (EIBConnection * con);

/** Remove all cache data for a group address - asynchronous.
 * \param con eibd connection
 * \param dest group address
 * \return 0 if started, -1 if error
 */
int EIB_Cache_Remove_async (EIBConnection * con, eibaddr_t dest);

/** Query the last value sent to a group address; if not in cache, try to get it via A_GroupValue_Read - asynchronous.
 * \param con eibd connection
 * \param dest group address
 * \param src source address of the last APDU
 * \param max_len buffer size
 * \param buf buffer for last APDU
 * \param age if non-zero, send a A_GroupValue_Read, if the cached telegram is older than age seconds.
 * \return 0 if started, -1 if error
 */
int EIB_Cache_Read_Sync_async (EIBConnection * con, eibaddr_t dest,
			       eibaddr_t * src, int max_len, uint8_t * buf,
			       uint16_t age);

/** Query the last value sent to a group address - asynchronous.
 * \param con eibd connection
 * \param dest group address
 * \param src source address of the last APDU
 * \param max_len buffer size
 * \param buf buffer for last APDU
 * \return 0 if started, -1 if error
 */
int EIB_Cache_Read_async (EIBConnection * con, eibaddr_t dest,
			  eibaddr_t * src, int max_len, uint8_t * buf);


/** Returns a list of the last updates in the groupcache - asynchronous.
 * \param con eibd connection
 * \param start start position (use 0 for first request)
 * \param timeout maximum time to wait in seconds, if there is no data
 * \param max_len buffer size
 * \param buf buffer for the returned group addresses (2 bytes per address) 
 * \param end position for the next request
 * \return 0 if started, -1 if error
 */
int EIB_Cache_LastUpdates_async (EIBConnection * con, uint16_t start,
				 uint8_t timeout, int max_len, uint8_t * buf,
				 uint16_t * end);
int EIB_Cache_LastUpdates2_async (EIBConnection * con, uint32_t start,
				  uint8_t timeout, int max_len, uint8_t * buf,
				  uint32_t * end);


__END_DECLS
#endif
