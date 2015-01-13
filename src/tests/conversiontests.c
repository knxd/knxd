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

#define RUN_TEST_GROUP(t) printf("Start running TestGroup %s\n", #t); t; printf("TestGroup %s run completed\n\n", #t);
#define RUN_TEST_SILENT(t) if(!t) return 0;
#define RUN_TEST(t) printf("Running test #%d\n", completedTests); if(!t){ printf("Test %s failed after tests #%d\n", #t, completedTests); return 0; } else completedTests++; printf("\n");

#define ASSERT(x,y) if(x!=y) { printf("ASSERT failes at %s:%d FirstValue %lld SecondValue %lld\n",__FILE__, __LINE__, x, y); return 0; }

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
    out[len*1+1] = '\0';
}

int testcaseIn(const char *payload, unsigned short mainGroup, unsigned short subGroup, unsigned short index, uint64_t expectedValue, int decode_ret_val) {
    char data[strlen(payload)/2];
    int payLen = hexToByteArray(payload, strlen(payload), data);

    printf("INCOMING: DPT=%d.%d-%d PAYLOAD=%s ", mainGroup, subGroup, index, payload);

    KNXValue val;
    KNXDatatype dp;
    dp.mainGroup = mainGroup;
    dp.subGroup = subGroup;
    dp.index = index;
    int retVal = KNX_Decode_Value(data, payLen, dp, &val);

    if(retVal)
        printf("Data=%lld\n", val.uiValue);
    else
        printf("\n");
    ASSERT(retVal, decode_ret_val);
    if(!retVal)
        return 1;
    ASSERT(val.uiValue, expectedValue);
    return 1;
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
        return 1;
    int memcmpResult = memcmp(data, expectedResultData, expectedLen) != 0;

    ASSERT(memcmpResult, 0);


    return 1;
}

int testcaseInOut(char const *payload, unsigned short mainGroup, unsigned short subGroup, unsigned short index, uint64_t attrValue, int endecode_ret_val) {
    RUN_TEST_SILENT(testcaseIn(payload, mainGroup, subGroup, index, attrValue, endecode_ret_val));
    KNXValue t;
    KNX_ASSUME_KNX_VALUE(t, attrValue);
    RUN_TEST_SILENT(testcaseOut(t, mainGroup, subGroup, index, payload, strlen(payload)/2, endecode_ret_val));
}

int DPT1() {
    int completedTests = 0;
    RUN_TEST(testcaseInOut("00", 1, 2, 0, 0, 1));
    RUN_TEST(testcaseInOut("01", 1, 2, 0, 1, 1));
    RUN_TEST(testcaseInOut("00", 1, 10, 0,  0, 1));
    RUN_TEST(testcaseInOut("01", 1, 11, 0,  1, 1));
    RUN_TEST(testcaseInOut("01", 1, 5, 0,  1, 1));
    RUN_TEST(testcaseInOut("01", 1, 3, 0, 1, 1));
    RUN_TEST(testcaseInOut("00", 1, 3, 0, 0, 1));
    RUN_TEST(testcaseIn("02", 1, 3, 0, 0, 1));
    RUN_TEST(testcaseInOut("01", 1, 6, 0, 1, 1));
    RUN_TEST(testcaseInOut("00", 1, 15, 0, 0, 1));
    RUN_TEST(testcaseIn("0100", 1, 1, 0, 0, 0));
    RUN_TEST(testcaseIn("", 1, 2, 0, 0, 0));
    RUN_TEST(testcaseIn("FF", 1, 1, 0,  1, 1));
    return 1;
}

int DPT2() {
    int completedTests = 0;
    RUN_TEST(testcaseIn("03", 2, 2, 0, 1, 1));
    RUN_TEST(testcaseIn("03", 2, 2, 1, 1, 1));
    RUN_TEST(testcaseIn("02", 2, 2, 1, 0, 1));
    RUN_TEST(testcaseIn("01", 2, 2, 0, 0, 1));
    RUN_TEST(testcaseIn("00", 2, 11, 0, 0, 1));
    RUN_TEST(testcaseIn("03", 2, 11, 1, 1, 1));
    RUN_TEST(testcaseIn("02", 2, 11, 0, 1, 1));
    RUN_TEST(testcaseIn("03", 2, 5, 0, 1, 1));
    RUN_TEST(testcaseIn("01", 2, 3, 1, 1, 1));
    RUN_TEST(testcaseIn("01", 2, 3, 0, 0, 1));
    RUN_TEST(testcaseIn("01", 2, 6, 1, 1, 1));
    RUN_TEST(testcaseIn("00", 2, 8, 0, 0, 1));
    RUN_TEST(testcaseIn("03", 2, 3, 0, 1, 1));
    RUN_TEST(testcaseIn("02D0", 2, 1, 0, 0, 0));
    RUN_TEST(testcaseIn("", 2, 2, 1, 0,0));

    return 1;
}

int DPT3() {
    int completedTests = 0;

    RUN_TEST(testcaseIn("08", 3, 7, 0, 1, 1));
    RUN_TEST(testcaseIn("05", 3, 7, 0, 0, 1));
    RUN_TEST(testcaseIn("08", 3, 7, 1, 0, 1));
    RUN_TEST(testcaseIn("08", 3, 7, 1, 0, 1));
    RUN_TEST(testcaseIn("0B", 3, 7, 1, 3, 1));
    RUN_TEST(testcaseIn("0C", 3, 7, 1, 4, 1));
    RUN_TEST(testcaseIn("0C", 3, 7, 1, 4, 1));
    RUN_TEST(testcaseIn("07", 3, 7, 1, 7, 1));
    RUN_TEST(testcaseIn("0620A5", 3, 7, 1, 0, 0));
    RUN_TEST(testcaseIn("", 3, 7, 1, 0, 0));
    RUN_TEST(testcaseIn("00", 3, 7, 1, 0, 1));
    RUN_TEST(testcaseIn("01", 3, 7, 1, 1, 1));
    RUN_TEST(testcaseIn("02", 3, 7, 1,  2, 1));
    RUN_TEST(testcaseIn("03", 3, 7, 1,  3, 1));
    RUN_TEST(testcaseIn("FF", 3, 7, 1, 7, 1));

    return 1;
}

int main (int ac, char *ag[])
{
    RUN_TEST_GROUP(DPT1());
    RUN_TEST_GROUP(DPT2());
    RUN_TEST_GROUP(DPT3());

    printf("\n>>>>>>>>>>All tests completed successfully<<<<<<<<<<<\n\n");
}
