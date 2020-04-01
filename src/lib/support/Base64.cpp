/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      Base-64 utility functions.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <ctype.h>
#include <stdint.h>

#include "Base64.h"

namespace chip {

// Convert a value in the range 0..63 to its equivalent base64 character.
// Return '=' for any value >= 64.
static char Base64ValToChar(uint8_t val)
{
    if (val < 26)
        return 'A' + val;
    val -= 26;
    if (val < 26)
        return 'a' + val;
    val -= 26;
    if (val < 10)
        return '0' + val;
    if (val == 10)
        return '+';
    if (val == 11)
        return '/';
    return '=';
}

// Convert a base64 character to a value in the range 0..63, or UINT8_MAX if the character is invalid.
static uint8_t Base64CharToVal(uint8_t c)
{
    if (c == 43)
        return 62;
    if (c == 47)
        return 63;
    // NOTE: c < 48 will fall through to return UINT8_MAX below.
    c -= 48;
    if (c < 10)
        return c + 52;
    c -= 17;
    if (c < 26)
        return c;
    c -= 32;
    if (c < 26)
        return c + 26;
    return UINT8_MAX;
}

// Convert a value in the range 0..63 to its equivalent base64url character (see RFC-4648, section 5).
// Return '=' for any value >= 64.
static char Base64URLValToChar(uint8_t val)
{
    if (val < 26)
        return 'A' + val;
    val -= 26;
    if (val < 26)
        return 'a' + val;
    val -= 26;
    if (val < 10)
        return '0' + val;
    if (val == 10)
        return '-';
    if (val == 11)
        return '_';
    return '=';
}

// Convert a base64url character to a value in the range 0..63, or UINT8_MAX if the character is invalid.
static uint8_t Base64URLCharToVal(uint8_t c)
{
    if (c == 45)
        return 62;
    if (c == 95)
        return 63;
    // NOTE: c < 48 will fall through to return UINT8_MAX below.
    c -= 48;
    if (c < 10)
        return c + 52;
    c -= 17;
    if (c < 26)
        return c;
    c -= 32;
    if (c < 26)
        return c + 26;
    return UINT8_MAX;
}

uint16_t Base64Encode(const uint8_t * in, uint16_t inLen, char * out, Base64ValToCharFunct valToCharFunct)
{
    char * outStart = out;

    while (inLen > 0)
    {
        uint8_t val1, val2, val3, val4;

        val1 = *in >> 2;
        val2 = (*in << 4) & 0x3F;
        in++;
        inLen--;
        if (inLen > 0)
        {
            val2 |= *in >> 4;
            val3 = (*in << 2) & 0x3F;
            in++;
            inLen--;
            if (inLen > 0)
            {
                val3 |= *in >> 6;
                val4 = *in & 0x3F;
                in++;
                inLen--;
            }
            else
                val4 = UINT8_MAX;
        }
        else
            val3 = val4 = UINT8_MAX;

        *out++ = valToCharFunct(val1);
        *out++ = valToCharFunct(val2);
        *out++ = valToCharFunct(val3);
        *out++ = valToCharFunct(val4);
    }

    return out - outStart;
}

uint16_t Base64Encode(const uint8_t * in, uint16_t inLen, char * out)
{
    return Base64Encode(in, inLen, out, Base64ValToChar);
}

uint16_t Base64URLEncode(const uint8_t * in, uint16_t inLen, char * out)
{
    return Base64Encode(in, inLen, out, Base64URLValToChar);
}

uint32_t Base64Encode32(const uint8_t * in, uint32_t inLen, char * out, Base64ValToCharFunct valToCharFunct)
{
    uint32_t outLen = 0;

    // Maximum number of input bytes to convert to base-64 in a single call to Base64Encode.
    // Number is the largest multiple of 3 bytes where the resulting number of base-64 characters
    // fits within a uint16_t.
    enum
    {
        kMaxConvert = (UINT16_MAX / 4) * 3
    };

    while (true)
    {
        uint16_t inChunkLen = (inLen > kMaxConvert) ? (uint16_t) kMaxConvert : (uint16_t) inLen;

        uint16_t outChunkLen = Base64Encode(in, inChunkLen, out, valToCharFunct);

        inLen -= inChunkLen;
        outLen += outChunkLen;

        if (inLen == 0)
            break;

        in += inChunkLen;
        out += outChunkLen;
    }

    return outLen;
}

uint32_t Base64Encode32(const uint8_t * in, uint32_t inLen, char * out)
{
    return Base64Encode32(in, inLen, out, Base64ValToChar);
}

uint16_t Base64Decode(const char * in, uint16_t inLen, uint8_t * out, Base64CharToValFunct charToValFunct)
{
    uint8_t * outStart = out;

    // isgraph() returns false for space and ctrl chars
    while (inLen > 0 && isgraph(*in))
    {
        if (inLen == 1)
            goto fail;

        uint8_t a = charToValFunct(*in++);
        uint8_t b = charToValFunct(*in++);
        inLen -= 2;

        if (a == UINT8_MAX || b == UINT8_MAX)
            goto fail;

        *out++ = (a << 2) | (b >> 4);

        if (inLen == 0 || *in == '=')
            break;

        uint8_t c = charToValFunct(*in++);
        inLen--;

        if (c == UINT8_MAX)
            goto fail;

        *out++ = (b << 4) | (c >> 2);

        if (inLen == 0 || *in == '=')
            break;

        uint8_t d = charToValFunct(*in++);
        inLen--;

        if (d == UINT8_MAX)
            goto fail;

        *out++ = (c << 6) | d;
    }

    return out - outStart;

fail:
    return UINT16_MAX;
}

uint16_t Base64Decode(const char * in, uint16_t inLen, uint8_t * out)
{
    return Base64Decode(in, inLen, out, Base64CharToVal);
}

uint16_t Base64URLDecode(const char * in, uint16_t inLen, uint8_t * out)
{
    return Base64Decode(in, inLen, out, Base64URLCharToVal);
}

uint32_t Base64Decode32(const char * in, uint32_t inLen, uint8_t * out, Base64CharToValFunct charToValFunct)
{
    uint32_t outLen = 0;

    // Maximum number of base-64 characters to convert in a single call to Base64Decode.
    // Number is the largest multiple of 4 characters that fits in a uint16_t.
    enum
    {
        kMaxConvert = (UINT16_MAX / 4) * 4
    };

    while (true)
    {
        uint16_t inChunkLen = (inLen > kMaxConvert) ? (uint16_t) kMaxConvert : (uint16_t) inLen;

        uint16_t outChunkLen = Base64Decode(in, inChunkLen, out, charToValFunct);
        if (outChunkLen == UINT16_MAX)
            return UINT32_MAX;

        inLen -= inChunkLen;
        outLen += outChunkLen;

        if (inLen == 0)
            break;

        in += inChunkLen;
        out += outChunkLen;
    }

    return outLen;
}

uint32_t Base64Decode32(const char * in, uint32_t inLen, uint8_t * out)
{
    return Base64Decode32(in, inLen, out, Base64CharToVal);
}

} // namespace chip

#ifdef TEST

#include <string.h>
#include <stdio.h>

void TestBase64(const char * test, bool base64URL = false)
{
    uint8_t buf[256];
    char buf2[256];
    uint16_t len;

    strcpy((char *) buf, test);

    len = (base64URL) ? nl::Base64URLDecode((char *) buf, strlen((char *) buf), buf)
                      : nl::Base64Decode((char *) buf, strlen((char *) buf), buf);
    printf("%s: ", test);
    if (len != UINT16_MAX)
    {
        printf("(%d) ", len);
        for (uint16_t i = 0; i < len; i++)
            printf("%c", buf[i]);

        len = (base64URL) ? nl::Base64URLEncode(buf, len, buf2) : nl::Base64Encode(buf, len, buf2);
        printf(" (%d) ", len);
        for (uint16_t i = 0; i < len; i++)
            printf("%c", buf2[i]);
    }
    else
        printf("ERROR");
    printf("\n");
}

int main(int argc, char * argv[])
{
    TestBase64("");
    TestBase64("Zg==");
    TestBase64("Zm8=");
    TestBase64("Zm9v");
    TestBase64("Zm9vYg==");
    TestBase64("Zm9vYmE=");
    TestBase64("Zm9vYmFy");
    TestBase64("QmFzZTY0D+8xMjM0D/8=");

    TestBase64("Zg");
    TestBase64("Zm8");
    TestBase64("Zm9vYg");
    TestBase64("Zm9vYmE");

    TestBase64("QmFzZTY0D-8xMjM0D_8=", true);

    // Error cases
    TestBase64("Z");
    TestBase64("Z\x019vYmFy");
    TestBase64("Zm9vY");
    TestBase64("Zm9vY;");
    TestBase64("Zm9 vYg");
}

#endif // TEST
