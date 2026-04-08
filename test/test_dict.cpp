#include <gtest/gtest.h>
#include "core/dict.hpp"

// Test basic set and get
TEST(DictTest, BasicSetAndGet) {
    DICT dict;
    int val = 100;

    dict.set(SDS("name"), &val);
    EXPECT_EQ(dict.size(), 1);

    SDS key("name");
    EXPECT_EQ(dict.get(key), &val);
}

// Test get non-existent key
TEST(DictTest, GetNonExistent) {
    DICT dict;
    SDS key("name");

    EXPECT_EQ(dict.get(key), nullptr);
}

// Test update existing key
TEST(DictTest, UpdateExistingKey) {
    DICT dict;
    int val1 = 100, val2 = 200;

    dict.set(SDS("name"), &val1);
    dict.set(SDS("name"), &val2);

    SDS key("name");
    EXPECT_EQ(dict.size(), 1);
    EXPECT_EQ(dict.get(key), &val2);
}

// Test erase
TEST(DictTest, Erase) {
    DICT dict;
    int val = 100;

    dict.set(SDS("name"), &val);
    
    SDS key("name");
    EXPECT_TRUE(dict.erase(key));
    EXPECT_EQ(dict.size(), 0);
    EXPECT_EQ(dict.get(key), nullptr);
}

// Test erase non-existent key
TEST(DictTest, EraseNonExistent) {
    DICT dict;
    SDS key("name");

    EXPECT_FALSE(dict.erase(key));
}

// Test multiple keys
TEST(DictTest, MultipleKeys) {
    DICT dict;
    int v1 = 10, v2 = 20, v3 = 30;

    dict.set(SDS("a"), &v1);
    dict.set(SDS("b"), &v2);
    dict.set(SDS("c"), &v3);

    EXPECT_EQ(dict.size(), 3);
    EXPECT_EQ(dict.get(SDS("a")), &v1);
    EXPECT_EQ(dict.get(SDS("b")), &v2);
    EXPECT_EQ(dict.get(SDS("c")), &v3);
}

// Test rehash (triggered when load factor exceeds 0.75)
TEST(DictTest, Rehash) {
    DICT dict;
    int values[100];

    for (int i = 0; i < 100; ++i) {
        values[i] = i;
        dict.set(SDS(std::to_string(i).c_str()), &values[i]);
    }

    EXPECT_EQ(dict.size(), 100);

    for (int i = 0; i < 100; ++i) {
        SDS key(std::to_string(i).c_str());
        EXPECT_EQ(*(int*)dict.get(key), i);
    }
}

//Test move constructor
TEST(DictTest, MoveConstructor) {
    DICT dict1;
    int val = 100;
    dict1.set(SDS("name"), &val);

    DICT dict2(std::move(dict1));
    SDS key("name");
    EXPECT_EQ(dict2.size(), 1);
    EXPECT_EQ(dict2.get(key), &val);
}

// Test move assignment
TEST(DictTest, MoveAssignment) {
    DICT dict1;
    int val = 100;
    dict1.set(SDS("name"), &val);

    DICT dict2;
    dict2 = std::move(dict1);

    SDS key("name");
    EXPECT_EQ(dict2.size(), 1);
    EXPECT_EQ(dict2.get(key), &val);
}

// Test with different value types
TEST(DictTest, DifferentValueTypes) {
    DICT dict;
    int int_val = 42;
    double double_val = 3.14;
    char* str_val = const_cast<char*>("hello");

    dict.set(SDS("int_key"), &int_val);
    dict.set(SDS("double_key"), &double_val);
    dict.set(SDS("str_key"), str_val);

    EXPECT_EQ(*(int*)dict.get(SDS("int_key")), 42);
    EXPECT_DOUBLE_EQ(*(double*)dict.get(SDS("double_key")), 3.14);
    EXPECT_EQ((char*)dict.get(SDS("str_key")), str_val);
}

// Test erase and re-add
TEST(DictTest, EraseAndReAdd) {
    DICT dict;
    int val1 = 100, val2 = 200;

    dict.set(SDS("name"), &val1);
    
    SDS key("name");
    EXPECT_TRUE(dict.erase(key));
    EXPECT_EQ(dict.size(), 0);

    dict.set(SDS("name"), &val2);
    EXPECT_EQ(dict.size(), 1);
    EXPECT_EQ(dict.get(key), &val2);
}
