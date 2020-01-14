#include <gtest/gtest.h>

#include "url.h"

TEST(url, parse_good)
{
    TM::Url u("http://hello:123/world");
    ASSERT_EQ(u.scheme(), TM::Url::Http);
    ASSERT_EQ(u.host(), "hello");
    ASSERT_EQ(u.port(), 123);
    ASSERT_EQ(u.uri(), "/world");
}

TEST(url, parse_good_just_host)
{
    TM::Url u("https://hello");
    ASSERT_EQ(u.scheme(), TM::Url::Https);
    ASSERT_EQ(u.host(), "hello");
    ASSERT_EQ(u.port(), 443);
    ASSERT_EQ(u.uri(), "");
}

TEST(url, parse_bad) { ASSERT_THROW(TM::Url("htps://hello"), std::invalid_argument); }
