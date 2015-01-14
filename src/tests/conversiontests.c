/*
    Conversion tests
    Copyright (C) 2014 Patrik Pfaffenbauer <patrik.pfaffenbauer@p3.co.at>

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

#include "eibclient.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <time.h>

#define true 1
#define false 0

#define TEST_CASE_START int completedTests = 0;
#define TEST_CASE_END return true;

#define RUN_TEST_GROUP(t) printf("Start running TestGroup %s\n", #t); if(t) { printf("TestGroup %s run completed\n\n", #t); } else { return false; }
#define RUN_TEST_SILENT(t) if(!t) return false;
#define RUN_TEST(t) printf("Running test #%d\n", completedTests); if(!t){ printf("Test %s failed after tests #%d\n", #t, completedTests); return false; } else completedTests++; printf("\n");

#define ASSERT(x,y) if(x!=y) { printf("ASSERT failes at %s:%d FirstValue %lld SecondValue %lld\n",__FILE__, __LINE__, x, y); return false; }

#define TESTCASE_IN_DEF(x, y) int x(const char *payload, unsigned short mainGroup, unsigned short subGroup, unsigned short index, y expectedValue, int decode_ret_val)
#define TESTCASE_IN(x) \
    char data[strlen(payload)/2]; \
    int payLen = hexToByteArray(payload, strlen(payload), data);\
    printf("INCOMING: DPT=%d.%d-%d PAYLOAD=%s ", mainGroup, subGroup, index, payload);\
    KNXValue val;\
    KNXDatatype dp;\
    dp.mainGroup = mainGroup;\
    dp.subGroup = subGroup;\
    dp.index = index;\
    int retVal = KNX_Decode_Value(data, payLen, dp, &val);\
    if(retVal)\
    x;\
    else\
    printf("\n");\
    ASSERT(retVal, decode_ret_val);\
    if(!retVal)\
    return true;\

int hexToByteArray(const char *hex, int len, char *out) {
    size_t count = 0;
    int outLen = len/2;
    const char *pos = hex;
    for(count = 0; count < outLen; count++) {
        sscanf(pos, "%2hhx", &out[count]);
        pos += 2 * sizeof(char);
    }
    return count;
}

int byteArrayToHex(const char *data, int len, char *out) {
    int i = 0;
    for (i = 0; i < len; i++)
        sprintf(out+i*2,"%02X",data[i]);
    out[len*2+1] = '\0';
}

TESTCASE_IN_DEF(testcaseInDouble, double) {
    TESTCASE_IN(printf("Data=4.3%f\n", val.dValue));
    ASSERT(roundf(val.dValue*1000)/1000, roundf(expectedValue*1000)/1000);
    return true;
}

TESTCASE_IN_DEF(testcaseInS8, int8_t) {
    TESTCASE_IN(printf("Data=%lld\n", val.uiValue));
    int8_t signedValue = val.uiValue;
    ASSERT(signedValue, (int8_t)expectedValue);
    return true;
}

TESTCASE_IN_DEF(testcaseInU8, uint8_t) {
    TESTCASE_IN(printf("Data=%lld\n", val.uiValue));
    uint8_t signedValue = val.uiValue;
    ASSERT(signedValue, (uint8_t)expectedValue);
    return true;
}

TESTCASE_IN_DEF(testcaseInS16, int16_t) {
    TESTCASE_IN(printf("Data=%lld\n", val.uiValue));
    int16_t signedValue = val.uiValue;
    ASSERT(signedValue, (int16_t)expectedValue);
    return true;
}

TESTCASE_IN_DEF(testcaseInU16, uint16_t) {
    TESTCASE_IN(printf("Data=%lld\n", val.uiValue));
    uint16_t signedValue = val.uiValue;
    ASSERT(signedValue, (uint16_t)expectedValue);
    return true;
}

TESTCASE_IN_DEF(testcaseInU32, uint32_t) {
    TESTCASE_IN(printf("Data=%lld\n", val.uiValue));
    uint32_t signedValue = val.uiValue;
    ASSERT(signedValue, (uint32_t)expectedValue);
    return true;
}

TESTCASE_IN_DEF(testcaseInS32, int32_t) {
    TESTCASE_IN(printf("Data=%lld\n", val.uiValue));
    int32_t signedValue = val.uiValue;
    ASSERT(signedValue, (int32_t)expectedValue);
    return true;
}

TESTCASE_IN_DEF(testcaseInS64, int64_t) {
    TESTCASE_IN(printf("Data=%lld\n", val.uiValue));
    int64_t signedValue = (int64_t)val.uiValue;
    ASSERT(signedValue, (int64_t)expectedValue);
    return true;
}

TESTCASE_IN_DEF(testcaseInU64, uint64_t) {
    TESTCASE_IN(printf("Data=%lld\n", val.uiValue));
    ASSERT(val.uiValue, expectedValue);
    return true;
}

TESTCASE_IN_DEF(testcaseInStr, const char *) {
    TESTCASE_IN(printf("Data=%s\n", val.strValue));
    int res = strncmp(val.strValue, expectedValue, strlen(expectedValue));

    ASSERT(res, 0);
    return true;
}

TESTCASE_IN_DEF(testcaseInTime, struct tm) {
    TESTCASE_IN(printf("Hour: %d  Min: %d  Sec: %d\n", val.tValue.tm_hour, val.tValue.tm_min, val.tValue.tm_sec));

    ASSERT(mktime(&val.tValue), mktime(&expectedValue));
    return true;

}

int testcaseOut(KNXValue value, unsigned short mainGroup, unsigned short subGroup, unsigned short index, const char *expectedResult, int expectedLen, int encode_ret_val) {
    printf("OUTGOING: DPT=%d.%d-%d Value=%lld ", mainGroup, subGroup, index, value.uiValue);
    unsigned char data[expectedLen];
    bzero(data, expectedLen);
    char hexOut[expectedLen*2+1];

    char expectedResultData[expectedLen];
    hexToByteArray(expectedResult, strlen(expectedResult), expectedResultData);

    KNXDatatype dp;
    dp.mainGroup = mainGroup;
    dp.subGroup = subGroup;
    dp.index = index;
    int retVal = KNX_Encode_Value(&value, data, expectedLen, dp);

    if(retVal) {
        byteArrayToHex(data, expectedLen, hexOut);
        printf("Data=%s\n", hexOut);
    }
    else
        printf("\n");

    ASSERT(retVal, encode_ret_val);

    if(!retVal)
        return true;
    int memcmpResult = memcmp(data, expectedResultData, expectedLen) != 0;

    ASSERT(memcmpResult, 0);

    return true;
}

#define TESTCASE_IN_OUT(x, y, z, a) int x(char const *payload, unsigned short mainGroup, unsigned short subGroup, unsigned short index, y attrValue, int endecode_ret_val) { \
    RUN_TEST_SILENT(z(payload, mainGroup, subGroup, index, attrValue, endecode_ret_val)); \
    KNXValue t; \
    a(t, attrValue); \
    RUN_TEST_SILENT(testcaseOut(t, mainGroup, subGroup, index, payload, strlen(payload)/2, endecode_ret_val)); \
    } \

TESTCASE_IN_OUT(testcaseInOutU8, uint8_t, testcaseInU8, KNX_ASSUME_KNX_VALUE)
TESTCASE_IN_OUT(testcaseInOutU64, uint64_t, testcaseInU64, KNX_ASSUME_KNX_VALUE)
TESTCASE_IN_OUT(testcaseInOutS64, int64_t, testcaseInS64, KNX_ASSUME_KNX_VALUE)
TESTCASE_IN_OUT(testcaseInOutDouble, double, testcaseInDouble, KNX_ASSUME_KNX_VALUE)
TESTCASE_IN_OUT(testcaseInOutString, const char *, testcaseInStr, KNX_ASSUME_STR_VALUE)

int DPT1() {
    TEST_CASE_START;

    RUN_TEST(testcaseInOutU64("00", 1, 2, 0, 0, true));
    RUN_TEST(testcaseInOutU64("01", 1, 2, 0, 1, true));
    RUN_TEST(testcaseInOutU64("00", 1, 10, 0,  0, true));
    RUN_TEST(testcaseInOutU64("01", 1, 11, 0,  1, true));
    RUN_TEST(testcaseInOutU64("01", 1, 5, 0,  1, true));
    RUN_TEST(testcaseInOutU64("01", 1, 3, 0, 1, true));
    RUN_TEST(testcaseInOutU64("00", 1, 3, 0, 0, true));
    RUN_TEST(testcaseInU64("02", 1, 3, 0, 0, true));
    RUN_TEST(testcaseInOutU64("01", 1, 6, 0, 1, true));
    RUN_TEST(testcaseInOutU64("00", 1, 15, 0, 0, true));
    RUN_TEST(testcaseInU64("0100", 1, 1, 0, 0, false));
    RUN_TEST(testcaseInU64("", 1, 2, 0, 0, false));
    RUN_TEST(testcaseInU64("FF", 1, 1, 0,  1, true));

    TEST_CASE_END
}

int DPT2() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU64("03", 2, 2, 0, 1, true));
    RUN_TEST(testcaseInU64("03", 2, 2, 1, 1, true));
    RUN_TEST(testcaseInU64("02", 2, 2, 1, 0, true));
    RUN_TEST(testcaseInU64("01", 2, 2, 0, 0, true));
    RUN_TEST(testcaseInU64("00", 2, 11, 0, 0, true));
    RUN_TEST(testcaseInU64("03", 2, 11, 1, 1, true));
    RUN_TEST(testcaseInU64("02", 2, 11, 0, 1, true));
    RUN_TEST(testcaseInU64("03", 2, 5, 0, 1, true));
    RUN_TEST(testcaseInU64("01", 2, 3, 1, 1, true));
    RUN_TEST(testcaseInU64("01", 2, 3, 0, 0, true));
    RUN_TEST(testcaseInU64("01", 2, 6, 1, 1, true));
    RUN_TEST(testcaseInU64("00", 2, 8, 0, 0, true));
    RUN_TEST(testcaseInU64("03", 2, 3, 0, 1, true));
    RUN_TEST(testcaseInU64("02D0", 2, 1, 0, 0, false));
    RUN_TEST(testcaseInU64("", 2, 2, 1, 0, false));

    TEST_CASE_END
}

int DPT3() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU64("08", 3, 7, 0, 1, true));
    RUN_TEST(testcaseInU64("05", 3, 7, 0, 0, true));
    RUN_TEST(testcaseInU64("08", 3, 7, 1, 0, true));
    RUN_TEST(testcaseInU64("08", 3, 7, 1, 0, true));
    RUN_TEST(testcaseInU64("0B", 3, 7, 1, 3, true));
    RUN_TEST(testcaseInU64("0C", 3, 7, 1, 4, true));
    RUN_TEST(testcaseInU64("0C", 3, 7, 1, 4, true));
    RUN_TEST(testcaseInU64("07", 3, 7, 1, 7, true));
    RUN_TEST(testcaseInU64("0620A5", 3, 7, 1, 0, false));
    RUN_TEST(testcaseInU64("", 3, 7, 1, 0, false));
    RUN_TEST(testcaseInU64("00", 3, 7, 1, 0, true));
    RUN_TEST(testcaseInU64("01", 3, 7, 1, 1, true));
    RUN_TEST(testcaseInU64("02", 3, 7, 1,  2, true));
    RUN_TEST(testcaseInU64("03", 3, 7, 1,  3, true));
    RUN_TEST(testcaseInU64("FF", 3, 7, 1, 7, true));

    TEST_CASE_END
}

int DPT4() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU64("45", 4, 1, 0, 'E', true));
    RUN_TEST(testcaseInU64("45", 4, 1, 0, 69, true));
    RUN_TEST(testcaseInU64("A7", 4, 1, 0, 0, false));
    RUN_TEST(testcaseInU64("A7", 4, 2, 0, 167, true));

    RUN_TEST(testcaseInU64("A7", 4, 1, 0, 0 , false));
    RUN_TEST(testcaseInU64("80", 4, 2, 0, 0x80, true));
    RUN_TEST(testcaseInU64("40", 4, 1, 0, '@', true));
    RUN_TEST(testcaseInU64("80", 4, 1, 0, 0, false));
    RUN_TEST(testcaseInU64("81", 4, 2, 0, 0x81, true));
    RUN_TEST(testcaseInU64("50", 4, 2, 0, 80, true));
    RUN_TEST(testcaseInU64("BA", 4, 1, 0, 0, false));
    RUN_TEST(testcaseInU64("", 4, 1, 0, 0, false));
    RUN_TEST(testcaseInU64("4577F1", 4, 1, 0, 0, false));
    KNXValue v;
    v.uiValue = 0xFFFF;
    RUN_TEST(testcaseOut(v, 4, 1, 0, "", 0, false));

    TEST_CASE_END
}

int DPT5() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU64("BA", 5, 4, 0, 186, true));
    RUN_TEST(testcaseInU64("BA", 5, 1, 0, 72, true));
    RUN_TEST(testcaseInU64("BA", 5, 3, 0, 262, true));
    RUN_TEST(testcaseInDouble("BA", 5, 3, 0, 262.588, true));
    RUN_TEST(testcaseInU64("80", 5, 5, 0, 128, true));
    RUN_TEST(testcaseInU64("00", 5, 5, 0, 0, true));
    RUN_TEST(testcaseInU64("0000", 5, 5, 0, 0, false));
    RUN_TEST(testcaseInU64("11", 5, 5, 0, 17, true));
    RUN_TEST(testcaseInU64("FF", 5, 6, 0, 0, false));
    RUN_TEST(testcaseInU64("05", 5, 6, 0, 5, true));
    RUN_TEST(testcaseInU64("", 5, 4, 0, 0, false));
    RUN_TEST(testcaseInU64("BADBEEF0", 5, 3, 0, 0, false));

    TEST_CASE_END;
}

int DPT6() {
    TEST_CASE_START;

    RUN_TEST(testcaseInS8("BA", 6, 1, 0, -70, true));

    RUN_TEST(testcaseInS8("00", 6, 1, 0, 0, true));
    RUN_TEST(testcaseInS8("28", 6, 1, 0, 40, true));
    RUN_TEST(testcaseInS8("FF", 6, 1, 0, -1, true));

    RUN_TEST(testcaseInS8("FF00", 6, 1, 0, 0, false));
    RUN_TEST(testcaseInS8("80", 6, 1, 0, -128, true));
    RUN_TEST(testcaseInS8("7F", 6, 10, 0, 127, true));

    RUN_TEST(testcaseInS8("53", 6, 20, 0, 0, true));
    RUN_TEST(testcaseInS8("53", 6, 20, 1, 1, true));
    RUN_TEST(testcaseInS8("53", 6, 20, 2, 0, true));
    RUN_TEST(testcaseInS8("53", 6, 20, 3, 1, true));
    RUN_TEST(testcaseInS8("53", 6, 20, 4, 0, true));
    RUN_TEST(testcaseInS8("50", 6, 20, 5, 0, true));
    RUN_TEST(testcaseInS8("FF", 6, 20, 5, 7, true));
    RUN_TEST(testcaseInS8("ABCD", 6, 20, 2, 0, false));
    RUN_TEST(testcaseInS8("", 6, 20, 0, 0, false));
    RUN_TEST(testcaseInS8("", 6, 20, 5, 0, false));
    RUN_TEST(testcaseInS8("53", 6, 20, 5, 3, true));

    TEST_CASE_END;
}

int DPT7() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU16("FFFF", 7, 1, 0, 65535, true));
    RUN_TEST(testcaseInU16("0000", 7, 1, 0, 0, true));
    RUN_TEST(testcaseInU16("FF", 7, 1, 0, 0, false));
    RUN_TEST(testcaseInU16("BEEF", 7, 1, 0, 48879, true));
    RUN_TEST(testcaseInU16("BEEF", 7, 10, 0, 48879, true));
    RUN_TEST(testcaseInU16("0001", 7, 1, 0, 1, true));
    RUN_TEST(testcaseInU16("", 7, 12, 0, 0, false));
    RUN_TEST(testcaseInU16("1234", 7, 2, 0, 4660, true));
    RUN_TEST(testcaseInU16("12345678", 7, 7, 0, 0, false));
    RUN_TEST(testcaseInU16("12", 7, 7, 0, 0, false));
    RUN_TEST(testcaseInU16("", 7, 7, 0, 0, false));

    TEST_CASE_END;
}

int DPT8() {
    TEST_CASE_START;

    RUN_TEST(testcaseInS16("FFFF", 8, 1, 0, -1, true));
    RUN_TEST(testcaseInS16("7FFF", 8, 1, 0, 32767, true));
    RUN_TEST(testcaseInS16("8000", 8, 1, 0, -32768, true));
    RUN_TEST(testcaseInS16("0000", 8, 1, 0, 0, true));
    RUN_TEST(testcaseInS16("FF", 8, 1, 0, 0, false));
    RUN_TEST(testcaseInS16("BEEF", 8, 1, 0, -16657, true));
    RUN_TEST(testcaseInS16("", 8, 1, 0, 0, false));

    RUN_TEST(testcaseInDouble("FFFF", 8, 10, 0, -0.01, true));
    RUN_TEST(testcaseInS16("FFFF", 8, 10, 0, 0, true));
    RUN_TEST(testcaseInS16("7FFF", 8, 10, 0, 327, true));
    RUN_TEST(testcaseInDouble("7FFF", 8, 10, 0, 327.67, true));
    RUN_TEST(testcaseInS16("8000", 8, 10, 0, -327, true));
    RUN_TEST(testcaseInDouble("8000", 8, 10, 0,  -327.68, true));

    TEST_CASE_END;
}

int DPT9() {
    TEST_CASE_START;

    RUN_TEST(testcaseInDouble("39CB", 9, 1, 0, 587.52, true));
    RUN_TEST(testcaseInS16("39CB", 9, 1, 0, 587, true));
    RUN_TEST(testcaseInDouble("", 9, 1, 0, 0, false));


    RUN_TEST(testcaseInDouble("39CB", 9, 2, 0, 587.52, true));
    RUN_TEST(testcaseInDouble("39CB", 9, 2, 0, 587.52, true));
    RUN_TEST(testcaseInDouble("39CB", 9, 3, 0, 587.52, true));
    RUN_TEST(testcaseInDouble("39CB", 9, 3, 0, 587.52, true));
    RUN_TEST(testcaseInDouble("39CB", 9, 4, 0, 587.52, true));
    RUN_TEST(testcaseInDouble("39CB", 9, 5, 0, 587.52, true));
    RUN_TEST(testcaseInDouble("39CB", 9, 5, 0, 587.52, true));
    RUN_TEST(testcaseInDouble("39CB", 9, 6, 0, 587.52, true));
    RUN_TEST(testcaseInDouble("39CB", 9, 7, 0, 587.52, true));
    RUN_TEST(testcaseInU64("39CB", 9, 10, 0, 587, true));

    TEST_CASE_END;
}

int DPT10() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU64("4E2139", 10, 1, 0, 2, true));

    struct tm exp;
    bzero(&exp, sizeof(struct tm));
    exp.tm_sec = 57;
    exp.tm_hour = 14;
    exp.tm_min = 33;

    RUN_TEST(testcaseInTime("4E2139", 10, 1, 1, exp, true));
    RUN_TEST(testcaseInTime("", 10, 1, 1, exp, false));
    RUN_TEST(testcaseInTime("A8", 10, 1, 1, exp, false));

    TEST_CASE_END;
}

int DPT11() {
    TEST_CASE_START;


    struct tm exp;
    bzero(&exp, sizeof(struct tm));
    exp.tm_mday = 18;
    exp.tm_mon = 9;
    exp.tm_year = 2014;

    RUN_TEST(testcaseInTime("12090E", 11, 1, 0, exp, true));
    RUN_TEST(testcaseInTime("FFFFFF", 11, 1, 0, exp, false));

    KNXValue v;
    v.tValue = exp;
    RUN_TEST(testcaseOut(v, 11, 1, 0, "12090E", 3, true));

    exp.tm_mday = 1;
    exp.tm_mon = 1;
    exp.tm_year = 2000;

    RUN_TEST(testcaseInTime("010100", 11, 1, 0, exp, true));

    RUN_TEST(testcaseInTime("", 11, 1, 0, exp, false));
    RUN_TEST(testcaseInTime("A8", 11, 1, 0, exp, false));
    RUN_TEST(testcaseInTime("A86666AF", 11, 1, 0, exp, false));
    TEST_CASE_END;
}

int DPT12() {
    TEST_CASE_START;

    RUN_TEST(testcaseInOutU64("FFFFFFFF", 12, 1, 0, 4294967295, true));
    RUN_TEST(testcaseInU32("FFFFFFFF", 12, 1, 0, (uint32_t)4.29497e+09, true));

    KNXValue v;
    KNX_ASSUME_KNX_VALUE(v, (uint32_t)4294967295);
    RUN_TEST(testcaseOut(v, 12, 1, 0, "FFFFFFFF", 4, true));

    RUN_TEST(testcaseInOutU64("00000000", 12, 1, 0, 0, true));
    RUN_TEST(testcaseInU32("", 12, 1, 0, 0, 0));
    RUN_TEST(testcaseInU32("00", 12, 1, 0, 0, 0));
    RUN_TEST(testcaseInU32("00FF", 12, 1, 0, 0, 0));
    RUN_TEST(testcaseInU32("00FF00", 12, 1, 0, 0, 0));
    RUN_TEST(testcaseInU32("00FF00FF00", 12, 1, 0, 0, 0));

    TEST_CASE_END;
}

int DPT13() {
    TEST_CASE_START;

    RUN_TEST(testcaseInOutS64("FFFFFFFF", 13, 1, 0, -1, true));
    RUN_TEST(testcaseInOutS64("7FFFFFFF", 13, 1, 0, 2147483647, true));
    RUN_TEST(testcaseInOutS64("80000000", 13, 1, 0, -2147483648, true));
    RUN_TEST(testcaseInOutS64("00000000", 13, 1, 0, 0, true));
    RUN_TEST(testcaseInS32("", 13, 1, 0, 0, 0));
    RUN_TEST(testcaseInS32("00", 13, 1, 0, 0, 0));
    RUN_TEST(testcaseInS32("00FF", 13, 1, 0, 0, 0));
    RUN_TEST(testcaseInS32("00FF00", 13, 1, 0, 0, 0));
    RUN_TEST(testcaseInS32("00FF00FF00", 13, 1, 0, 0, 0));


    TEST_CASE_END;
}

int DPT14() {
    TEST_CASE_START;

    RUN_TEST(testcaseInS64("3F400000", 14, 1, 0, 0, true));
    RUN_TEST(testcaseInOutDouble("3F400000", 14, 0, 0, 0.75f, true));
    RUN_TEST(testcaseInOutDouble("BF400000", 14, 0, 0, -0.75f, true));
    RUN_TEST(testcaseInOutDouble("00000000", 14, 0, 0, 0, true));
    RUN_TEST(testcaseInDouble("43DB1000DEAD", 14, 0, 0, 0, false));
    RUN_TEST(testcaseInDouble("", 14, 0, 0, 0, false));
    RUN_TEST(testcaseInDouble("43", 14, 0, 0, 0, false));
    RUN_TEST(testcaseInDouble("43DE", 14, 0, 0, 0, false));
    RUN_TEST(testcaseInDouble("43DEAD", 14, 0, 0, 0, false));

    TEST_CASE_END;
}

int DPT15() {
    TEST_CASE_START;


    RUN_TEST(testcaseInDouble("123456A5", 15, 1, 0, 0, false));
    RUN_TEST(testcaseInU64("123456A5", 15, 0, 0, 123456, true));
    KNXValue v;
    KNX_ASSUME_KNX_VALUE(v, 123456);
    RUN_TEST(testcaseOut(v, 15, 0, 0, "12345600", 4, true));


    RUN_TEST(testcaseInDouble("123456A5", 15, 0, 0, 123456, true));

    RUN_TEST(testcaseInS8("123456A5", 15, 0, 1, 1, true));
    RUN_TEST(testcaseInS8("123456A5", 15, 0, 2, 0, true));
    RUN_TEST(testcaseInS8("123456A5", 15, 0, 3, 1, true));
    RUN_TEST(testcaseInS8("123456A5", 15, 0, 4, 0, true));
    RUN_TEST(testcaseInS8("123456A5", 15, 0, 5, 5, true));
    RUN_TEST(testcaseInS8("123456AF", 15, 0, 5, 15, true));
    RUN_TEST(testcaseInS8("12345FA5", 15, 0, 0, 0, false));

    TEST_CASE_END;
}

int DPT16() {
    TEST_CASE_START;

    //TODO String is not implemented currently
    //RUN_TEST(testcaseInOutString("4B4E58206973204F4B0000000000", 16, 0, 0, "KNX is OK", true));
    //RUN_TEST(testcaseInOutString("4B4E58206973204F4B0000000000", 16, 1, 0, "KNX is OK", true));

    TEST_CASE_END;
}

int DPT17() {
    TEST_CASE_START;

    RUN_TEST(testcaseInOutU8("03", 17, 1, 0, 3, true));
    RUN_TEST(testcaseInOutU8("35", 17, 1, 0, 53, true));
    RUN_TEST(testcaseInU8("", 17, 1, 0, 0, false));
    RUN_TEST(testcaseInU8("0103", 17, 1, 0, 0, false));

    TEST_CASE_END;
}

int DPT18() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU8("03", 18, 1, 0, 0, true));
    RUN_TEST(testcaseInU8("03", 18, 1, 1, 3, true));
    RUN_TEST(testcaseInU8("83", 17, 1, 0, 3, true));
    RUN_TEST(testcaseInU8("03", 18, 1, 2, 0, false));
    RUN_TEST(testcaseInU8("75", 18, 1, 0, 0, true));
    RUN_TEST(testcaseInU8("75", 18, 1, 1, 53, true));

    RUN_TEST(testcaseInU8("", 18, 1, 1, 53, false));
    RUN_TEST(testcaseInU8("7503", 18, 1, 1, 53, false));

    TEST_CASE_END;
}

int DPT19() {
    TEST_CASE_START;

    struct tm exp;
    bzero(&exp, sizeof(struct tm));
    exp.tm_hour = 10;
    exp.tm_min = 26;
    exp.tm_sec = 39;
    exp.tm_mday = 23;
    exp.tm_mon = 9;
    exp.tm_year = 2014;

    RUN_TEST(testcaseInTime("7209174A1A274180", 19, 0, 0, exp, false));
    RUN_TEST(testcaseInTime("7209174A1A274180", 19, 1, 0, exp, true));
    RUN_TEST(testcaseInU8("7209174A1A274180", 19, 1, 1, 2, true));
    RUN_TEST(testcaseInU8("7209174A1A274180", 19, 1, 2, 1, true));
    RUN_TEST(testcaseInU8("7209174A1A274180", 19, 1, 3, 0, true));
    RUN_TEST(testcaseInU8("7209174A1A274180", 19, 1, 9, 1, true));
    RUN_TEST(testcaseInU8("7209174A1A274180", 19, 1, 10, 1, true));

    RUN_TEST(testcaseInU8("", 19, 1, 10, 1, false));
    RUN_TEST(testcaseInU8("AA", 19, 1, 10, 1, false));
    RUN_TEST(testcaseInU8("AAAA", 19, 1, 10, 1, false));
    RUN_TEST(testcaseInU8("AAAAAA", 19, 1, 10, 1, false));
    RUN_TEST(testcaseInU8("AAAAAAAA", 19, 1, 10, 1, false));
    RUN_TEST(testcaseInU8("AAAAAAAAAA", 19, 1, 10, 1, false));
    RUN_TEST(testcaseInU8("AAAAAAAAAAAA", 19, 1, 10, 1, false));
    RUN_TEST(testcaseInU8("AAAAAAAAAAAAAA", 19, 1, 10, 1, false));
    RUN_TEST(testcaseInU8("AAAAAAAAAAAAAAAAAA", 19, 1, 10, 1, false));

    KNXValue v;
    v.tValue = exp;
    RUN_TEST(testcaseOut(v, 19, 0, 0, "", 0, 0));
    RUN_TEST(testcaseOut(v, 19, 1, 0, "7209170A1A270000", 8, true));

    KNX_ASSUME_KNX_VALUE(v, 2);
    RUN_TEST(testcaseOut(v, 19, 1, 1, "0000004000000000", 8, true));

    KNX_ASSUME_KNX_VALUE(v, 1);
    RUN_TEST(testcaseOut(v, 19, 1, 2, "0000000000004000", 8, true));

    KNX_ASSUME_KNX_VALUE(v, 1);
    RUN_TEST(testcaseOut(v, 19, 1, 3, "0000000000008000", 8, true));

    KNX_ASSUME_KNX_VALUE(v, 1);
    RUN_TEST(testcaseOut(v, 19, 1, 9, "0000000000000100", 8, true));

    KNX_ASSUME_KNX_VALUE(v, 1);
    RUN_TEST(testcaseOut(v, 19, 1, 10, "0000000000000080", 8, true));

    //TODO add test cases, when todatetimes impl is done
    //7209170000000200 == 2014-09-23
    //0000000A1A271800 == 10:24:39


    TEST_CASE_END;
}

int DPT26() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU8("03", 26, 1, 0, 0, true));
    RUN_TEST(testcaseInU8("03", 26, 1, 1, 3, true));
    RUN_TEST(testcaseInU8("83", 26, 1, 1, 3, true));
    RUN_TEST(testcaseInU8("B5", 26, 1, 1, 53, true));

    RUN_TEST(testcaseInU8("83", 26, 1, 2, 3, false));
    RUN_TEST(testcaseInU8("", 26, 1, 2, 3, false));
    RUN_TEST(testcaseInU8("8383", 26, 1, 2, 3, false));

    TEST_CASE_END;
}

int DPT28() {
    TEST_CASE_START;

    //TODO

    TEST_CASE_END;
}

int DPT29() {
    TEST_CASE_START;

    RUN_TEST(testcaseInS64("FFFFFFFFFFFFFFFF", 29, 1, 0, 0, false));
    RUN_TEST(testcaseInOutS64("FFFFFFFFFFFFFFFF", 29, 10, 0, -1, true));
    RUN_TEST(testcaseInOutS64("7FFFFFFFFFFFFFFF", 29, 10, 0, 9223372036854775807, true));
    RUN_TEST(testcaseInOutS64("8000000000000000", 29, 10, 0, (int64_t)-9223372036854775808, true));
    RUN_TEST(testcaseInOutS64("0000000000000000", 29, 10, 0, 0, true));

    RUN_TEST(testcaseInS64("", 29, 10, 0, 0, false));
    RUN_TEST(testcaseInS64("0000", 29, 10, 0, 0, false));
    RUN_TEST(testcaseInS64("000000", 29, 10, 0, 0, false));
    RUN_TEST(testcaseInS64("00000000", 29, 10, 0, 0, false));
    RUN_TEST(testcaseInS64("0000000000", 29, 10, 0, 0, false));
    RUN_TEST(testcaseInS64("000000000000", 29, 10, 0, 0, false));
    RUN_TEST(testcaseInS64("00000000000000", 29, 10, 0, 0, false));
    RUN_TEST(testcaseInS64("000000000000000000", 29, 10, 0, 0, false));

    TEST_CASE_END;
}

int DPT217() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU16("1EF0", 217, 1, 0, 3, true));
    RUN_TEST(testcaseInU16("1EF0", 217, 1, 1, 27, true));
    RUN_TEST(testcaseInU16("1EF0", 217, 1, 2, 48, true));

    RUN_TEST(testcaseInU16("", 217, 1, 0, 3, false));
    RUN_TEST(testcaseInU16("1E", 217, 1, 0, 3, false));
    RUN_TEST(testcaseInU16("1E1E1E", 217, 1, 0, 3, false));

    KNXValue v;
    KNX_ASSUME_KNX_VALUE(v, 3);
    RUN_TEST(testcaseOut(v, 217, 1, 0, "1800", 2, true));

    KNX_ASSUME_KNX_VALUE(v, 27);
    RUN_TEST(testcaseOut(v, 217, 1, 1, "06C0", 2, true));

    KNX_ASSUME_KNX_VALUE(v, 48);
    RUN_TEST(testcaseOut(v, 217, 1, 2, "0030", 2, true));

    KNX_ASSUME_KNX_VALUE(v, 35);
    RUN_TEST(testcaseOut(v, 217, 1, 0, "", 0, false));
    KNX_ASSUME_KNX_VALUE(v, 36);
    RUN_TEST(testcaseOut(v, 217, 1, 1, "", 0, false));
    KNX_ASSUME_KNX_VALUE(v, 72);
    RUN_TEST(testcaseOut(v, 217, 1, 2, "", 0, false));


    TEST_CASE_END;
}

int DPT221() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU32("D327017F4CC2", 221, 0, 1, 0, false));
    RUN_TEST(testcaseInU32("D327017F4CC2", 221, 1, 0, 54055, true));
    RUN_TEST(testcaseInU32("D327017F4CC2", 221, 1, 1, 25119938, true));

    RUN_TEST(testcaseInU32("", 221, 1, 1, 25119938, false));
    RUN_TEST(testcaseInU32("AB", 221, 1, 1, 25119938, false));
    RUN_TEST(testcaseInU32("ABCD", 221, 1, 1, 25119938, false));
    RUN_TEST(testcaseInU32("ABCDEF", 221, 1, 1, 25119938, false));
    RUN_TEST(testcaseInU32("ABCDEFAB", 221, 1, 1, 25119938, false));
    RUN_TEST(testcaseInU32("ABCDEFABCD", 221, 1, 1, 25119938, false));
    RUN_TEST(testcaseInU32("ABCDEFABCDEFAB", 221, 1, 1, 25119938, false));

    KNXValue v;
    KNX_ASSUME_KNX_VALUE(v, 54055);
    RUN_TEST(testcaseOut(v, 221, 1, 0, "D32700000000", 6, true));


    TEST_CASE_END;
}

int DPT225() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU32("014F0C", 225, 1, 0, 335, true));
    RUN_TEST(testcaseInU32("014F0C", 225, 1, 1, 4, true));
    RUN_TEST(testcaseInU32("82DC0C", 225, 2, 0, 33500, true));

    RUN_TEST(testcaseInU32("01A44D", 225, 3, 0, 420, true));
    RUN_TEST(testcaseInU32("01A44D", 225, 3, 1, 77, true));

    KNXValue v;
    KNX_ASSUME_KNX_VALUE(v, 335);
    RUN_TEST(testcaseOut(v, 225, 1, 0, "014F00", 3, true));

    KNX_ASSUME_KNX_VALUE(v, 33500);
    RUN_TEST(testcaseOut(v, 225, 2, 0, "82DC00", 3, true));

    KNX_ASSUME_KNX_VALUE(v, 420);
    RUN_TEST(testcaseOut(v, 225, 3, 0, "01A400", 3, true));

    KNX_ASSUME_KNX_VALUE(v, 77);
    RUN_TEST(testcaseOut(v, 225, 3, 1, "00004D", 3, true));

    TEST_CASE_END;
}

int DPT231() {
    TEST_CASE_START;

    //TODO ADD TEST CASES

    TEST_CASE_END;
}
int DPT234() {
    TEST_CASE_START;

    //TODO ADD TEST CASES

    TEST_CASE_END;
}

int DPT232() {
    TEST_CASE_START;

    RUN_TEST(testcaseInOutU64("FFFFFF", 232, 600, 0, 16777215, true));
    RUN_TEST(testcaseInOutU64("000000", 232, 600, 0, 0, true));
    RUN_TEST(testcaseInOutU64("906085", 232, 600, 0, 9461893, true));


    RUN_TEST(testcaseInU64("", 232, 600, 0, 0, false));
    RUN_TEST(testcaseInU64("00", 232, 600, 0, 0, false));
    RUN_TEST(testcaseInU64("0000", 232, 600, 0, 0, false));
    RUN_TEST(testcaseInU64("00000000", 232, 600, 0, 0, false));

    TEST_CASE_END;
}

int DPT235() {
    TEST_CASE_START;

    RUN_TEST(testcaseInS64("3459F0252F03", 235, 1, 0, 878309413, true));
    RUN_TEST(testcaseInS64("3459F0252F03", 235, 1, 1, 47, true));
    RUN_TEST(testcaseInS64("3459F0252F03", 235, 1, 2, 1, true));
    RUN_TEST(testcaseInS64("3459F0252F03", 235, 1, 3, 1, true));

    RUN_TEST(testcaseInS64("3459F0252F03", 235, 0, 0, 0, false));
    RUN_TEST(testcaseInS64("3459F0252F", 235, 0, 0, 0, false));
    RUN_TEST(testcaseInS64("", 235, 0, 0, 0, false));
    RUN_TEST(testcaseInS64("3459F0252F0382", 235, 0, 0, 0, false));

    KNXValue v;
    KNX_ASSUME_KNX_VALUE(v, 878309413);
    RUN_TEST(testcaseOut(v, 235, 1, 0, "3459F0250000", 6, true));

    KNX_ASSUME_KNX_VALUE(v, 47);
    RUN_TEST(testcaseOut(v, 235, 1, 1, "000000002F00", 6, true));

    KNX_ASSUME_KNX_VALUE(v, 1);
    RUN_TEST(testcaseOut(v, 235, 1, 2, "000000000002", 6, true));
    RUN_TEST(testcaseOut(v, 235, 1, 3, "000000000001", 6, true));

    TEST_CASE_END;
}

int DPT238() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU8("03", 238, 0, 0, 0, false));
    RUN_TEST(testcaseInU8("03", 238, 1, 3, 0, false));

    RUN_TEST(testcaseInU8("03", 238, 1, 0, 3, true));
    RUN_TEST(testcaseInU8("03", 238, 1, 1, 0, true));

    RUN_TEST(testcaseInU8("83", 238, 1, 0, 3, true));
    RUN_TEST(testcaseInU8("83", 238, 1, 2, 1, true));

    RUN_TEST(testcaseInU8("", 238, 1, 2, 1, false));
    RUN_TEST(testcaseInU8("FFFF", 238, 1, 2, 1, false));

    TEST_CASE_END;
}

int DPT239() {
    TEST_CASE_START;

    RUN_TEST(testcaseInU16("C401", 239, 1, 0, 76, true));
    RUN_TEST(testcaseInU16("C4FF", 239, 1, 1, 1, true));

    KNXValue v;
    KNX_ASSUME_KNX_VALUE(v, 76);
    RUN_TEST(testcaseOut(v, 239, 1, 0, "C200", 2, true));

    KNX_ASSUME_KNX_VALUE(v, 1);
    RUN_TEST(testcaseOut(v, 239, 1, 1, "0001", 2, true));

    KNX_ASSUME_KNX_VALUE(v, 0);
    RUN_TEST(testcaseOut(v, 239, 1, 0, "0000", 2, true));

    RUN_TEST(testcaseInU16("C401", 239, 0, 0, 0, false));
    RUN_TEST(testcaseInU16("", 239, 1, 0, 0, false));
    RUN_TEST(testcaseInU16("01", 239, 1, 0, 0, false));
    RUN_TEST(testcaseInU16("01C491", 239, 1, 0, 0, false));

    TEST_CASE_END;
}

int main (int ac, char *ag[])
{
    RUN_TEST_GROUP(DPT1());
    RUN_TEST_GROUP(DPT2());
    RUN_TEST_GROUP(DPT3());
    RUN_TEST_GROUP(DPT4());
    RUN_TEST_GROUP(DPT5());
    RUN_TEST_GROUP(DPT6());
    RUN_TEST_GROUP(DPT7());
    RUN_TEST_GROUP(DPT8());
    RUN_TEST_GROUP(DPT9());
    RUN_TEST_GROUP(DPT10());
    RUN_TEST_GROUP(DPT11());
    RUN_TEST_GROUP(DPT12());
    RUN_TEST_GROUP(DPT13());
    RUN_TEST_GROUP(DPT14());
    RUN_TEST_GROUP(DPT15());
    RUN_TEST_GROUP(DPT17());
    RUN_TEST_GROUP(DPT18());
    RUN_TEST_GROUP(DPT19());
    RUN_TEST_GROUP(DPT26());
    RUN_TEST_GROUP(DPT29());
    RUN_TEST_GROUP(DPT217());
    RUN_TEST_GROUP(DPT221());
    RUN_TEST_GROUP(DPT225());
    RUN_TEST_GROUP(DPT232());
    RUN_TEST_GROUP(DPT235());
    RUN_TEST_GROUP(DPT238());
    RUN_TEST_GROUP(DPT239());


    //TODO
    /*
    RUN_TEST_GROUP(DPT16());
    RUN_TEST_GROUP(DPT28());
    RUN_TEST_GROUP(DPT231());
    RUN_TEST_GROUP(DPT234());
    */
    printf("\n>>>>>>>>>>All tests completed successfully<<<<<<<<<<<\n\n");
}
