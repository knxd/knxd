/*
    KNX client library - internals
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>
    Copyright (C) 2014 Patrik Pfaffenbauer <patrik.pfaffenbauer@p3.co.at>

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

#ifndef DPTCONVERT_H
#define DPTCONVERT_H

#include <eibclient-int.h>

//KNX to internal
int busValueToBinary(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToBinaryControl(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToStepControl(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToCharacter(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToUnsigned8(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToSigned8(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToStatusAndMode(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToUnsigned16(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToTimePeriod(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToSigned16(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToTimeDelta(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToFloat16(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToTime(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToDate(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToUnsigned32(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToSigned32(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToLongTimePeriod(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToFloat32(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToAccess(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToString(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToScene(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToSceneControl(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToSceneInfo(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToSceneConfig(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToDateTime(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToUnicode(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToSigned64(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToAlarmInfo(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToSerialNumber(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToVersion(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToScaling(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToTariff(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToLocale(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToRGB(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToFlaggedScaling(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);
int busValueToActiveEnergy(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value);

//Internal to KNX
int valueToBusValueBinary(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueBinaryControl(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueStepControl(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueCharacter(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueUnsigned8(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueSigned8(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueStatusAndMode(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueUnsigned16(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueTimePeriod(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueSigned16(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueTimeDelta(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueFloat16(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueTime(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueDate(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueUnsigned32(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueSigned32(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueLongTimePeriod(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueFloat32(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueAccess(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueString(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueScene(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueSceneControl(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueSceneInfo(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueSceneConfig(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueDateTime(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueUnicode(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueSigned64(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueAlarmInfo(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueSerialNumber(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueVersion(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueScaling(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueTariff(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueLocale(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueRGB(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueFlaggedScaling(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);
int valueToBusValueActiveEnergy(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype);

//Payload manipulation
int bitFromPayload(const uint8_t *payload, int index);
uint8_t unsigned8FromPayload(const uint8_t *payload, int index);
int8_t signed8FromPayload(const uint8_t *payload, int index);
uint16_t unsigned16FromPayload(const uint8_t *payload, int index);
int16_t signed16FromPayload(const uint8_t *payload, int index);
uint32_t unsigned32FromPayload(const uint8_t *payload, int index);
int32_t signed32FromPayload(const uint8_t *payload, int index);
double float16FromPayload(const uint8_t *payload, int index);
float float32FromPayload(const uint8_t *payload, int index);
int64_t signed64FromPayload(const uint8_t *payload, int index);
uint8_t bcdFromPayload(const uint8_t *payload, int index);

void bitToPayload(uint8_t *payload, int payload_length, int index, int value);
void unsigned8ToPayload(uint8_t *payload, int payload_length, int index, uint8_t value, uint8_t mask); //mask 0xFF
void signed8ToPayload(uint8_t *payload, int payload_length, int index, int8_t value, uint8_t mask); //mask 0xFF
void unsigned16ToPayload(uint8_t *payload, int payload_length, int index, uint16_t value, uint16_t mask); //mask 0xFFFF
void signed16ToPayload(uint8_t *payload, int payload_length, int index, int16_t value, uint16_t mask); //mask 0xFFFF
void unsigned32ToPayload(uint8_t *payload, int payload_length, int index, uint32_t value, uint32_t mask); //mask = 0xFFFFFFFF
void signed32ToPayload(uint8_t *payload, int payload_length, int index, int32_t value, uint32_t mask); //mask  = 0xFFFFFFFF
void float16ToPayload(uint8_t *payload, int payload_length, int index, double value, uint16_t mask); //mask = 0xFFFF
void float32ToPayload(uint8_t *payload, int payload_length, int index, double value, uint32_t mask); //mask  = 0xFFFFFFFF
void signed64ToPayload(uint8_t *payload, int payload_length, int index, int64_t value, uint64_t mask); //mask = UINT64_C(0xFFFFFFFFFFFFFFFF)
void bcdToPayload(uint8_t *payload, int payload_length, int index, uint8_t value);


#endif
