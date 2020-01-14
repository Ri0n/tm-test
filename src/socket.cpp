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

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "reactor.h"
#include "socket.h"

namespace TM {

struct Socket::Private {
    std::string      host;
    std::uint16_t    port;
    sockaddr_in      addr;
    Socket::Callback readyReadCB;
    Socket::Callback readyWriteCB;
    Socket::Callback connectedCB;
    Socket::Callback disconnectedCallback;

    bool resolveHost();
};

bool Socket::Private::resolveHost()
{
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = 0;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo *result;

    int s = getaddrinfo(host.c_str(), nullptr, &hints, &result);
    if (s != 0 || !result) {
        Log("dns resolve failed for ") << host;
        disconnectedCallback();
        return false;
    }

    // just take first from the linked list
    memcpy(&addr, result->ai_addr, result->ai_addrlen);
    addr.sin_port = htons(port);

    freeaddrinfo(result);
    return true;
}

Socket::Socket() : d(new Private) {}

Socket::~Socket() {}

void Socket::setConnectedCallback(Socket::Callback callback) { d->connectedCB = callback; }

void Socket::setDisconnectedCallback(Socket::Callback callback)
{
    d->disconnectedCallback = callback;
}

void Socket::setReadyReadCallback(Socket::Callback callback) { d->readyReadCB = callback; }

void Socket::setReadyWriteCallback(Socket::Callback callback) { d->readyWriteCB = callback; }

void Socket::connect(const std::string &host, std::uint16_t port)
{
    d->host = host;
    d->port = port;
    if (!d->resolveHost())
        return;

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        Log::syserr("failed to create socket");
        on_disconnect();
        return;
    }

    _reactor->addDevice(shared_from_this());

    if (::connect(fd, reinterpret_cast<sockaddr *>(&d->addr), sizeof(d->addr)) == -1) {
        Log::syserr("Error connecting to server.\n");
        on_disconnect();
        return;
    }
    on_connected();
}

void Socket::disconnect()
{
    if (fd != -1) {
        _reactor->removeDevice(shared_from_this());
        close(fd);
        fd = -1;
    }
}

void Socket::on_readyRead()
{
    if (d->readyReadCB) {
        d->readyReadCB();
    }
}

void Socket::on_readyWrite()
{
    if (d->readyWriteCB) {
        d->readyWriteCB();
    }
}

void Socket::on_connected()
{
    if (d->connectedCB)
        d->connectedCB();
}

void Socket::on_disconnect()
{
    disconnect();
    if (d->disconnectedCallback)
        d->disconnectedCallback();
}

} // namespace TM
