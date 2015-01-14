#include <dptconvert.h>
#include <math.h>
#include <stdlib.h>
#include <strings.h>

#define false 0
#define true 1

#define ASSERT_PAYLOAD(x) if (payload_length != (x))  return false
#define ENSURE_PAYLOAD(x) for (int pi = payload_length; pi < (x); ++pi) payload[pi] = 0

#define KNX_ASSUME_VALUE(y) value->bValue = y; value->iValue = y; value->cValue = y; value->sValue = y; value->uiValue = y; value->dValue = y;
#define KNX_ASSUME_CHAR_VALUE(y) KNX_ASSUME_VALUE(y)

#define KNX_ASSUME_VALUE_RET(y) KNX_ASSUME_VALUE(y) return true
#define KNX_ASSUME_STR_VALUE_RET(value, y) value->strValue = (char*)malloc(strlen(y)); strncpy(value->strValue, y, strlen(y)); return true
#define KNX_ASSUME_CHAR_VALUE_RET(y) KNX_ASSUME_CHAR_VALUE(y) return true


int KNX_APDU_To_Payload(uint8_t *apdu, int apdu_len, uint8_t *payload, int payload_max_len) {

    if (apdu_len == 2) {
        payload[0] = (apdu[1] & 0x3F);
        return true;
    }
    else {
        if((apdu_len-2) > payload_max_len)
            return false;
        memcpy(payload, apdu+2, apdu_len-2);
        return apdu_len-2;
    }

}

int KNX_Decode_Value(uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    if(payload_length > 0) {
        // DPT 1.* - Binary
        if (datatype.mainGroup == 1 && datatype.subGroup >= 1 && datatype.subGroup <= 23 && datatype.subGroup != 20 && !datatype.index) {
            return busValueToBinary(payload, payload_length, datatype, value);
        }// DPT 2.* - Binary Control
        if (datatype.mainGroup == 2 && datatype.subGroup >= 1 && datatype.subGroup <= 12 && datatype.index <= 1)
            return busValueToBinaryControl(payload, payload_length, datatype, value);
        // DPT 3.* - Step Control
        if (datatype.mainGroup == 3 && datatype.subGroup >= 7 && datatype.subGroup <= 8 && datatype.index <= 1)
            return busValueToStepControl(payload, payload_length, datatype, value);
        // DPT 4.* - Character// DPT 2.* - Binary Control
        if (datatype.mainGroup == 2 && datatype.subGroup >= 1 && datatype.subGroup <= 12 && datatype.index <= 1)
            return busValueToBinaryControl(payload, payload_length, datatype, value);
        // DPT 3.* - Step Control
        if (datatype.mainGroup == 3 && datatype.subGroup >= 7 && datatype.subGroup <= 8 && datatype.index <= 1)
            return busValueToStepControl(payload, payload_length, datatype, value);
        // DPT 4.* - Character
        if (datatype.mainGroup == 4 && datatype.subGroup >= 1 && datatype.subGroup <= 2 && !datatype.index)
            return busValueToCharacter(payload, payload_length, datatype, value);
        // DPT 5.* - Unsigned 8 Bit Integer
        if (datatype.mainGroup == 5 && ((datatype.subGroup >= 1 && datatype.subGroup <= 6 && datatype.subGroup != 2) || datatype.subGroup == 10) && !datatype.index)
            return busValueToUnsigned8(payload, payload_length, datatype, value);
        // DPT 6.001/6.010 - Signed 8 Bit Integer
        if (datatype.mainGroup == 6 && (datatype.subGroup == 1 || datatype.subGroup == 10) && !datatype.index)
            return busValueToSigned8(payload, payload_length, datatype, value);
        // DPT 6.020 - Status with Mode
        if (datatype.mainGroup == 6 && datatype.subGroup == 20 && datatype.index <= 5)
            return busValueToStatusAndMode(payload, payload_length, datatype, value);
        // DPT 7.001/7.010/7.011/7.012/7.013 - Unsigned 16 Bit Integer
        if (datatype.mainGroup == 7 && (datatype.subGroup == 1 || (datatype.subGroup >= 10 && datatype.subGroup <= 13)) && !datatype.index)
            return busValueToUnsigned16(payload, payload_length, datatype, value);
        // DPT 7.002-DPT 7.007 - Time Period
        if (datatype.mainGroup == 7 && datatype.subGroup >= 2 && datatype.subGroup <= 7 && !datatype.index)
            return busValueToTimePeriod(payload, payload_length, datatype, value);
        // DPT 8.001/8.010/8.011 - Signed 16 Bit Integer
        if (datatype.mainGroup == 8 && (datatype.subGroup == 1 || datatype.subGroup == 10 || datatype.subGroup == 11) && !datatype.index)
            return busValueToSigned16(payload, payload_length, datatype, value);
        // DPT 8.002-DPT 8.007 - Time Delta
        if (datatype.mainGroup == 8 && datatype.subGroup >= 2 && datatype.subGroup <= 7 && !datatype.index)
            return busValueToTimeDelta(payload, payload_length, datatype, value);
        // DPT 9.* - 16 Bit Float
        if (datatype.mainGroup == 9 && ((datatype.subGroup >= 1 && datatype.subGroup <= 11 && datatype.subGroup != 9) || (datatype.subGroup >= 20 && datatype.subGroup <= 28)) && !datatype.index)
            return busValueToFloat16(payload, payload_length, datatype, value);
        // DPT 10.* - Time and Weekday
        if (datatype.mainGroup == 10 && datatype.subGroup == 1 && datatype.index <= 1)
            return busValueToTime(payload, payload_length, datatype, value);
        // DPT 11.* - Date
        if (datatype.mainGroup == 11 && datatype.subGroup == 1 && !datatype.index)
            return busValueToDate(payload, payload_length, datatype, value);
        // DPT 12.* - Unsigned 32 Bit Integer
        if (datatype.mainGroup == 12 && datatype.subGroup == 1 && !datatype.index)
            return busValueToUnsigned32(payload, payload_length, datatype, value);
        // DPT 13.001/13.002/13.010-13.015 - Signed 32 Bit Integer
        if (datatype.mainGroup == 13 && (datatype.subGroup == 1 || datatype.subGroup == 2 || (datatype.subGroup >= 10 && datatype.subGroup <= 15)) && !datatype.index)
            return busValueToSigned32(payload, payload_length, datatype, value);
        // DPT 13.100 - Long Time Period
        if (datatype.mainGroup == 13 && datatype.subGroup == 100 && !datatype.index)
            return busValueToLongTimePeriod(payload, payload_length, datatype, value);
        // DPT 14.* - 32 Bit Float
        if (datatype.mainGroup == 14 && datatype.subGroup <= 79 && !datatype.index)
            return busValueToFloat32(payload, payload_length, datatype, value);
        // DPT 15.* - Access Data
        if (datatype.mainGroup == 15 && !datatype.subGroup && datatype.index <= 5)
            return busValueToAccess(payload, payload_length, datatype, value);
        // DPT 16.* - String
        if (datatype.mainGroup == 16 && datatype.subGroup <= 1 && !datatype.index)
            return busValueToString(payload, payload_length, datatype, value);
        // DPT 17.* - Scene Number
        if (datatype.mainGroup == 17 && datatype.subGroup == 1 && !datatype.index)
            return busValueToScene(payload, payload_length, datatype, value);
        // DPT 18.* - Scene Control
        if (datatype.mainGroup == 18 && datatype.subGroup == 1 && datatype.index <= 1)
            return busValueToSceneControl(payload, payload_length, datatype, value);
        // DPT 19.* - Date and Time
        if (datatype.mainGroup == 19 && datatype.subGroup == 1 && (datatype.index <= 3 || datatype.index == 9 || datatype.index == 10))
            return busValueToDateTime(payload, payload_length, datatype, value);
        // DPT 26.* - Scene Info
        if (datatype.mainGroup == 26 && datatype.subGroup == 1 && datatype.index <= 1)
            return busValueToSceneInfo(payload, payload_length, datatype, value);
        // DPT 28.* - Unicode String
        if (datatype.mainGroup == 28 && datatype.subGroup == 1 && !datatype.index)
            return busValueToUnicode(payload, payload_length, datatype, value);
        // DPT 29.* - Signed 64 Bit Integer
        if (datatype.mainGroup == 29 && datatype.subGroup >= 10 && datatype.subGroup <= 12 && !datatype.index)
            return busValueToSigned64(payload, payload_length, datatype, value);
        // DPT 219.* - Alarm Info
        if (datatype.mainGroup == 219 && datatype.subGroup == 1 && datatype.index <= 10)
            return busValueToAlarmInfo(payload, payload_length, datatype, value);
        // DPT 221.* - Serial Number
        if (datatype.mainGroup == 221 && datatype.subGroup == 1 && datatype.index <= 1)
            return busValueToSerialNumber(payload, payload_length, datatype, value);
        // DPT 217.* - Version
        if (datatype.mainGroup == 217 && datatype.subGroup == 1 && datatype.index <= 2)
            return busValueToVersion(payload, payload_length, datatype, value);
        // DPT 225.001/225.002 - Scaling Speed and Scaling Step Time
        if (datatype.mainGroup == 225 && datatype.subGroup >= 1 && datatype.subGroup <= 2 && datatype.index <= 1)
            return busValueToScaling(payload, payload_length, datatype, value);
        // DPT 225.003 - Next Tariff
        if (datatype.mainGroup == 225 && datatype.subGroup == 3 && datatype.index <= 1)
            return busValueToTariff(payload, payload_length, datatype, value);
        // DPT 231.* - Locale
        if (datatype.mainGroup == 231 && datatype.subGroup == 1 && datatype.index <= 1)
            return busValueToLocale(payload, payload_length, datatype, value);
        // DPT 232.600 - RGB
        if (datatype.mainGroup == 232 && datatype.subGroup == 600 && !datatype.index)
            return busValueToRGB(payload, payload_length, datatype, value);
        // DPT 234.* - Language and Region
        if (datatype.mainGroup == 234 && datatype.subGroup >= 1 && datatype.subGroup <= 2 && !datatype.index)
            return busValueToLocale(payload, payload_length, datatype, value);
        // DPT 235.* - Active Energy
        if (datatype.mainGroup == 235 && datatype.subGroup == 1 && datatype.index <= 3)
            return busValueToActiveEnergy(payload, payload_length, datatype, value);
        // DPT 238.* - Scene Config
        if (datatype.mainGroup == 238 && datatype.subGroup == 1 && datatype.index <= 2)
            return busValueToSceneConfig(payload, payload_length, datatype, value);
        // DPT 239.* - Flagged Scaling
        if (datatype.mainGroup == 239 && datatype.subGroup == 1 && datatype.index <= 1)
            return busValueToFlaggedScaling(payload, payload_length, datatype, value);
        if (datatype.mainGroup == 4 && datatype.subGroup >= 1 && datatype.subGroup <= 2 && !datatype.index)
            return busValueToCharacter(payload, payload_length, datatype, value);
        // DPT 5.* - Unsigned 8 Bit Integer
        if (datatype.mainGroup == 5 && ((datatype.subGroup >= 1 && datatype.subGroup <= 6 && datatype.subGroup != 2) || datatype.subGroup == 10) && !datatype.index)
            return busValueToUnsigned8(payload, payload_length, datatype, value);
        // DPT 6.001/6.010 - Signed 8 Bit Integer
        if (datatype.mainGroup == 6 && (datatype.subGroup == 1 || datatype.subGroup == 10) && !datatype.index)
            return busValueToSigned8(payload, payload_length, datatype, value);
        // DPT 6.020 - Status with Mode
        if (datatype.mainGroup == 6 && datatype.subGroup == 20 && datatype.index <= 5)
            return busValueToStatusAndMode(payload, payload_length, datatype, value);
        // DPT 7.001/7.010/7.011/7.012/7.013 - Unsigned 16 Bit Integer
        if (datatype.mainGroup == 7 && (datatype.subGroup == 1 || (datatype.subGroup >= 10 && datatype.subGroup <= 13)) && !datatype.index)
            return busValueToUnsigned16(payload, payload_length, datatype, value);
        // DPT 7.002-DPT 7.007 - Time Period
        if (datatype.mainGroup == 7 && datatype.subGroup >= 2 && datatype.subGroup <= 7 && !datatype.index)
            return busValueToTimePeriod(payload, payload_length, datatype, value);
        // DPT 8.001/8.010/8.011 - Signed 16 Bit Integer
        if (datatype.mainGroup == 8 && (datatype.subGroup == 1 || datatype.subGroup == 10 || datatype.subGroup == 11) && !datatype.index)
            return busValueToSigned16(payload, payload_length, datatype, value);
        // DPT 8.002-DPT 8.007 - Time Delta
        if (datatype.mainGroup == 8 && datatype.subGroup >= 2 && datatype.subGroup <= 7 && !datatype.index)
            return busValueToTimeDelta(payload, payload_length, datatype, value);
        // DPT 9.* - 16 Bit Float
        if (datatype.mainGroup == 9 && ((datatype.subGroup >= 1 && datatype.subGroup <= 11 && datatype.subGroup != 9) || (datatype.subGroup >= 20 && datatype.subGroup <= 28)) && !datatype.index)
            return busValueToFloat16(payload, payload_length, datatype, value);
        // DPT 10.* - Time and Weekday
        if (datatype.mainGroup == 10 && datatype.subGroup == 1 && datatype.index <= 1)
            return busValueToTime(payload, payload_length, datatype, value);
        // DPT 11.* - Date
        if (datatype.mainGroup == 11 && datatype.subGroup == 1 && !datatype.index)
            return busValueToDate(payload, payload_length, datatype, value);
        // DPT 12.* - Unsigned 32 Bit Integer
        if (datatype.mainGroup == 12 && datatype.subGroup == 1 && !datatype.index)
            return busValueToUnsigned32(payload, payload_length, datatype, value);
        // DPT 13.001/13.002/13.010-13.015 - Signed 32 Bit Integer
        if (datatype.mainGroup == 13 && (datatype.subGroup == 1 || datatype.subGroup == 2 || (datatype.subGroup >= 10 && datatype.subGroup <= 15)) && !datatype.index)
            return busValueToSigned32(payload, payload_length, datatype, value);
        // DPT 13.100 - Long Time Period
        if (datatype.mainGroup == 13 && datatype.subGroup == 100 && !datatype.index)
            return busValueToLongTimePeriod(payload, payload_length, datatype, value);
        // DPT 14.* - 32 Bit Float
        if (datatype.mainGroup == 14 && datatype.subGroup <= 79 && !datatype.index)
            return busValueToFloat32(payload, payload_length, datatype, value);
        // DPT 15.* - Access Data
        if (datatype.mainGroup == 15 && !datatype.subGroup && datatype.index <= 5)
            return busValueToAccess(payload, payload_length, datatype, value);
        // DPT 16.* - String
        if (datatype.mainGroup == 16 && datatype.subGroup <= 1 && !datatype.index)
            return busValueToString(payload, payload_length, datatype, value);
        // DPT 17.* - Scene Number
        if (datatype.mainGroup == 17 && datatype.subGroup == 1 && !datatype.index)
            return busValueToScene(payload, payload_length, datatype, value);
        // DPT 18.* - Scene Control
        if (datatype.mainGroup == 18 && datatype.subGroup == 1 && datatype.index <= 1)
            return busValueToSceneControl(payload, payload_length, datatype, value);
        // DPT 19.* - Date and Time
        if (datatype.mainGroup == 19 && datatype.subGroup == 1 && (datatype.index <= 3 || datatype.index == 9 || datatype.index == 10))
            return busValueToDateTime(payload, payload_length, datatype, value);
        // DPT 26.* - Scene Info
        if (datatype.mainGroup == 26 && datatype.subGroup == 1 && datatype.index <= 1)
            return busValueToSceneInfo(payload, payload_length, datatype, value);
        // DPT 28.* - Unicode String
        if (datatype.mainGroup == 28 && datatype.subGroup == 1 && !datatype.index)
            return busValueToUnicode(payload, payload_length, datatype, value);
        // DPT 29.* - Signed 64 Bit Integer
        if (datatype.mainGroup == 29 && datatype.subGroup >= 10 && datatype.subGroup <= 12 && !datatype.index)
            return busValueToSigned64(payload, payload_length, datatype, value);
        // DPT 219.* - Alarm Info
        if (datatype.mainGroup == 219 && datatype.subGroup == 1 && datatype.index <= 10)
            return busValueToAlarmInfo(payload, payload_length, datatype, value);
        // DPT 221.* - Serial Number
        if (datatype.mainGroup == 221 && datatype.subGroup == 1 && datatype.index <= 1)
            return busValueToSerialNumber(payload, payload_length, datatype, value);
        // DPT 217.* - Version
        if (datatype.mainGroup == 217 && datatype.subGroup == 1 && datatype.index <= 2)
            return busValueToVersion(payload, payload_length, datatype, value);
        // DPT 225.001/225.002 - Scaling Speed and Scaling Step Time
        if (datatype.mainGroup == 225 && datatype.subGroup >= 1 && datatype.subGroup <= 2 && datatype.index <= 1)
            return busValueToScaling(payload, payload_length, datatype, value);
        // DPT 225.003 - Next Tariff
        if (datatype.mainGroup == 225 && datatype.subGroup == 3 && datatype.index <= 1)
            return busValueToTariff(payload, payload_length, datatype, value);
        // DPT 231.* - Locale
        if (datatype.mainGroup == 231 && datatype.subGroup == 1 && datatype.index <= 1)
            return busValueToLocale(payload, payload_length, datatype, value);
        // DPT 232.600 - RGB
        if (datatype.mainGroup == 232 && datatype.subGroup == 600 && !datatype.index)
            return busValueToRGB(payload, payload_length, datatype, value);
        // DPT 234.* - Language and Region
        if (datatype.mainGroup == 234 && datatype.subGroup >= 1 && datatype.subGroup <= 2 && !datatype.index)
            return busValueToLocale(payload, payload_length, datatype, value);
        // DPT 235.* - Active Energy
        if (datatype.mainGroup == 235 && datatype.subGroup == 1 && datatype.index <= 3)
            return busValueToActiveEnergy(payload, payload_length, datatype, value);
        // DPT 238.* - Scene Config
        if (datatype.mainGroup == 238 && datatype.subGroup == 1 && datatype.index <= 2)
            return busValueToSceneConfig(payload, payload_length, datatype, value);
        // DPT 239.* - Flagged Scaling
        if (datatype.mainGroup == 239 && datatype.subGroup == 1 && datatype.index <= 1)
            return busValueToFlaggedScaling(payload, payload_length, datatype, value);
    }
    return false;
}

int KNX_Encode_Value(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {

    if (datatype.mainGroup == 1 && datatype.subGroup >= 1 && datatype.subGroup <= 23 && datatype.subGroup != 20 && !datatype.index)
        return valueToBusValueBinary(value, payload, payload_length, datatype);
    // DPT 2.* - Binary Control
    if (datatype.mainGroup == 2 && datatype.subGroup >= 1 && datatype.subGroup <= 12 && datatype.index <= 1)
        return valueToBusValueBinaryControl(value, payload, payload_length, datatype);
    // DPT 3.* - Step Control
    if (datatype.mainGroup == 3 && datatype.subGroup >= 7 && datatype.subGroup <= 8 && datatype.index <= 1)
        return valueToBusValueStepControl(value, payload, payload_length, datatype);
    // DPT 4.* - Character
    if (datatype.mainGroup == 4 && datatype.subGroup >= 1 && datatype.subGroup <= 2 && !datatype.index)
        return valueToBusValueCharacter(value, payload, payload_length, datatype);
    // DPT 5.* - Unsigned 8 Bit Integer
    if (datatype.mainGroup == 5 && ((datatype.subGroup >= 1 && datatype.subGroup <= 6 && datatype.subGroup != 2) || datatype.subGroup == 10) && !datatype.index)
        return valueToBusValueUnsigned8(value, payload, payload_length, datatype);
    // DPT 6.001/6.010 - Signed 8 Bit Integer
    if (datatype.mainGroup == 6 && (datatype.subGroup == 1 || datatype.subGroup == 10) && !datatype.index)
        return valueToBusValueSigned8(value, payload, payload_length, datatype);
    // DPT 6.020 - Status with Mode
    if (datatype.mainGroup == 6 && datatype.subGroup == 20 && datatype.index <= 5)
        return valueToBusValueStatusAndMode(value, payload, payload_length, datatype);
    // DPT 7.001/7.010/7.011/7.012/7.013 - Unsigned 16 Bit Integer
    if (datatype.mainGroup == 7 && (datatype.subGroup == 1 || (datatype.subGroup >= 10 && datatype.subGroup <= 13)) && !datatype.index)
        return valueToBusValueUnsigned16(value, payload, payload_length, datatype);
    // DPT 7.002-DPT 7.007 - Time Period
    if (datatype.mainGroup == 7 && datatype.subGroup >= 2 && datatype.subGroup <= 7 && !datatype.index)
        return valueToBusValueTimePeriod(value, payload, payload_length, datatype);
    // DPT 8.001/8.010/8.011 - Signed 16 Bit Integer
    if (datatype.mainGroup == 8 && (datatype.subGroup == 1 || datatype.subGroup == 10 || datatype.subGroup == 11) && !datatype.index)
        return valueToBusValueSigned16(value, payload, payload_length, datatype);
    // DPT 8.002-DPT 8.007 - Time Delta
    if (datatype.mainGroup == 8 && datatype.subGroup >= 2 && datatype.subGroup <= 7 && !datatype.index)
        return valueToBusValueTimeDelta(value, payload, payload_length, datatype);
    // DPT 9.* - 16 Bit Float
    if (datatype.mainGroup == 9 && ((datatype.subGroup >= 1 && datatype.subGroup <= 11 && datatype.subGroup != 9) || (datatype.subGroup >= 20 && datatype.subGroup <= 28)) && !datatype.index)
        return valueToBusValueFloat16(value, payload, payload_length, datatype);
    // DPT 10.* - Time and Weekday
    if (datatype.mainGroup == 10 && datatype.subGroup == 1 && datatype.index <= 1)
        return valueToBusValueTime(value, payload, payload_length, datatype);
    // DPT 11.* - Date
    if (datatype.mainGroup == 11 && datatype.subGroup == 1 && !datatype.index)
        return valueToBusValueDate(value, payload, payload_length, datatype);
    // DPT 12.* - Unsigned 32 Bit Integer
    if (datatype.mainGroup == 12 && datatype.subGroup == 1 && !datatype.index)
        return valueToBusValueUnsigned32(value, payload, payload_length, datatype);
    // DPT 13.001/13.002/13.010-13.015 - Signed 32 Bit Integer
    if (datatype.mainGroup == 13 && (datatype.subGroup == 1 || datatype.subGroup == 2 || (datatype.subGroup >= 10 && datatype.subGroup <= 15)) && !datatype.index)
        return valueToBusValueSigned32(value, payload, payload_length, datatype);
    // DPT 13.100 - Long Time Period
    if (datatype.mainGroup == 13 && datatype.subGroup == 100 && !datatype.index)
        return valueToBusValueLongTimePeriod(value, payload, payload_length, datatype);
    // DPT 14.* - 32 Bit Float
    if (datatype.mainGroup == 14 && datatype.subGroup <= 79 && !datatype.index)
        return valueToBusValueFloat32(value, payload, payload_length, datatype);
    // DPT 15.* - Access Data
    if (datatype.mainGroup == 15 && !datatype.subGroup && datatype.index <= 5)
        return valueToBusValueAccess(value, payload, payload_length, datatype);
    // DPT 16.* - String
    if (datatype.mainGroup == 16 && datatype.subGroup <= 1 && !datatype.index)
        return valueToBusValueString(value, payload, payload_length, datatype);
    // DPT 17.* - Scene Number
    if (datatype.mainGroup == 17 && datatype.subGroup == 1 && !datatype.index)
        return valueToBusValueScene(value, payload, payload_length, datatype);
    // DPT 18.* - Scene Control
    if (datatype.mainGroup == 18 && datatype.subGroup == 1 && datatype.index <= 1)
        return valueToBusValueSceneControl(value, payload, payload_length, datatype);
    // DPT 19.* - Date and Time
    if (datatype.mainGroup == 19 && datatype.subGroup == 1 && (datatype.index <= 3 || datatype.index == 9 || datatype.index == 10))
        return valueToBusValueDateTime(value, payload, payload_length, datatype);
    // DPT 26.* - Scene Info
    if (datatype.mainGroup == 26 && datatype.subGroup == 1 && datatype.index <= 1)
        return valueToBusValueSceneInfo(value, payload, payload_length, datatype);
    // DPT 28.* - Unicode String
    if (datatype.mainGroup == 28 && datatype.subGroup == 1 && !datatype.index)
        return valueToBusValueUnicode(value, payload, payload_length, datatype);
    // DPT 29.* - Signed 64 Bit Integer
    if (datatype.mainGroup == 29 && datatype.subGroup >= 10 && datatype.subGroup <= 12 && !datatype.index)
        return valueToBusValueSigned64(value, payload, payload_length, datatype);
    // DPT 219.* - Alarm Info
    if (datatype.mainGroup == 219 && datatype.subGroup == 1 && datatype.index <= 10)
        return valueToBusValueAlarmInfo(value, payload, payload_length, datatype);
    // DPT 221.* - Serial Number
    if (datatype.mainGroup == 221 && datatype.subGroup == 1 && datatype.index <= 1)
        return valueToBusValueSerialNumber(value, payload, payload_length, datatype);
    // DPT 217.* - Version
    if (datatype.mainGroup == 217 && datatype.subGroup == 1 && datatype.index <= 2)
        return valueToBusValueVersion(value, payload, payload_length, datatype);
    // DPT 225.001/225.002 - Scaling Speed and Scaling Step Time
    if (datatype.mainGroup == 225 && datatype.subGroup >= 1 && datatype.subGroup <= 2 && datatype.index <= 1)
        return valueToBusValueScaling(value, payload, payload_length, datatype);
    // DPT 225.003 - Next Tariff
    if (datatype.mainGroup == 225 && datatype.subGroup == 3 && datatype.index <= 1)
        return valueToBusValueTariff(value, payload, payload_length, datatype);
    // DPT 231.* - Locale
    if (datatype.mainGroup == 231 && datatype.subGroup == 1 && datatype.index <= 1)
        return valueToBusValueLocale(value, payload, payload_length, datatype);
    // DPT 232.600 - RGB
    if (datatype.mainGroup == 232 && datatype.subGroup == 600 && !datatype.index)
        return valueToBusValueRGB(value, payload, payload_length, datatype);
    // DPT 234.* - Language and Region
    if (datatype.mainGroup == 234 && datatype.subGroup >= 1 && datatype.subGroup <= 2 && !datatype.index)
        return valueToBusValueLocale(value, payload, payload_length, datatype);
    // DPT 235.* - Active Energy
    if (datatype.mainGroup == 235 && datatype.subGroup == 1 && datatype.index <= 3)
        return valueToBusValueActiveEnergy(value, payload, payload_length, datatype);
    // DPT 238.* - Scene Config
    if (datatype.mainGroup == 238 && datatype.subGroup == 1 && datatype.index <= 2)
        return valueToBusValueSceneConfig(value, payload, payload_length, datatype);
    // DPT 239.* - Flagged Scaling
    if (datatype.mainGroup == 239 && datatype.subGroup == 1 && datatype.index <= 1)
        return valueToBusValueFlaggedScaling(value, payload, payload_length, datatype);
    return false;
}

int busValueToBinary(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(1);
    KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 7));
}

int busValueToBinaryControl(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value)
{
    ASSERT_PAYLOAD(1);
    switch (datatype.index) {
        case 0: KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 6));
        case 1: KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 7));
    }

    return false;
}

int busValueToStepControl(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(1);
    switch (datatype.index) {
        case 0: KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 4));
        case 1: {
            const unsigned char stepCode = unsigned8FromPayload(payload, 0) & 0x07;
            KNX_ASSUME_VALUE_RET(stepCode);
        }
    }

    return false;
}
int busValueToCharacter(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(1);
    int8_t charValue = signed8FromPayload(payload, 0);
    if (datatype.subGroup == 1 && (charValue & 0x80))
        return false;
    if(datatype.subGroup == 2) {
        KNX_ASSUME_CHAR_VALUE_RET((uint8_t)charValue);
    }

    KNX_ASSUME_CHAR_VALUE_RET(charValue);
}

int busValueToUnsigned8(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(1);
    switch (datatype.subGroup) {
        case 1: KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, 0) * 100.0 / 255.0);
        case 3: KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, 0) * 360.0 / 255.0);
        case 6: {
            uint8_t numValue = unsigned8FromPayload(payload, 0);
            if (numValue == 0xFF)
                return false;
            KNX_ASSUME_VALUE_RET(numValue);
        }
    }

    KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, 0));
}



int busValueToSigned8(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(1);
    KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, 0));
}

int busValueToStatusAndMode(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(1);
    if (datatype.index < 5) {
        KNX_ASSUME_VALUE_RET(bitFromPayload(payload, datatype.index));
    }
    else if (datatype.index == 5) {
        KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, 0) & 0x07);
    }
    return false;
}

int busValueToUnsigned16(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(2);
    KNX_ASSUME_VALUE_RET(unsigned16FromPayload(payload, 0));
}

int busValueToTimePeriod(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(2);

    int64_t duration = unsigned16FromPayload(payload, 0);
    KNX_ASSUME_VALUE_RET(duration);
}

int busValueToSigned16(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(2);
    if (datatype.subGroup == 10) {
        KNX_ASSUME_VALUE_RET(signed16FromPayload(payload, 0) / 100.0);
    }
    KNX_ASSUME_VALUE_RET(signed16FromPayload(payload, 0));
}

int busValueToTimeDelta(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(2);

    int64_t duration = signed16FromPayload(payload, 0);
    KNX_ASSUME_VALUE_RET(duration);
}

int busValueToFloat16(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(2);
    if (unsigned16FromPayload(payload, 0) == 0x7FFF)
        return false;

    KNX_ASSUME_VALUE_RET(float16FromPayload(payload, 0));
}

int busValueToTime(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(3);
    switch (datatype.index) {
        case 0: KNX_ASSUME_VALUE_RET((unsigned8FromPayload(payload, 0) >> 5) & 0x07);
        case 1: {
            unsigned char hours = unsigned8FromPayload(payload, 0) & 0x1F;
            unsigned char minutes = unsigned8FromPayload(payload, 1) & 0x3F;
            unsigned char seconds = unsigned8FromPayload(payload, 2) & 0x3F;

            if (hours > 23 || minutes > 59 || seconds > 59)
                return false;
            bzero(&value->tValue, sizeof(struct tm));
            value->tValue.tm_hour = hours;
            value->tValue.tm_min = minutes;
            value->tValue.tm_sec = seconds;
            return true;
        }
    }

    return false;
}

int busValueToDate(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(3);
    unsigned short year = unsigned8FromPayload(payload, 2) & 0x7F;
    unsigned char month = unsigned8FromPayload(payload, 1) & 0x0F;
    unsigned char day = unsigned8FromPayload(payload, 0) & 0x1F;

    if (year > 99 || month < 1 || month > 12 || day < 1)
        return false;

    bzero(&value->tValue, sizeof(struct tm));
    year += year >= 90 ? 1900 : 2000;
    value->tValue.tm_mday = day;
    value->tValue.tm_year = year;
    value->tValue.tm_mon = month;
    return true;
}

int busValueToUnsigned32(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(4);
    KNX_ASSUME_VALUE_RET(unsigned32FromPayload(payload, 0));
}

int busValueToSigned32(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(4);
    KNX_ASSUME_VALUE_RET(signed32FromPayload(payload, 0));
}

int busValueToLongTimePeriod(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(4);
    KNX_ASSUME_VALUE_RET(signed32FromPayload(payload, 0));
}

int busValueToFloat32(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(4);
    KNX_ASSUME_VALUE_RET(float32FromPayload(payload, 0));
}

int busValueToAccess(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(4);
    switch (datatype.index) {
        case 0: {
            int digits = 0;
            for (int n = 0, factor = 100000; n < 6; ++n, factor /= 10) {
                unsigned char digit = bcdFromPayload(payload, n);
                if (digit > 9)
                    return false;
                digits += digit * factor;
            }
            KNX_ASSUME_VALUE_RET(digits);
        }
        case 1:
        case 2:
        case 3:
        case 4: KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 23 + datatype.index));
        case 5: KNX_ASSUME_VALUE_RET(bcdFromPayload(payload, 7));
    }

    return false;
}

int busValueToString(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(14);
    char strValue[15];
    strValue[14] = '\0';
    for (int n = 0; n < 14; ++n) {
        strValue[n] = signed8FromPayload(payload, n);
        if (!datatype.subGroup && (strValue[n] & 0x80))
            return false;
    }
    KNX_ASSUME_STR_VALUE_RET(value, strValue);
}

int busValueToScene(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(1);
    return KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, 0) & 0x3F);
}

int busValueToSceneControl(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(1);
    switch (datatype.index) {
        case 0: KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 0));
        case 1: KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, 0) & 0x3F);
    }

    return false;
}

int busValueToSceneInfo(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(1);
    switch (datatype.index) {
        case 0: KNX_ASSUME_VALUE(bitFromPayload(payload, 1));
        case 1: KNX_ASSUME_VALUE(unsigned8FromPayload(payload, 0) & 0x3F);
    }

    return false;
}

int busValueToSceneConfig(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(1);
    switch (datatype.index) {
        case 0: KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, 0) & 0x3F);
        case 1:
        case 2: KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 2 - datatype.index));
    }

    return false;
}

int busValueToDateTime(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(8);
    if (datatype.index == 3)
        KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 48));

    if (!bitFromPayload(payload, 48)) {
        switch (datatype.index) {
            case 0: {
                if (bitFromPayload(payload, 51) || bitFromPayload(payload, 52))
                    return false;

                unsigned short year = unsigned8FromPayload(payload, 0) + 1900;
                unsigned short month = unsigned8FromPayload(payload, 1) & 0x0F;
                unsigned short day = unsigned8FromPayload(payload, 2) & 0x1F;
                unsigned short hours = unsigned8FromPayload(payload, 3) & 0x1F;
                unsigned short minutes = unsigned8FromPayload(payload, 4) & 0x3F;
                unsigned short seconds = unsigned8FromPayload(payload, 5) & 0x3F;

                if ((month < 1 || month > 12 || day < 1))
                    return false;
                if ((hours > 24 || minutes > 59 || seconds > 59))
                    return false;

                bzero(&value->tValue, sizeof(struct tm));
                value->tValue.tm_sec = seconds;
                value->tValue.tm_min = minutes;
                value->tValue.tm_hour = hours;
                value->tValue.tm_mday = day;
                value->tValue.tm_mon = month;
                value->tValue.tm_year = year;

                return true;
            }
            case 1: {
                if (bitFromPayload(payload, 53))
                    return false;

                KNX_ASSUME_VALUE_RET((unsigned8FromPayload(payload, 3) >> 5) & 0x07);
            }
            case 2: {
                if (bitFromPayload(payload, 50))
                    return false;

                KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 49));
            }
            case 9: KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 55));
            case 10: KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 56));
        }
    }

    return false;
}

int busValueToUnicode(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    //TODO
}

int busValueToSigned64(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(8);
    KNX_ASSUME_VALUE_RET(signed64FromPayload(payload, 0));
}

int busValueToAlarmInfo(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(6);
    switch (datatype.index) {
        case 1: {
            unsigned char prio = unsigned8FromPayload(payload, 1);
            if (prio > 3)
                return false;
            KNX_ASSUME_VALUE_RET(prio);
        }
        case 0:
        case 2:
        case 3: KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, datatype.index));
        case 4:
        case 5:
        case 6:
        case 7: KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 43 - datatype.index));
        case 8:
        case 9:
        case 10: KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 55 - datatype.index));
    }
    return false;
}

int busValueToSerialNumber(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(6);
    switch (datatype.index) {
        case 0: KNX_ASSUME_VALUE_RET(unsigned16FromPayload(payload, 0));
        case 1: KNX_ASSUME_VALUE_RET(unsigned32FromPayload(payload, 2));
    }
    return false;
}

int busValueToVersion(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(2);
    switch (datatype.index) {
        case 0: KNX_ASSUME_VALUE_RET((unsigned8FromPayload(payload, 0) >> 3) & 0x1F);
        case 1: KNX_ASSUME_VALUE_RET((unsigned16FromPayload(payload, 0) >> 6) & 0x1F);
        case 2: KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, 1) & 0x3F);
    }

    return false;
}

int busValueToScaling(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(3);
    switch (datatype.index) {
        case 0: KNX_ASSUME_VALUE_RET(unsigned16FromPayload(payload, 0));
        case 1: KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, 2) * 100.0 / 255.0);
    }

    return false;
}

int busValueToTariff(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(3);
    switch (datatype.index) {
        case 0: KNX_ASSUME_VALUE_RET(unsigned16FromPayload(payload, 0));
        case 1: {
            uint8_t tariff = unsigned8FromPayload(payload, 2);
            if (tariff > 254)
                return false;
            KNX_ASSUME_VALUE_RET(tariff);
        }
    }

    return false;
}

int busValueToLocale(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(datatype.mainGroup == 231 ? 4 : 2);
    if (!datatype.index || (datatype.mainGroup == 231 && datatype.index == 1)) {
        char code[2];
        code[0] = unsigned8FromPayload(payload, datatype.index * 2);
        code[1] = unsigned8FromPayload(payload, datatype.index * 2 + 1);
        KNX_ASSUME_STR_VALUE_RET(value, code);
    }
    return false;
}

int busValueToRGB(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(3);
    uint32_t rgb = unsigned16FromPayload(payload, 0) * 256 + unsigned8FromPayload(payload, 2);
    if (rgb > 16777215)
        return false;
    return KNX_ASSUME_VALUE_RET(rgb);
}

int busValueToFlaggedScaling(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(2);
    switch (datatype.index) {
        case 0: KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, 0) * 100.0 / 255.0);
        case 1: KNX_ASSUME_VALUE_RET(bitFromPayload(payload, 15));
    }
    return false;
}

int busValueToActiveEnergy(const uint8_t *payload, int payload_length, KNXDatatype datatype, KNXValue *value) {
    ASSERT_PAYLOAD(6);
    switch (datatype.index) {
        case 0: KNX_ASSUME_VALUE_RET(signed32FromPayload(payload, 0));
        case 1: KNX_ASSUME_VALUE_RET(unsigned8FromPayload(payload, 4));
        case 2:
        case 3: KNX_ASSUME_VALUE_RET(bitFromPayload(payload, datatype.index + 44));
    }

    return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------

int valueToBusValueBinary(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    bitToPayload(payload, payload_length, 7, value->bValue);
    return true;
}

int valueToBusValueBinaryControl(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 0: bitToPayload(payload, payload_length, 6, value->bValue); break;
        case 1: bitToPayload(payload, payload_length, 7, value->bValue); break;
        default: return false;
    }

    return true;
}

int valueToBusValueStepControl(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 0: bitToPayload(payload, payload_length, 4, value->bValue); break;
        case 1: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(7))
                return false;
            unsigned8ToPayload(payload, payload_length, 0, value->uiValue, 0x07);
        }
            break;
        default: return false;
    }

    return true;
}

int valueToBusValueCharacter(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    if (value->uiValue < INT64_C(0) || value->uiValue > INT64_C(255) || (datatype.subGroup == 1 && value->uiValue > INT64_C(127)))
        return false;
    unsigned8ToPayload(payload, payload_length, 0, value->uiValue, 0xFF);
    return true;
}

int valueToBusValueUnsigned8(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    if ((int64_t)value->uiValue < INT64_C(0))
        return false;

    switch (datatype.subGroup) {
        case 1: {
            if (value->dValue > 100.0)
                return false;
            unsigned8ToPayload(payload, payload_length, 0, round(value->dValue * 255.0 / 100.0), 0xFF);
            break;
        }
        case 3: {
            if (value->dValue > 360.0)
                return false;
            unsigned8ToPayload(payload, payload_length, 0, round(value->dValue * 255.0 / 360.0), 0xFF);
            break;
        }
        case 6: {
            if ((int64_t)value->uiValue > INT64_C(254))
                return false;
            unsigned8ToPayload(payload,payload_length, 0, value->uiValue, 0xFF);
            break;
        }
        default: {
            if ((int64_t)value->uiValue > INT64_C(255))
                return false;
            unsigned8ToPayload(payload, payload_length, 0, value->uiValue, 0xFF);
        }
    }

    return true;
}

int valueToBusValueSigned8(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    if ((int64_t)value->uiValue < INT64_C(-128) || (int64_t)value->uiValue > INT64_C(127))
        return false;

    signed8ToPayload(payload, 0, payload_length, value->uiValue, 0xFF);
    return true;
}

int valueToBusValueStatusAndMode(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    if (datatype.index < 5)
        bitToPayload(payload, payload_length, datatype.index, value->bValue);
    else if (datatype.index == 5) {
        if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(7))
            return false;
        unsigned8ToPayload(payload, payload_length, 0, value->uiValue, 0x07);
    }
    else return false;

    return true;
}

int valueToBusValueUnsigned16(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(65535))
        return false;

    unsigned16ToPayload(payload, payload_length, 0, value->uiValue, 0xFFFF);
    return true;
}

int valueToBusValueTimePeriod(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    time_t timeSinceEpoch = mktime(&value->tValue);

    if (timeSinceEpoch < INT64_C(0) || timeSinceEpoch > INT64_C(65535))
        return false;

    unsigned16ToPayload(payload, payload_length, 0, timeSinceEpoch, 0xFFFF);
    return true;
}

int valueToBusValueSigned16(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    if ((int64_t)value->uiValue < INT64_C(-32768) || (int64_t)value->uiValue > INT64_C(32767))
        return false;

    if (datatype.subGroup == 10) {
        if (value->dValue < -327.68 || value->dValue > 327.67)
            return false;
        signed16ToPayload(payload, payload_length, 0, value->dValue * 100.0, 0xFF);
    }
    else signed16ToPayload(payload, payload_length, 0, value->uiValue, 0xffff);

    return true;
}

int valueToBusValueTimeDelta(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    time_t timeSinceEpoch = mktime(&value->tValue);

    if (timeSinceEpoch < INT64_C(-32768) || timeSinceEpoch > INT64_C(32767))
        return false;

    signed16ToPayload(payload, payload_length, 0, timeSinceEpoch, 0xFFFF);
    return true;
}

int valueToBusValueFloat16(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    double numValue = value->bValue;
    switch (datatype.subGroup) {
        case 1: if (numValue < -273.0 || numValue > 670760.0) return false; break;
        case 4:
        case 7:
        case 8: if (numValue < 0.0 || numValue > 670760.0) return false; break;
        case 5: {
            if (numValue < 0.0 || numValue > 670760.0)
                return false;
            break;
        }
        case 6: {
            if (numValue < 0.0 || numValue > 670760.0)
                return false;
            break;
        }
        case 10: {
            if (numValue < -670760.0 || numValue > 670760.0)
                return false;
            break;
        }
        case 11: {
            if (numValue < -670760.0 || numValue > 670760.0)
                return false;
            break;
        }
        case 20: {
            if (numValue < -670760.0 || numValue > 670760.0)
                return false;
            break;
        }
        case 21: {
            if (numValue < -670760.0 || numValue > 670760.0)
                return false;
            break;
        }
        case 2:
        case 3:
        case 22:
        case 23:
        case 24: if (numValue < -670760.0 || numValue > 670760.0) return false; break;
        case 25: {
            if (numValue < -670760.0 || numValue > 670760.0)
                return false;
            break;
        }
        case 27: {
            if (numValue < -459.6)
                return false;
            break;
        }
        case 28: if (numValue < 0.0) return false; break;
    }

    if (numValue < -671088.64 || numValue > 670760.96)
        return false;

    float16ToPayload(payload, payload_length, 0, numValue, 0xFFFF);
    return true;
}

int valueToBusValueTime(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 0: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(7))
                return false;
            ENSURE_PAYLOAD(3);
            unsigned8ToPayload(payload, payload_length, 0, value->uiValue << 5, 0xE0);
            break;
        }
        case 1: {
            unsigned8ToPayload(payload, payload_length, 0, value->tValue.tm_hour, 0x1F);
            unsigned8ToPayload(payload, payload_length, 1, value->tValue.tm_min, 0x3F);
            unsigned8ToPayload(payload, payload_length, 2, value->tValue.tm_sec, 0x3F);
            break;
        }
        default: return false;
    }

    return true;
}

int valueToBusValueDate(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    if (value->tValue.tm_year < 1990 || value->tValue.tm_year > 2089)
        return false;

    unsigned8ToPayload(payload, payload_length, 0, value->tValue.tm_mday , 0x1F);
    unsigned8ToPayload(payload, payload_length, 1, value->tValue.tm_mon , 0x0F);
    unsigned8ToPayload(payload, payload_length, 2, value->tValue.tm_year  % 100, 0x7F);
    return true;
}

int valueToBusValueUnsigned32(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(4294967295))
        return false;

    unsigned32ToPayload(payload, payload_length, 0, value->uiValue, 0xFFFFFF);
    return true;
}

int valueToBusValueSigned32(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    if ((int64_t)value->uiValue < INT64_C(-2147483648) || (int64_t)value->uiValue > INT64_C(2147483647))
        return false;
    signed32ToPayload(payload, payload_length, 0, value->uiValue, 0xFFFFFF);
    return true;
}

int valueToBusValueLongTimePeriod(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    if ((int64_t)value->uiValue < INT64_C(-2147483648) || (int64_t)value->uiValue > INT64_C(2147483647))
        return false;

    signed32ToPayload(payload, payload_length, 0, value->uiValue, 0xFFFFFF);
    return true;
}

int valueToBusValueFloat32(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    double numValue = value->dValue;
    if (numValue < (-8388608.0 * pow(2, 255)) || numValue > (8388607.0 * pow(2, 255)))
        return false;

    float32ToPayload(payload, payload_length, 0, numValue, 0xFFFFFF);
    return true;
}

int valueToBusValueAccess(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 0: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(999999))
                return false;
            ENSURE_PAYLOAD(4);
            for (int n = 0, factor = 100000; n < 6; ++n, factor /= 10)
                bcdToPayload(payload, payload_length, n, (value->uiValue / factor) % 10);
            break;
        }
        case 1:
        case 2:
        case 3:
        case 4: bitToPayload(payload, payload_length, 23 + datatype.index, value->bValue); break;
        case 5: {
            if (value->uiValue < INT64_C(0) || value->uiValue > INT64_C(15))
                return false;
            bcdToPayload(payload, payload_length, 7, value->uiValue);
            break;
        }
        default: return false;
    }

    return true;
}

int valueToBusValueString(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    //TODO
}

int valueToBusValueScene(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(63))
        return false;

    unsigned8ToPayload(payload, payload_length, 0, value->uiValue, 0xFF);
    return true;
}

int valueToBusValueSceneControl(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 0: bitToPayload(payload, payload_length, 0, value->bValue); break;
        case 1: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(63))
                return false;
            unsigned8ToPayload(payload, payload_length, 0, (int64_t)value->uiValue, 0x3F);
            break;
        }
        default: return false;
    }

    return true;
}

int valueToBusValueSceneInfo(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype)  {
    switch (datatype.index) {
        case 0: bitToPayload(payload, payload_length, 1, value->bValue); break;
        case 1: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(63))
                return false;
            unsigned8ToPayload(payload, payload_length, 0, (int64_t)value->uiValue, 0x3F);
            break;
        }
        default: return false;
    }

    return true;
}

int valueToBusValueSceneConfig(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 0: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(63))
                return false;
            unsigned8ToPayload(payload, payload_length, 0, (int64_t)value->uiValue, 0x3F);
            break;
        }
        case 1:
        case 2: bitToPayload(payload, payload_length, 2 - datatype.index, value->bValue); break;
        default: return false;
    }

    return true;
}

int valueToBusValueDateTime(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    //TODO
    return false;
}

int valueToBusValueUnicode(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    //TODO
    return false;
}

int valueToBusValueSigned64(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    signed64ToPayload(payload, payload_length, 0, (int64_t)value->uiValue, UINT64_C(0xFFFFFFFFFFFFFFFF));
    return true;
}

int valueToBusValueAlarmInfo(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 1: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(3))
                return false;
            ENSURE_PAYLOAD(6);
            unsigned8ToPayload(payload, payload_length, 1, (int64_t)value->uiValue, 0xFF);
            break;
        }
        case 0:
        case 2:
        case 3: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(255))
                return false;
            ENSURE_PAYLOAD(6);
            unsigned8ToPayload(payload, payload_length, datatype.index, (int64_t)value->uiValue, 0xFF);
            break;
        }
        case 4:
        case 5:
        case 6:
        case 7: {
            ENSURE_PAYLOAD(6);
            bitToPayload(payload, payload_length, 43 - datatype.index, value->bValue);
            break;
        }
        case 8:
        case 9:
        case 10: {
            ENSURE_PAYLOAD(6);
            bitToPayload(payload, payload_length, 55 - datatype.index, value->bValue);
            break;
        }
        default: return false;
    }

    return true;
}

int valueToBusValueSerialNumber(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 0: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(65535))
                return false;
            ENSURE_PAYLOAD(6);
            unsigned16ToPayload(payload, payload_length, 0, (int64_t)value->uiValue, 0xFFFF);
            break;
        }
        case 1: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(4294967295))
                return false;
            ENSURE_PAYLOAD(6);
            unsigned32ToPayload(payload, payload_length, 2, (int64_t)value->uiValue, 0xFFFF);
            break;
        }
        default: return false;
    }

    return true;
}

int valueToBusValueVersion(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 0: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(31))
                return false;
            ENSURE_PAYLOAD(2);
            unsigned8ToPayload(payload, payload_length, 0, (int64_t)value->uiValue << 3, 0xF8);
            break;
        }
        case 1: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(31))
                return false;
            unsigned16ToPayload(payload, payload_length, 0, (int64_t)value->uiValue << 6, 0x07C0);
            break;
        }
        case 2: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(63))
                return false;
            unsigned8ToPayload(payload, payload_length, 1, (int64_t)value->uiValue, 0x3F);
            break;
        }
        default: return false;
    }

    return true;
}

int valueToBusValueScaling(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 0: {
            time_t duration = mktime(&value->tValue);

            if (duration < INT64_C(0) || duration > INT64_C(65535))
                return false;

            ENSURE_PAYLOAD(3);
            unsigned16ToPayload(payload, payload_length, 0, duration, 0xFFFF);
            return true;
        }
        case 1: {
            if (value->dValue < 0.0 || value->dValue > 100.0)
                return false;
            unsigned8ToPayload(payload, payload_length, 2, round(value->dValue * 255.0 / 100.0), 0xff);
            break;
        }
    }

    return true;
}

int valueToBusValueTariff(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 0: {
            time_t duration = mktime(&value->tValue);

            if (duration < INT64_C(0) || duration > INT64_C(65535))
                return false;

            ENSURE_PAYLOAD(3);
            unsigned16ToPayload(payload, payload_length, 0, duration, 0xFFFF);
            return true;
        }
        case 1: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(254))
                return false;
            unsigned8ToPayload(payload, payload_length, 2, (int64_t)value->uiValue, 0xff);
            break;
        }
    }

    return true;
}

int valueToBusValueLocale(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    int strl = strlen(value->strValue);
    if(strl != 2)
        return false;

    if (!datatype.index || (datatype.mainGroup == 231 && datatype.index == 1)) {
        ENSURE_PAYLOAD(datatype.mainGroup == 231 ? 4 : 2);
        unsigned8ToPayload(payload, payload_length, datatype.index * 2, value->strValue[0], 0xff);
        unsigned8ToPayload(payload, payload_length, datatype.index * 2 + 1, value->strValue[1], 0xff);
        return true;
    }

    return false;
}

int valueToBusValueRGB(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(16777215))
        return false;

    unsigned int rgb = (int64_t)value->uiValue;

    unsigned16ToPayload(payload, payload_length, 0, rgb / 256, 0xffff);
    unsigned8ToPayload(payload, payload_length, 2, rgb % 256, 0xff);
    return true;
}

int valueToBusValueFlaggedScaling(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 0: {
            if (value->dValue < 0.0 || value->dValue > 100.0)
                return false;
            ENSURE_PAYLOAD(2);
            unsigned8ToPayload(payload, payload_length, 0, round(value->dValue * 255.0 / 100.0), 0xff);
            break;
        }
        case 1: bitToPayload(payload, payload_length, 15, value->bValue); break;
        default: return false;
    }

    return true;
}

int valueToBusValueActiveEnergy(KNXValue *value, uint8_t *payload, int payload_length, KNXDatatype datatype) {
    switch (datatype.index) {
        case 0: {

            if ((int64_t)value->uiValue < INT64_C(-2147483648) || (int64_t)value->uiValue > INT64_C(2147483647))
                return false;
            ENSURE_PAYLOAD(6);
            signed32ToPayload(payload, payload_length, 0, (int64_t)value->uiValue, 0xffffff);
            break;
        }
        case 1: {
            if ((int64_t)value->uiValue < INT64_C(0) || (int64_t)value->uiValue > INT64_C(254))
                return false;
            ENSURE_PAYLOAD(6);
            unsigned8ToPayload(payload, payload_length, 4, (int64_t)value->uiValue, 0xff);
            break;
        }
        case 2:
        case 3: bitToPayload(payload, payload_length, datatype.index + 44, value->bValue); break;
        default: return false;
    }

    return true;
}


// Helper functions
int bitFromPayload(const const uint8_t *payload, int index) {
    int bit = payload[index / 8] & (1 << (7 - (index % 8)));
    return bit ? 1 : 0;
}
uint8_t unsigned8FromPayload(const uint8_t *payload, int index) {
    return (uint8_t)payload[index];
}
int8_t signed8FromPayload(const uint8_t *payload, int index) {
    return (int8_t)payload[index];
}
uint16_t unsigned16FromPayload(const uint8_t *payload, int index) {
    return ((((uint16_t)payload[index]) << 8) & 0xFF00) | (((uint16_t)payload[index + 1]) & 0x00FF);
}
int16_t signed16FromPayload(const uint8_t *payload, int index) {
    return ((((uint16_t)payload[index]) << 8) & 0xFF00) | (((uint16_t)payload[index + 1]) & 0x00FF);
}
uint32_t unsigned32FromPayload(const uint8_t *payload, int index) {
    return ((((uint32_t)payload[index]) << 24) & 0xFF000000) |
            ((((uint32_t)payload[index + 1]) << 16) & 0x00FF0000) |
            ((((uint32_t)payload[index + 2]) << 8) & 0x0000FF00) |
            (((uint32_t)payload[index + 3]) & 0x000000FF);
}
int32_t signed32FromPayload(const uint8_t *payload, int index) {
    return (int32_t)unsigned32FromPayload(payload, index);
}
double float16FromPayload(const uint8_t *payload, int index) {
    uint16_t mantissa = unsigned16FromPayload(payload, index) & 0x87FF;
    if (mantissa & 0x8000)
        return ((~mantissa & 0x07FF) + 1.0) * -0.01 * (1 << ((payload[index] >> 3) & 0x0F));

    return mantissa * 0.01 * (1 << ((payload[index] >> 3) & 0x0F));
}
double float32FromPayload(const uint8_t *payload, int index) {
    uint32_t area = unsigned32FromPayload(payload, index);
    return (double)area;
}
int64_t signed64FromPayload(const uint8_t *payload, int index) {
    return ((((uint64_t)payload[index]) << 56) & UINT64_C(0xFF00000000000000)) |
            ((((uint64_t)payload[index + 1]) << 48) & UINT64_C(0x00FF000000000000)) |
            ((((uint64_t)payload[index + 2]) << 40) & UINT64_C(0x0000FF0000000000)) |
            ((((uint64_t)payload[index + 3]) << 32) & UINT64_C(0x000000FF00000000)) |
            ((((uint64_t)payload[index + 4]) << 24) & UINT64_C(0x00000000FF000000)) |
            ((((uint64_t)payload[index + 5]) << 16) & UINT64_C(0x0000000000FF0000)) |
            ((((uint64_t)payload[index + 6]) << 8) & UINT64_C(0x000000000000FF00)) |
            (((uint64_t)payload[index + 7]) & UINT64_C(0x00000000000000FF));
}
uint8_t bcdFromPayload(const uint8_t *payload, int index) {
    if (index % 2)
        return (uint8_t)(payload[index / 2] & 0x0F);
    return (uint8_t)((payload[index / 2] >> 4) & 0x0F);
}

void bitToPayload(uint8_t *payload, int payload_length, int index, int value) {
    ENSURE_PAYLOAD(index / 8 +1);
    payload[index / 8] = (payload[index / 8] & ~(1 << (7 - (index % 8)))) | (value ? (1 << (7 - (index % 8))) : 0);
}
void unsigned8ToPayload(uint8_t *payload, int payload_length, int index, uint8_t value, uint8_t mask) {
    ENSURE_PAYLOAD(index + 1);
    payload[index] = (payload[index] & ~mask) | (value & mask);
}
void signed8ToPayload(uint8_t *payload, int payload_length, int index, int8_t value, uint8_t mask) {
    ENSURE_PAYLOAD(index + 1);
    payload[index] = (payload[index] & ~mask) | (value & mask);
}
void unsigned16ToPayload(uint8_t *payload, int payload_length, int index, uint16_t value, uint16_t mask) {
    ENSURE_PAYLOAD(index + 2);
    payload[index] = (payload[index] & (~mask >> 8)) | ((value >> 8) & (mask >> 8));
    payload[index + 1] = (payload[index + 1] & ~mask) | (value & mask);
}
void signed16ToPayload(uint8_t *payload, int payload_length, int index, int16_t value, uint16_t mask) {
    ENSURE_PAYLOAD(index + 2);
    payload[index] = (payload[index] & (~mask >> 8)) | ((value >> 8) & (mask >> 8));
    payload[index + 1] = (payload[index + 1] & ~mask) | (value & mask);
}
void unsigned32ToPayload(uint8_t *payload, int payload_length, int index, uint32_t value, uint32_t mask) {
    ENSURE_PAYLOAD(index + 4);
    payload[index] = (payload[index] & (~mask >> 24)) | ((value >> 24) & (mask >> 24));
    payload[index + 1] = (payload[index + 1] & (~mask >> 16)) | ((value >> 16) & (mask >> 16));
    payload[index + 2] = (payload[index + 2] & (~mask >> 8)) | ((value >> 8) & (mask >> 8));
    payload[index + 3] = (payload[index + 3] & ~mask) | (value & mask);
}
void signed32ToPayload(uint8_t *payload, int payload_length, int index, int32_t value, uint32_t mask) {
    ENSURE_PAYLOAD(index + 4);
    payload[index] = (payload[index] & (~mask >> 24)) | ((value >> 24) & (mask >> 24));
    payload[index + 1] = (payload[index + 1] & (~mask >> 16)) | ((value >> 16) & (mask >> 16));
    payload[index + 2] = (payload[index + 2] & (~mask >> 8)) | ((value >> 8) & (mask >> 8));
    payload[index + 3] = (payload[index + 3] & ~mask) | (value & mask);
}

void float16ToPayload(uint8_t *payload, int payload_length, int index, double value, uint16_t mask) {
    value *= 100.0;
    unsigned short exponent = ceil(log2(value) - 10.0);
    short mantissa = roundf(value / (1 << exponent));
    signed16ToPayload(payload, payload_length, index, mantissa, mask);
    unsigned8ToPayload(payload, payload_length, index, exponent << 3, 0x78 & (mask >> 8));
}
void float32ToPayload(uint8_t *payload, int payload_length, int index, double value, uint32_t mask) {
    float num = value;
    unsigned32ToPayload(payload, payload_length, index, (unsigned int)num, mask);
}
void signed64ToPayload(uint8_t *payload, int payload_length, int index, int64_t value, uint64_t mask) {
    ENSURE_PAYLOAD(index + 8);
    payload[index] = (payload[index] & (~mask >> 56)) | ((value >> 56) & (mask >> 56));
    payload[index + 1] = (payload[index + 1] & (~mask >> 48)) | ((value >> 48) & (mask >> 48));
    payload[index + 2] = (payload[index + 2] & (~mask >> 40)) | ((value >> 40) & (mask >> 40));
    payload[index + 3] = (payload[index + 3] & (~mask >> 32)) | ((value >> 32) & (mask >> 32));
    payload[index + 4] = (payload[index + 4] & (~mask >> 24)) | ((value >> 24) & (mask >> 24));
    payload[index + 5] = (payload[index + 5] & (~mask >> 16)) | ((value >> 16) & (mask >> 16));
    payload[index + 6] = (payload[index + 6] & (~mask >> 8)) | ((value >> 8) & (mask >> 8));
    payload[index + 7] = (payload[index + 7] & ~mask) | (value & mask);
}
void bcdToPayload(uint8_t *payload, int payload_length, int index, uint8_t value) {
    ENSURE_PAYLOAD(index / 2 + 1);
    if (index % 2)
        payload[index / 2] = (payload[index / 2] & 0xF0) | (value & 0x0F);
    else
        payload[index / 2] = (payload[index / 2] & 0x0F) | ((value << 4) & 0xF0);
}
