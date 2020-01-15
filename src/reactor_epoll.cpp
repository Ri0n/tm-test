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

#include <sstream>
#include <sys/epoll.h>
#include <unistd.h>

#include "exception.h"
#include "log.h"
#include "reactor_epoll.h"

namespace TM {

ReactorEpoll::ReactorEpoll()
{
    _epfd = epoll_create1(0);
    if (_epfd < 0) {
        throw ReactorException("Failed to init reactor");
    }
}

ReactorEpoll::~ReactorEpoll()
{
    if (_active)
        Log("Destroying active reactor. Something went terribly wrong.");

    close(_epfd);
}

void ReactorEpoll::start()
{
    _active = true;

    epoll_event events[MaxEvents];
    while (_active) {
        int n = epoll_wait(_epfd, events, MaxEvents, -1);
        if (n == -1) {
            Log::syserr("epoll_wait failed");
            break;
        }
        for (int i = 0; i < n; i++) {
            auto &ev = events[i];
            // auto  userData = reinterpret_cast<UserData *>(ev.data.ptr);
            // auto  dev      = userData->device.lock();
            auto devIt = _devices.find(ev.data.fd);
            if (devIt == _devices.end()) {
                epoll_ctl(_epfd, EPOLL_CTL_DEL, ev.data.fd, nullptr);
                continue;
            }
            auto dev = devIt->second;

            if (ev.events & EPOLLHUP || ev.events & EPOLLERR) {
                std::ostringstream ss;
                ss << "Got unexpected events " << ev.events;
                _active = false;
                throw ReactorException(ss.str());
            }

            if (ev.events & EPOLLIN)
                dev->on_readyRead();

            if (ev.events & EPOLLOUT)
                dev->on_readyWrite();
        }
    }
    _active = false;
}

void ReactorEpoll::stop() { _active = false; }

void ReactorEpoll::addDevice(std::shared_ptr<Device> dev)
{
    auto fd = dev->fileDescriptor();
    if (fd == -1)
        throw ReactorException("Device is not open");
    _devices.insert(std::make_pair(fd, dev));

    epoll_event ev;
    ev.data.fd = fd;
    ev.events  = EPOLLIN | EPOLLOUT;
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        Log::syserr("Failed to add fd to epoll") << " fd=" << fd;
    }
}

void ReactorEpoll::removeDevice(std::shared_ptr<Device> dev)
{
    auto fd = dev->fileDescriptor();
    epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr);
    _devices.erase(fd);
}

} // namespace TM
