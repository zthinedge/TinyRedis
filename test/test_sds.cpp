#include <gtest/gtest.h>
#include "core/sds.hpp"

// Test default constructor
TEST(SDSTest, DefaultConstructor) {
    SDS s;
    EXPECT_EQ(s.len(), 0);
    EXPECT_STREQ(s.c_str(), "");
}

// Test constructor with C-string
TEST(SDSTest, CStringConstructor) {
    SDS s("hello");
    EXPECT_EQ(s.len(), 5);
    EXPECT_STREQ(s.c_str(), "hello");
}

// Test constructor with string_view
TEST(SDSTest, StringViewConstructor) {
    std::string_view sv("world");
    SDS s(sv);
    EXPECT_EQ(s.len(), 5);
    EXPECT_STREQ(s.c_str(), "world");
}

// Test append functionality
TEST(SDSTest, Append) {
    SDS s("hello");
    s.append(" world", 6);
    
    EXPECT_STREQ(s.c_str(), "hello world");
    EXPECT_EQ(s.len(), 11);
}

// Test multiple appends
TEST(SDSTest, MultipleAppends) {
    SDS s("hello");
    s.append(" world", 6);
    s.append(" redis", 6);
    
    EXPECT_STREQ(s.c_str(), "hello world redis");
    EXPECT_EQ(s.len(), 17);
}

// Test capacity
TEST(SDSTest, Capacity) {
    SDS s("hello");
    EXPECT_GT(s.capacity(), 0);
    EXPECT_GE(s.capacity(), s.len());
}

// Test available space
TEST(SDSTest, Available) {
    SDS s("hello");
    size_t avail = s.avail();
    size_t cap = s.capacity();
    size_t len = s.len();
    
    EXPECT_EQ(avail, cap - len);
}

// Test clear
TEST(SDSTest, Clear) {
    SDS s("hello");
    s.clear();
    
    EXPECT_EQ(s.len(), 0);
    EXPECT_STREQ(s.c_str(), "");
}

// Test move constructor
TEST(SDSTest, MoveConstructor) {
    SDS s1("hello");
    size_t len = s1.len();
    
    SDS s2(std::move(s1));
    EXPECT_EQ(s2.len(), len);
    EXPECT_STREQ(s2.c_str(), "hello");
}

// Test move assignment
TEST(SDSTest, MoveAssignment) {
    SDS s1("hello");
    size_t len = s1.len();
    
    SDS s2("world");
    s2 = std::move(s1);
    
    EXPECT_EQ(s2.len(), len);
    EXPECT_STREQ(s2.c_str(), "hello");
}

// Test append with string_view overload
TEST(SDSTest, AppendStringView) {
    SDS s("hello");
    s.append(std::string_view(" world", 6));
    
    EXPECT_STREQ(s.c_str(), "hello world");
    EXPECT_EQ(s.len(), 11);
}
