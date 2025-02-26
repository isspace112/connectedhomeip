/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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
 *      This file implements a unit test suite for the Platform Time
 *      code functionality.
 *
 */

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nlunit-test.h>
#include <support/CodeUtils.h>
#include <support/UnitTestRegistration.h>
#include <system/SystemClock.h>

#include <platform/internal/CHIPDeviceLayerInternal.h>

#define TEST_TIME_MARGIN_MS 2
#define TEST_TIME_MARGIN_US 500

using namespace chip;
using namespace chip::Logging;
using namespace chip::System::Platform::Clock;

// =================================
//      Test Vectors
// =================================

struct time_test_vector
{
    uint64_t delay;
};

static const struct time_test_vector test_vector_system_time_ms[] = {
    { .delay = 10 },
    { .delay = 100 },
    { .delay = 250 },
};

static const struct time_test_vector test_vector_system_time_us[] = {
    { .delay = 600 },
    { .delay = 900 },
    { .delay = 1500 },
};

// =================================
//      OS-specific utils
// =================================
// TODO: Make tests OS agnostic

#include <unistd.h>

void test_os_sleep_us(uint64_t microsecs)
{
    usleep(static_cast<useconds_t>(microsecs));
}

void test_os_sleep_ms(uint64_t millisecs)
{
    test_os_sleep_us(millisecs * 1000);
}

// =================================
//      Unit tests
// =================================

static void TestDevice_GetMonotonicMicroseconds(nlTestSuite * inSuite, void * inContext)
{
    int numOfTestVectors = ArraySize(test_vector_system_time_us);
    int numOfTestsRan    = 0;
    const struct time_test_vector * test_params;

    uint64_t margin = TEST_TIME_MARGIN_US;
    uint64_t Tstart, Tend, Tdelta, Tdelay;

    for (int vectorIndex = 0; vectorIndex < numOfTestVectors; vectorIndex++)
    {
        test_params = &test_vector_system_time_us[vectorIndex];
        Tdelay      = test_params->delay;
        Tstart      = GetMonotonicMicroseconds();

        test_os_sleep_us(test_params->delay);

        Tend   = GetMonotonicMicroseconds();
        Tdelta = Tend - Tstart;

        ChipLogProgress(DeviceLayer, "Start=%" PRIu64 " End=%" PRIu64 " Delta=%" PRIu64 " Expected=%" PRIu64, Tstart, Tend, Tdelta,
                        Tdelay);
        // verify that timers don't fire early
        NL_TEST_ASSERT(inSuite, Tdelta > (Tdelay - margin));
        // verify they're not too late
        //        NL_TEST_ASSERT(inSuite, Tdelta < (Tdelay + margin));
        numOfTestsRan++;
    }
    NL_TEST_ASSERT(inSuite, numOfTestsRan > 0);
}

static void TestDevice_GetMonotonicMilliseconds(nlTestSuite * inSuite, void * inContext)
{
    int numOfTestVectors = ArraySize(test_vector_system_time_ms);
    int numOfTestsRan    = 0;
    const struct time_test_vector * test_params;

    uint64_t margin = TEST_TIME_MARGIN_MS;
    uint64_t Tstart, Tend, Tdelta, Tdelay;

    for (int vectorIndex = 0; vectorIndex < numOfTestVectors; vectorIndex++)
    {
        test_params = &test_vector_system_time_ms[vectorIndex];
        Tdelay      = test_params->delay;
        Tstart      = GetMonotonicMilliseconds();

        test_os_sleep_ms(test_params->delay);

        Tend   = GetMonotonicMilliseconds();
        Tdelta = Tend - Tstart;

        ChipLogProgress(DeviceLayer, "Start=%" PRIu64 " End=%" PRIu64 " Delta=%" PRIu64 " Expected=%" PRIu64, Tstart, Tend, Tdelta,
                        Tdelay);
        // verify that timers don't fire early
        NL_TEST_ASSERT(inSuite, Tdelta > (Tdelay - margin));
        // verify they're not too late
        //        NL_TEST_ASSERT(inSuite, Tdelta < (Tdelay + margin));
        numOfTestsRan++;
    }
    NL_TEST_ASSERT(inSuite, numOfTestsRan > 0);
}

/**
 *   Test Suite. It lists all the test functions.
 */
static const nlTest sTests[] = {

    NL_TEST_DEF("Test DeviceLayer::GetMonotonicMicroseconds", TestDevice_GetMonotonicMicroseconds),
    NL_TEST_DEF("Test DeviceLayer::GetMonotonicMilliseconds", TestDevice_GetMonotonicMilliseconds),

    NL_TEST_SENTINEL()
};

int TestPlatformTime()
{
    nlTestSuite theSuite = { "PlatformTime tests", &sTests[0], nullptr, nullptr };

    // Run test suit againt one context.
    nlTestRunner(&theSuite, nullptr);
    return nlTestRunnerStats(&theSuite);
}

CHIP_REGISTER_TEST_SUITE(TestPlatformTime)
