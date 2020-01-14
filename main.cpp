#include <iostream>

#include "httpclient.h"
#include "log.h"
#include "reactor.h"

using namespace std;

std::string_view extractBrief(std::string_view data)
{
    int         level    = 1;
    std::size_t idxOpen  = 0;
    std::size_t idxClose = 0;
    while (level > 0) {
        auto idx      = std::min(idxOpen, idxClose);
        auto idxOpen  = data.find("<div", idx);
        auto idxClose = data.find("</div>", idx);
        // the above can be optimized quite a bit but it will triple the code size

        if (idxOpen == std::string::npos || idxClose == std::string::npos)
            return std::string_view(); // bull shit
        if (idxOpen < idxClose) {
            // go to inside div
            level++;
            idxOpen++;
        } else {
            level--;
            idxClose++;
        }
    }
    return data.substr(0, idxClose);
}

std::string briefToJson(std::string_view data) {}

int main()
{
    auto reactor = TM::Reactor::factory("epoll");
    if (!reactor) {
        TM::Log("failed to find epoll reactor");
        return -1;
    }

    auto client = std::make_shared<TM::HttpClient>(reactor, "http://time.com");
    client->execute([&](const std::string &data) {
        reactor->stop();
        if (data.empty()) {
            std::cout << "got empty contents. try verbose (-v) mode\n";
        } else {
            auto idx = data.find(">The Brief<");
            if (idx == std::string::npos || (idx = data.find("</div>", idx)) == std::string::npos) {
                std::cout << "Unable to find Brief. try verbose (-v) mode\n";
                return;
            }

            auto view  = std::string_view(data).substr(idx + 6);
            auto brief = extractBrief(view);
            if (brief.empty()) {
                std::cout << "Unable to extract Brief. try verbose (-v) mode\n";
            } else {
                std::cout << briefToJson(brief);
            }
        }
    });

    reactor->start();

    return 0;
}
