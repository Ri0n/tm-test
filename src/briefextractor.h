#ifndef BRIEFEXTRACTOR_H
#define BRIEFEXTRACTOR_H

#include <map>
#include <string>
#include <vector>

namespace TM {

class BriefExtractor {
public:
    using Links = std::vector<std::pair<std::string, std::string>>;

    static std::string extractDiv(const std::string &data, std::size_t startPos = 0);
    static Links       links(const std::string &data, const std::string &base_url);
    static std::string linksToJson(const Links &links);
    static std::string extract(const std::string &html, const std::string &base_url);
};

} // namespace TM

#endif // BRIEFEXTRACTOR_H
