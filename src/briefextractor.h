#ifndef BRIEFEXTRACTOR_H
#define BRIEFEXTRACTOR_H

#include <map>
#include <string>

namespace TM {

class BriefExtractor
{
public:
    static std::string extractDiv(const std::string &data, std::size_t startPos = 0);
    static std::map<std::string, std::string> linksMap(const std::string &data);
    static std::string mapToJson(const std::map<std::string, std::string> &links);
    static std::string extract(const std::string &html);
};

} // namespace TM

#endif // BRIEFEXTRACTOR_H
