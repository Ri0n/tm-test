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

#include <deque>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "log.h"
#include "securesocket.h"

namespace TM {

static bool sslInitialized = false;

static void sslInit()
{
    if (sslInitialized)
        return;
    SSL_library_init();
    SSLeay_add_ssl_algorithms();
    SSL_load_error_strings();
    sslInitialized = true;
}

static void SSL_trace(int write_p, int version, int content_type, const void *buf, size_t len,
                      SSL *ssl, void *arg)
{
    std::ostringstream verstr;
    if (version >= TLS1_VERSION && version <= TLS_MAX_VERSION) {
        verstr << "tls" << char(version - TLS1_VERSION + '0');
    } else {
        verstr << "unknown_ver";
    }
    Log("SSL trace: ") << (write_p ? "sent " : "recv ") << verstr.str() << " ct=" << content_type;
}

static void logSsl()
{
    while (unsigned long err = ERR_get_error()) {
        char *str = ERR_error_string(err, nullptr);
        if (!str)
            break;
        Log("SSL: ") << str;
    }
}

struct SecureSocket::Private {
    SSL *                              ssl = nullptr;
    std::deque<std::vector<std::byte>> buffer;
};

SecureSocket::SecureSocket() : d(new Private) { sslInit(); }

SecureSocket::~SecureSocket()
{
    if (d->ssl)
        SSL_free(d->ssl);
}

void SecureSocket::on_connected()
{
    const SSL_METHOD *method = TLS_client_method();
    SSL_CTX *         ctx;

    if (!(method = TLS_client_method()) || !(ctx = SSL_CTX_new(method))
        || !(d->ssl = SSL_new(ctx))) {

        Log("SSL context init error");
        logSsl();
        on_disconnect();
        return;
    }
    SSL_set_tlsext_host_name(d->ssl, remoteHostname().c_str());

    SSL_set_msg_callback(d->ssl, SSL_trace);

    SSL_set_fd(d->ssl, fd);
    if (SSL_connect(d->ssl) <= 0) {
        Log("SSL connection failure");
        logSsl();
        on_disconnect();
        return;
    }
    Socket::on_connected();
}

std::size_t SecureSocket::writeData(const char *data, std::size_t size)
{
    int len = SSL_write(d->ssl, data, int(size));
    if (len < 0) {
        int err = SSL_get_error(d->ssl, len);
        if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ)
            return 0;
        return std::size_t(-1);
    }
    return std::size_t(len);
}

std::vector<std::byte> SecureSocket::readData(std::size_t size)
{
    std::vector<std::byte> buf(size);

    int len = SSL_read(d->ssl, buf.data(), int(size));
    if (len < 0) {
        int err = SSL_get_error(d->ssl, len);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
            return std::vector<std::byte>();
        Log("failed to read from secure socket");
        on_disconnect();
        return std::vector<std::byte>();
    }
    buf.resize(std::size_t(len));
    return buf;
}

} // namespace TM
