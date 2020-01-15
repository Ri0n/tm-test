#include <iostream>
#include <unistd.h>

#include "briefextractor.h"
#include "httpclient.h"
#include "log.h"
#include "reactor.h"

using namespace std;

int main(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "vh")) > 0)
        switch (opt) {
        case 'v':
            TM::Log::setEnabled(true);
            break;

        case 'h':
        default:
            std::cout << R"(
 -v  - enable verbose mode
 -h  - show this help
)";
            break;
        }

    auto reactor = TM::Reactor::factory("epoll");
    if (!reactor) {
        std::cerr << "failed to find epoll reactor\n";
        return -1;
    }

    bool        finished = false;
    std::string url      = "http://time.com";
    auto        client   = std::make_shared<TM::HttpClient>(reactor, url);
    client->execute([&](const std::string &data) {
        finished = true;
        reactor->stop();
        if (data.empty()) {
            std::cout << "got empty contents. try verbose (-v) mode\n" << std::flush;
        } else {
            try {
                std::cout << TM::BriefExtractor::extract(data, url) << "\n";
            } catch (std::exception &e) {
                std::cerr << "There was an error extracting brief: " << e.what() << "\n";
            }
        }
    });

    if (!finished)
        reactor->start();

    return 0;
}
