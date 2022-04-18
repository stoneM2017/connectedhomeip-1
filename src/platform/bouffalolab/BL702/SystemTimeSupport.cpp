/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2018 Nest Labs, Inc.
 *    All rights reserved.
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
 *          Provides implementations of the CHIP System Layer platform
 *          time/clock functions that are suitable for use on the BL702 platform.
 */
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <lib/support/logging/CHIPLogging.h>
#include <platform/bouffalolab/BL702/SystemTimeSupport.h>

#include <sys/time.h>
extern "C" {
    #include <bl_timer.h>
}

namespace chip {
namespace System {
namespace Clock {

namespace Internal {
ClockImpl gClockImpl;
}
Microseconds64 systemTimeOffset = Clock::Microseconds64((static_cast<uint64_t>(CHIP_SYSTEM_CONFIG_VALID_REAL_TIME_THRESHOLD) * UINT64_C(1000000)));

Microseconds64 ClockImpl::GetMonotonicMicroseconds64(void)
{
    return Clock::Microseconds64((uint64_t)bl_timer_get_overflow_cnt() << 32 | bl_timer_get_current_time());
}

Milliseconds64 ClockImpl::GetMonotonicMilliseconds64(void)
{
    return Clock::Milliseconds64(GetMonotonicMicroseconds64().count() / UINT64_C(1000));
}

// CHIP_ERROR ClockImpl::GetClock_RealTime(Clock::Microseconds64 & aCurTime)
// {
//     aCurTime = Clock::Microseconds64(systemTimeOffset.count()) + GetMonotonicMicroseconds64();
//     return CHIP_NO_ERROR;
// }

// CHIP_ERROR ClockImpl::GetClock_RealTimeMS(Clock::Milliseconds64 & aCurTime)
// {
//     aCurTime = Clock::Milliseconds64(systemTimeOffset.count() / UINT64_C(1000)) + GetMonotonicMilliseconds64();
//     return CHIP_NO_ERROR;
// }

// CHIP_ERROR ClockImpl::SetClock_RealTime(Clock::Microseconds64 aNewCurTime)
// {
//     systemTimeOffset = aNewCurTime - GetMonotonicMicroseconds64();
//     return CHIP_NO_ERROR;
// }


CHIP_ERROR ClockImpl::GetClock_RealTime(Clock::Microseconds64 & aCurTime)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR ClockImpl::GetClock_RealTimeMS(Clock::Milliseconds64 & aCurTime)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}

CHIP_ERROR ClockImpl::SetClock_RealTime(Clock::Microseconds64 aNewCurTime)
{
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
}


// CHIP_ERROR InitClock_RealTime()
// {
//     systemTimeOffset = Clock::Microseconds64((static_cast<uint64_t>(CHIP_SYSTEM_CONFIG_VALID_REAL_TIME_THRESHOLD) * UINT64_C(1000000)));
//     return System::SystemClock().SetClock_RealTime(systemTimeOffset);
// }

} // namespace Clock
} // namespace System
} // namespace chip
