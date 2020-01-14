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

#ifndef DEVICE_H
#define DEVICE_H

#include <cstddef>
#include <memory>
#include <vector>

namespace TM {

class Reactor;

class Device : public std::enable_shared_from_this<Device> {
public:
    static const int ReadBufSz = 4096;

    Device();
    virtual ~Device();

    void                   setReactor(std::shared_ptr<Reactor>);
    int                    fileDescriptor() const { return fd; }
    std::vector<std::byte> read(std::size_t size);
    std::size_t            write(const std::string &data);

    virtual void on_readyRead()  = 0;
    virtual void on_readyWrite() = 0;

protected:
    virtual std::vector<std::byte> readData(std::size_t size = 0);
    virtual std::size_t            writeData(const char *data, std::size_t size);

protected:
    int                      fd = -1;
    std::shared_ptr<Reactor> _reactor;
};

} // namespace TM

#endif // DEVICE_H
