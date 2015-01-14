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
    TESTCASE_IN(printf("Data=4.3f%\n", val.dValue));
    ASSERT(roundf(val.dValue*1000)/1000, roundf(expectedValue*1000)/1000);
    return true;
}

TESTCASE_IN_DEF(testcaseInS8, int8_t) {
    TESTCASE_IN(printf("Data=%lld\n", val.uiValue));
    int8_t signedValue = val.uiValue;
    ASSERT(signedValue, (int8_t)expectedValue);
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

TESTCASE_IN_DEF(testcaseInU64, uint64_t) {
    TESTCASE_IN(printf("Data=%lld\n", val.uiValue));
    ASSERT(val.uiValue, expectedValue);
    return true;
}

TESTCASE_IN_DEF(testcaseInTime, struct tm) {
    TESTCASE_IN(printf("Hour: %d  Min: %d  Sec: %d\n", val.tValue.tm_hour, val.tValue.tm_min, val.tValue.tm_sec));

    ASSERT(mktime(&val.tValue), mktime(&expectedValue));
    return true;

}

int testcaseOut(KNXValue value, unsigned short mainGroup, unsigned short subGroup, unsigned short index, const char *expectedResult, int expectedLen, int encode_ret_val) {
    printf("OUTGOING: DPT=%d.%d-%d Value=%lld ", mainGroup, subGroup, index, value.uiValue);
    char data[expectedLen];
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

int testcaseInOut(char const *payload, unsigned short mainGroup, unsigned short subGroup, unsigned short index, uint64_t attrValue, int endecode_ret_val) {
    RUN_TEST_SILENT(testcaseInU64(payload, mainGroup, subGroup, index, attrValue, endecode_ret_val));
    KNXValue t;
    KNX_ASSUME_KNX_VALUE(t, attrValue);
    RUN_TEST_SILENT(testcaseOut(t, mainGroup, subGroup, index, payload, strlen(payload)/2, endecode_ret_val));
}

int DPT1() {
    TEST_CASE_START;

    RUN_TEST(testcaseInOut("00", 1, 2, 0, 0, true));
    RUN_TEST(testcaseInOut("01", 1, 2, 0, 1, true));
    RUN_TEST(testcaseInOut("00", 1, 10, 0,  0, true));
    RUN_TEST(testcaseInOut("01", 1, 11, 0,  1, true));
    RUN_TEST(testcaseInOut("01", 1, 5, 0,  1, true));
    RUN_TEST(testcaseInOut("01", 1, 3, 0, 1, true));
    RUN_TEST(testcaseInOut("00", 1, 3, 0, 0, true));
    RUN_TEST(testcaseInU64("02", 1, 3, 0, 0, true));
    RUN_TEST(testcaseInOut("01", 1, 6, 0, 1, true));
    RUN_TEST(testcaseInOut("00", 1, 15, 0, 0, true));
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


    TEST_CASE_END;
}

int main (int ac, char *ag[])
{
    /*RUN_TEST_GROUP(DPT1());
    RUN_TEST_GROUP(DPT2());
    RUN_TEST_GROUP(DPT3());
    RUN_TEST_GROUP(DPT4());
    RUN_TEST_GROUP(DPT5());
    RUN_TEST_GROUP(DPT6());
    RUN_TEST_GROUP(DPT7());
    RUN_TEST_GROUP(DPT8());
    RUN_TEST_GROUP(DPT9());
    RUN_TEST_GROUP(DPT10());*/

    RUN_TEST_GROUP(DPT11());

    printf("\n>>>>>>>>>>All tests completed successfully<<<<<<<<<<<\n\n");
}
