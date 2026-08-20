#include "../../include/protocol/resp_serializer.hpp"

std::string RespSerializer::serialize(const RespObject &object) {
    switch (object.type) {
    case RespType::SIMPLE_STRING:
        return "+" + std::get<std::string>(object.value) + "\r\n";
    case RespType::ERROR:
        return "-" + std::get<std::string>(object.value) + "\r\n";
    case RespType::INTEGER:
        return ":" + std::to_string(std::get<std::int64_t>(object.value)) + "\r\n";
    case RespType::NULL_BULK_STRING:
        return "$-1\r\n";
    case RespType::BULK_STRING: {
        const std::string &str = std::get<std::string>(object.value);
        return "$" + std::to_string(str.length()) + "\r\n" + str + "\r\n";
    }
    case RespType::ARRAY: {
        const auto &array = std::get<std::vector<RespObject>>(object.value);
        std::string result = "*" + std::to_string(array.size()) + "\r\n";
        for (const auto &element : array)
            result += serialize(element);
        return result;
    }
    }
    return "-ERR internal serialization error\r\n";
}
