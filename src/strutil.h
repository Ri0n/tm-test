#include <algorithm>
#include <iomanip>
#include <string>

/*
 * Next utils were copied from StackOverflow.
 * This is rather a boilerplate code and doesn't worth reimplementation.
 */

namespace TM::str {

inline void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(),
            s.end());
}

inline void trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}

inline void tolower(std::string &s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
}

inline std::string jsonEscape(const std::string &s)
{
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"':
            o << "\\\"";
            break;
        case '\\':
            o << "\\\\";
            break;
        case '\b':
            o << "\\b";
            break;
        case '\f':
            o << "\\f";
            break;
        case '\n':
            o << "\\n";
            break;
        case '\r':
            o << "\\r";
            break;
        case '\t':
            o << "\\t";
            break;
        default:
            if ('\x00' <= *c && *c <= '\x1f') {
                o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(*c);
            } else {
                o << *c;
            }
        }
    }
    return o.str();
}

inline void htmlEntitiesDecode(std::string &s)
{
    static std::string subs[]
        = { "& #34;", "&quot;", "& #39;", "&apos;", "& #38;", "&amp;", "& #60;", "&lt;",
            "& #62;", "&gt;",   "&34;",   "&39;",   "&38;",   "&60;",  "&62;" };

    static std::string reps[]
        = { "\"", "\"", "'", "'", "&", "&", "<", "<", ">", ">", "\"", "'", "&", "<", ">" };

    size_t found;
    for (int j = 0; j < 15; j++) {
        do {
            found = s.find(subs[j]);
            if (found != std::string::npos)
                s.replace(found, subs[j].length(), reps[j]);
        } while (found != std::string::npos);
    }
}

} // namespace TM::str
