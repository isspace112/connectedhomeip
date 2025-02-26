/*
 *
 *    Copyright (c) 2020-2021 Project CHIP Authors
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
 *      This file declares the abstraction of one-directional, anonymous
 *      data stream built on top of two file descriptors.
 */

#include <system/SystemConfig.h>

#if CHIP_SYSTEM_CONFIG_USE_SOCKETS

#include <system/WakeEvent.h>

// Include additional CHIP headers
#include <support/CodeUtils.h>
#include <system/SystemError.h>

// Include system and language headers
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#if !CHIP_SYSTEM_CONFIG_USE_POSIX_PIPE
#include <sys/eventfd.h>
#endif

namespace chip {
namespace System {

#if CHIP_SYSTEM_CONFIG_USE_POSIX_PIPE

namespace {
inline int SetNonBlockingMode(int fd)
{
    int flags = ::fcntl(fd, F_GETFL, 0);
    return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
} // anonymous namespace

CHIP_ERROR WakeEvent::Open(WatchableEventManager & watchState)
{
    enum
    {
        FD_READ  = 0,
        FD_WRITE = 1
    };
    int fds[2];

    if (::pipe(fds) < 0)
        return chip::System::MapErrorPOSIX(errno);

    if (SetNonBlockingMode(fds[FD_READ]) < 0)
        return chip::System::MapErrorPOSIX(errno);

    if (SetNonBlockingMode(fds[FD_WRITE]) < 0)
        return chip::System::MapErrorPOSIX(errno);

    mWriteFD = fds[FD_WRITE];

    mFD.Init(watchState);
    ReturnErrorOnFailure(mFD.Attach(fds[FD_READ]));
    mFD.SetCallback(Confirm, reinterpret_cast<intptr_t>(this));
    ReturnErrorOnFailure(mFD.RequestCallbackOnPendingRead());

    return CHIP_NO_ERROR;
}

void WakeEvent::Close()
{
    mFD.Close();
    VerifyOrDie(::close(mWriteFD) == 0);
    mWriteFD = -1;
}

void WakeEvent::Confirm()
{
    uint8_t buffer[128];
    ssize_t res;

    do
    {
        res = ::read(mFD.GetFD(), buffer, sizeof(buffer));
        if (res < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            ChipLogError(chipSystemLayer, "System wake event confirm failed: %s", ErrorStr(chip::System::MapErrorPOSIX(errno)));
            return;
        }
    } while (res == sizeof(buffer));
}

CHIP_ERROR WakeEvent::Notify()
{
    char byte = 1;

    if (::write(mWriteFD, &byte, 1) < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        return chip::System::MapErrorPOSIX(errno);
    }

    return CHIP_NO_ERROR;
}

#else // CHIP_SYSTEM_CONFIG_USE_POSIX_PIPE

CHIP_ERROR WakeEvent::Open(WatchableEventManager & watchState)
{
    mFD.Init(watchState);

    const int fd = ::eventfd(0, 0);
    if (fd == -1)
    {
        return chip::System::MapErrorPOSIX(errno);
    }

    mFD.Attach(fd);
    mFD.SetCallback(Confirm, reinterpret_cast<intptr_t>(this));
    mFD.RequestCallbackOnPendingRead();

    return CHIP_NO_ERROR;
}

void WakeEvent::Close()
{
    mFD.Close();
}

void WakeEvent::Confirm()
{
    uint64_t value;

    if (::read(mFD.GetFD(), &value, sizeof(value)) < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        ChipLogError(chipSystemLayer, "System wake event confirm failed: %s", ErrorStr(chip::System::MapErrorPOSIX(errno)));
    }
}

CHIP_ERROR WakeEvent::Notify()
{
    uint64_t value = 1;

    if (::write(mFD.GetFD(), &value, sizeof(value)) < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        return chip::System::MapErrorPOSIX(errno);
    }

    return CHIP_NO_ERROR;
}

#endif // CHIP_SYSTEM_CONFIG_USE_POSIX_PIPE

} // namespace System
} // namespace chip

#endif // CHIP_SYSTEM_CONFIG_USE_SOCKETS
