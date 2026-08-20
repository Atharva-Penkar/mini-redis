#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include "../include/protocol/resp_parser.hpp"
#include "../include/protocol/resp_serializer.hpp"
#include "../include/storage/dict.hpp"
#include "../include/server/command_dispatcher.hpp"

#define ASSERT(condition) \
    if (!(condition)) { \
        std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
        std::exit(1); \
    }

void test_parse_simple_string() {
    RespParser parser;
    auto obj = parser.parse("+OK\r\n");
    ASSERT(obj.has_value());
    ASSERT(obj->type == RespType::SIMPLE_STRING);
    ASSERT(std::get<std::string>(obj->value) == "OK");
}

void test_serialize_simple_string() {
    RespObject obj{RespType::SIMPLE_STRING, std::string("OK")};
    ASSERT(RespSerializer::serialize(obj) == "+OK\r\n");
}

void test_parse_integer() {
    RespParser parser;
    auto obj = parser.parse(":1000\r\n");
    ASSERT(obj.has_value());
    ASSERT(obj->type == RespType::INTEGER);
    ASSERT(std::get<int64_t>(obj->value) == 1000);
}

void test_parse_bulk_string() {
    RespParser parser;
    auto obj = parser.parse("$5\r\nhello\r\n");
    ASSERT(obj.has_value());
    ASSERT(obj->type == RespType::BULK_STRING);
    ASSERT(std::get<std::string>(obj->value) == "hello");
}

void test_parse_null_bulk_string() {
    RespParser parser;
    auto obj = parser.parse("$-1\r\n");
    ASSERT(obj.has_value());
    ASSERT(obj->type == RespType::NULL_BULK_STRING);
}

void test_parse_array() {
    RespParser parser;
    auto obj = parser.parse("*2\r\n$4\r\nECHO\r\n$5\r\nhello\r\n");
    ASSERT(obj.has_value());
    ASSERT(obj->type == RespType::ARRAY);
    auto arr = std::get<std::vector<RespObject>>(obj->value);
    ASSERT(arr.size() == 2);
    ASSERT(std::get<std::string>(arr[0].value) == "ECHO");
    ASSERT(std::get<std::string>(arr[1].value) == "hello");
}

void test_dict_set_get() {
    Dict db;
    db.set("key1", {StorageDataType::STRING, std::string("value1")});
    auto val = db.get("key1");
    ASSERT(val != nullptr);
    ASSERT(val->type == StorageDataType::STRING);
    ASSERT(std::get<std::string>(val->value) == "value1");
}

void test_dict_delete() {
    Dict db;
    db.set("key1", {StorageDataType::STRING, std::string("value1")});
    ASSERT(db.del("key1") == true);
    ASSERT(db.get("key1") == nullptr);
    ASSERT(db.del("key1") == false);
}

void test_dict_incremental_rehashing() {
    Dict db;
    for (int i = 0; i < 100; ++i) {
        db.set("key" + std::to_string(i), {StorageDataType::STRING, std::to_string(i)});
    }
    for (int i = 0; i < 100; ++i) {
        auto val = db.get("key" + std::to_string(i));
        ASSERT(val != nullptr);
        ASSERT(std::get<std::string>(val->value) == std::to_string(i));
    }
}

void test_dispatcher_set_get() {
    CommandDispatcher dispatcher;
    RespObject set_cmd{RespType::ARRAY, std::vector<RespObject>{
        {RespType::BULK_STRING, std::string("SET")},
        {RespType::BULK_STRING, std::string("mykey")},
        {RespType::BULK_STRING, std::string("myval")}
    }};
    ASSERT(dispatcher.execute(set_cmd) == "+OK\r\n");
    
    RespObject get_cmd{RespType::ARRAY, std::vector<RespObject>{
        {RespType::BULK_STRING, std::string("GET")},
        {RespType::BULK_STRING, std::string("mykey")}
    }};
    ASSERT(dispatcher.execute(get_cmd) == "$5\r\nmyval\r\n");
}

void test_dispatcher_del() {
    CommandDispatcher dispatcher;
    RespObject set_cmd{RespType::ARRAY, std::vector<RespObject>{
        {RespType::BULK_STRING, std::string("SET")},
        {RespType::BULK_STRING, std::string("key1")},
        {RespType::BULK_STRING, std::string("val1")}
    }};
    dispatcher.execute(set_cmd);

    RespObject del_cmd{RespType::ARRAY, std::vector<RespObject>{
        {RespType::BULK_STRING, std::string("DEL")},
        {RespType::BULK_STRING, std::string("key1")},
        {RespType::BULK_STRING, std::string("key2")}
    }};
    ASSERT(dispatcher.execute(del_cmd) == ":1\r\n");
}

int main() {
    test_parse_simple_string();
    test_serialize_simple_string();
    test_parse_integer();
    test_parse_bulk_string();
    test_parse_null_bulk_string();
    test_parse_array();
    test_dict_set_get();
    test_dict_delete();
    test_dict_incremental_rehashing();
    test_dispatcher_set_get();
    test_dispatcher_del();
    std::cout << "All tests passed successfully.\n";
    return 0;
}