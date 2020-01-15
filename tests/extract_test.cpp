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
              <div><a href="http://hello/world">The World got crazy</a>
              </div><a href="/test">Just a test</a>
            </div>
            <a
               href="https://dark/net"
            >Don't click this secret link
            </a>
          </div>
                       )";
    auto        m    = TM::BriefExtractor::linksMap(data, "http://testhost/");
    ASSERT_EQ(m.size(), 3);
    ASSERT_TRUE(m.find("http://hello/world") != m.end());
    ASSERT_TRUE(m.find("https://dark/net") != m.end());
    ASSERT_TRUE(m.find("http://testhost/test") != m.end());
    ASSERT_EQ(m["https://dark/net"], "Don't click this secret link");
}

TEST(extract, json_gen)
{
    std::map<std::string, std::string> m
        = { { "http://hello/world", "The World\ngot crazy" },
            { "https://dark/net", "Don't click this \"secret link\"" } };
    std::string data = TM::BriefExtractor::mapToJson(m);
    ASSERT_EQ(data, R"({ news: [ {
title:"The World\ngot crazy",
link:"http://hello/world"
},
{
title:"Don't click this \"secret link\"",
link:"https://dark/net"
}]})");
}
