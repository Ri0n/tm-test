#include <algorithm>
#include <gtest/gtest.h>

#include "briefextractor.h"

TEST(extract, div)
{
    std::string data   = R"(
            <div>
              <div>Hello
              </div>
            </div>
          </div>
                       )";
    std::string expect = R"(<div>
              <div>Hello
              </div>
            </div>)";

    auto div = TM::BriefExtractor::extractDiv(data);
    ASSERT_EQ(div, expect);
}

TEST(extract, links_map)
{
    std::string data = R"(
            <div>
              <div><a href="http://hello/world?a&amp;b">The World got crazy</a>
              </div><a href="/test">Just a test</a>
            </div>
            <a
               href="https://dark/net"
            >Don't click &quot;this&quot; secret link
            </a>
          </div>
                       )";
    auto        m    = TM::BriefExtractor::links(data, "http://testhost/");

    auto findLink = [&](const std::string &link) {
        return std::find_if(m.cbegin(), m.cend(), [&](auto const &v) { return v.first == link; });
    };

    ASSERT_EQ(m.size(), 3);
    ASSERT_TRUE(findLink("http://hello/world?a&b") != m.end());
    ASSERT_TRUE(findLink("https://dark/net") != m.end());
    ASSERT_TRUE(findLink("http://testhost/test") != m.end());
    ASSERT_EQ(findLink("https://dark/net")->second, "Don't click \"this\" secret link");
}

TEST(extract, json_gen)
{
    TM::BriefExtractor::Links m    = { { "http://hello/world", "The World\ngot crazy" },
                                    { "https://dark/net", "Don't click this \"secret link\"" } };
    std::string               data = TM::BriefExtractor::linksToJson(m);
    ASSERT_EQ(data, R"({ news: [ {
title:"The World\ngot crazy",
link:"http://hello/world"
},
{
title:"Don't click this \"secret link\"",
link:"https://dark/net"
}]})");
}
