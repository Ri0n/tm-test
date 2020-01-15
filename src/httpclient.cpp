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

#include <cstring>
#include <map>

#include "httpclient.h"
#include "log.h"
#include "securesocket.h"
#include "strutil.h"
#include "url.h"

namespace TM {

struct HttpClient::Private {
    std::shared_ptr<Reactor>            reactor;
    Url                                 url;
    std::function<void(std::string &&)> callback;
    std::shared_ptr<Socket>             socket;
    std::string                         contents;
    bool                                headersParsed  = false;
    uint8_t                             redirectsAvail = 5;
    int                                 status;
    size_t                              bytesToRead = 0;
    std::map<std::string, std::string>  headers;

    void tryParseHeaders()
    {
        auto idx = contents.find("\r\n\r\n");
        if (idx == std::string::npos)
            return;
        auto rawHeaders = contents.substr(0, idx + 2);
        contents        = contents.substr(idx + 4);

        Log("=== Response headers ===\n") << rawHeaders;

        headers.clear();
        std::istringstream stream(rawHeaders);
        std::string        line;
        std::getline(stream, line);
        idx = line.find(' ');
        if (idx == std::string::npos)
            throw std::invalid_argument(line);
        auto idx2 = line.find(' ', idx + 1);
        if (idx2 == std::string::npos)
            throw std::invalid_argument(line);
        status = std::atoi(&line[idx + 1]);
        if (status < 100 || status >= 600)
            throw std::invalid_argument(line);

        while (std::getline(stream, line)) {
            idx = line.find(':');
            if (idx == std::string::npos)
                throw std::invalid_argument(line);
            auto name  = line.substr(0, idx);
            auto value = line.substr(idx + 1);
            str::trim(name);
            str::trim(value);
            str::tolower(name);
            if (name.empty())
                throw std::invalid_argument(line);
            auto it = headers.find(name);
            if (it == headers.end())
                headers.emplace(std::make_pair(std::move(name), std::move(value)));
            else
                it->second += value;
        }
        auto clit = headers.find("content-length");
        if (clit != headers.end()) {
            bytesToRead = std::strtoul(clit->second.c_str(), nullptr, 10);
        }

        headersParsed = true;
    }

    void doRequest()
    {
        socket = url.scheme() == Url::Https ? std::make_shared<SecureSocket>()
                                            : std::make_shared<Socket>();
        socket->setReactor(reactor);

        socket->setConnectedCallback([this]() {
            std::ostringstream query;
            query << "GET " << (url.uri().empty() ? "/" : url.uri())
                  << " HTTP/1.1\r\n"
                     "Host: "
                  << url.host()
                  << "\r\n"
                     "Accept: text/*\r\n"
                     "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:72.0) Gecko/20100101 "
                     "Firefox/72.0\r\n"
                     "Connection: close\r\n\r\n";
            Log("=== Request ===\n") << query.str();
            socket->write(query.str());
        });

        socket->setReadyReadCallback([this]() {
            // we really have to parse response headers instead of size hardcoding. but no time for
            // this
            size_t toRead = headersParsed && bytesToRead ? bytesToRead - contents.size() : 16384;
            if (toRead > 16384)
                toRead = 16384;

            auto bytes = socket->read(toRead);
            if (bytes.size()) {
                std::string partContents(reinterpret_cast<const char *>(&bytes[0]), bytes.size());
                contents += std::move(partContents);
                if (!headersParsed) {
                    try {
                        tryParseHeaders();
                    } catch (std::invalid_argument &e) {
                        socket->disconnect();
                        Log("Failed to parse headers: ") << e.what();
                        callback("");
                        return;
                    }

                    // if we just parsed headers. let's check for reirections
                    if (headersParsed) {
                        if (handleRedirect())
                            return;
                    }
                }
                Log("content-size=") << contents.size() << " of " << bytesToRead;
                if (headersParsed && contents.size() == bytesToRead) {
                    std::string body;
                    std::swap(body, contents);
                    socket->disconnect();
                    callback(std::move(body));
                }
            }
        });

        socket->setDisconnectedCallback([this]() { callback(""); });

        socket->connect(url.host(), url.port());
    }

    bool handleRedirect()
    {
        if (--redirectsAvail == 0) {
            Log("Too many redirects");
            callback("");
            return false;
        }
        decltype(headers.begin()) it;
        if (status >= 300 && status < 400 && (it = headers.find("location")) != headers.end()) {
            Log("=== Handle redirect ===");
            try {
                url = it->second;
                socket->disconnect();
                headersParsed = false;
                contents.clear();
                bytesToRead = 0;
                doRequest();
                return true;
            } catch (std::exception &e) {
                Log("Redirect failed early: ") << e.what();
                callback("");
            }
        }
        return false;
    }
};

HttpClient::HttpClient(std::shared_ptr<Reactor> reactor, const std::string &url) :
    d(new Private { reactor, url })
{
}

HttpClient::~HttpClient() {}

void HttpClient::execute(std::function<void(std::string &&)> finishCallback)
{
    d->callback = finishCallback;
    d->doRequest();
}

} // namespace TM
