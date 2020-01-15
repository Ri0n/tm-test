#include <algorithm>
#include <regex>
#include <string>

#include "briefextractor.h"
#include "exception.h"
#include "strutil.h"

namespace TM {

std::string BriefExtractor::extractDiv(const std::string &data, std::size_t startPos)
{
    int  level = 1;
    auto idx   = startPos;
    while (level > 0) {
        idx = data.find("div", idx);
        if (idx == std::string::npos) {
            // check if we exited all internal divs
            return level == 1 ? data.substr(startPos) : std::string();
        }
        // make sure it's opening or closing tag
        bool isOpen = idx > 0 && data[idx - 1] == '<';
        if (!isOpen && (idx < 2 || !(data[idx - 2] == '<' && data[idx - 1] == '/'))) {
            // it's not closing either
            idx++;
            continue;
        }

        if (isOpen)
            // go to inside div
            level++;
        else
            level--;
        idx++;
    }
    auto ret = data.substr(0, idx - 3);
    str::trim(ret);
    return ret;
}

std::map<std::string, std::string> BriefExtractor::linksMap(const std::string &data)
{
    std::regex                         re(R"re(<a[\s]+href="([^"]*)"[\s]*>([^<]*)</a>)re");
    std::smatch                        sm;
    std::map<std::string, std::string> ret;
    std::string                        d = data;
    while (std::regex_search(d, sm, re)) {
        // TODO we have to also convert XML entities in both links and titles
        std::string title = sm[2];
        str::trim(title);
        ret.insert(std::make_pair(sm[1], title));
        d = sm.suffix();
    }
    return ret;
}

/**
 * @brief BriefExtractor::mapToJson converts a map of link=>title to json
 * @param links
 * @return json string representation
 */
std::string BriefExtractor::mapToJson(const std::map<std::string, std::string> &links)
{
    // it's way better to use any existing lib. nlohmann json for example is just perfect
    // but we avoid libs usage and the requirement was to generate invalid json (unquoted keys)
    // hence is our bicycle
    std::stringstream ret;
    for (auto const &[link, title] : links) {
        if (ret.rdbuf()->in_avail() > 0)
            ret << ",\n";
        ret << "{\n"
            << "title:\"" << str::jsonEscape(title) << "\",\n"
            << "link:\"" << str::jsonEscape(link) << "\"\n}";
    }
    return "{ news: [ " + ret.str() + "]}";
}

std::string BriefExtractor::extract(const std::string &html)
{
    auto idx = html.find(">The Brief<");
    if (idx == std::string::npos || (idx = html.find("</div>", idx)) == std::string::npos)
        throw NoValidBrief("\"The Brief\" not found");

    auto div = extractDiv(html, idx + 6);
    if (div.empty())
        throw NoValidBrief("invalid html for Brief");

    auto links = linksMap(div);
    return mapToJson(links);
}

} // namespace TM
