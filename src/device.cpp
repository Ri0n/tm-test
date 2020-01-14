/*
 * Copyright (c) 2020 Sergey Ilinykh <rion4ik@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of mosquitto nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <unistd.h>

#include "device.h"
#include "log.h"
#include "reactor.h"

namespace TM {

Device::Device() {}

Device::~Device()
{
    if (fd >= 0) {
        close(fd);
    }
}

void Device::setReactor(std::shared_ptr<Reactor> r) { _reactor = r; }

std::size_t Device::write(const std::string &data)
{
    return writeData(data.c_str(), data.length());
}
std::vector<std::byte> Device::read(std::size_t size) { return readData(size); }

std::vector<std::byte> Device::readData(std::size_t size)
{
    std::vector<std::byte> buf(size > ReadBufSz ? ReadBufSz : size);
    // FIXME figure out how many bytes available before allocating buffer
    auto realsize = ::read(fd, buf.data(), buf.size());
    if (realsize < 0) {
        Log::syserr("read failed");
        return std::vector<std::byte>();
    }
    buf.resize(std::size_t(realsize));
    return buf;
}

std::size_t Device::writeData(const char *data, std::size_t size)
{
    return std::size_t(::write(fd, data, size));
}

} // namespace TM
