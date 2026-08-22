#include "../../include/server/command_dispatcher.hpp"
#include "../../include/protocol/resp_serializer.hpp"
#include <algorithm>
#include <stdexcept>
#include <limits>

CommandDispatcher::CommandDispatcher() {}

std::string CommandDispatcher::to_upper(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::optional<std::string> CommandDispatcher::get_bulk_string_arg(const RespObject &arg, RespObject &error_response) {
    if (arg.type != RespType::BULK_STRING) {
        error_response = {RespType::ERROR, std::string("ERR argument must be a bulk string")};
        return std::nullopt;
    }
    return std::get<std::string>(arg.value);
}

std::string CommandDispatcher::execute(const RespObject &request) {
    if (request.type != RespType::ARRAY)
        return RespSerializer::serialize({RespType::ERROR, std::string("ERR invalid request type")});
    const auto &args = std::get<std::vector<RespObject>>(request.value);
    if (args.empty() || args[0].type != RespType::BULK_STRING)
        return RespSerializer::serialize({RespType::ERROR, std::string("ERR invalid command format")});
    std::string command = to_upper(std::get<std::string>(args[0].value));
    if (command == "GET")
        return RespSerializer::serialize(handle_get(args));
    if (command == "SET")
        return RespSerializer::serialize(handle_set(args));
    if (command == "DEL")
        return RespSerializer::serialize(handle_del(args));
    if (command == "EXISTS")
        return RespSerializer::serialize(handle_exists(args));
    if (command == "INCR")
        return RespSerializer::serialize(handle_incr(args));
    if (command == "DECR")
        return RespSerializer::serialize(handle_decr(args));
    return RespSerializer::serialize({RespType::ERROR, std::string("ERR unknown command type")});
}

RespObject CommandDispatcher::handle_get(const std::vector<RespObject> &args) {
    if (args.size() != 2)
        return {RespType::ERROR, std::string("ERR wrong number of arguments for GET command")};
    if (args[1].type != RespType::BULK_STRING)
        return {RespType::ERROR, std::string("ERR key must be a bulk string")};
    const std::string &key = std::get<std::string>(args[1].value);
    StorageObject *object = db.get(key);
    if (!object)
        return {RespType::NULL_BULK_STRING, std::string("")};
    if (object->type != StorageDataType::STRING)
        return {RespType::ERROR, std::string("ERR wrongtype operation against a key holding a wrong kind of value")};
    return {RespType::BULK_STRING, std::get<std::string>(object->value)};
}

RespObject CommandDispatcher::handle_set(const std::vector<RespObject> &args) {
    if (args.size() != 3)
        return {RespType::ERROR, std::string("ERR wrong number of arguments for SET command")};
    RespObject error_response{RespType::ERROR, std::string("")};
    auto key = get_bulk_string_arg(args[1], error_response);
    if (!key) return error_response;
    auto value = get_bulk_string_arg(args[2], error_response);
    if (!value) return error_response;
    db.set(*key, {StorageDataType::STRING, *value});
    return {RespType::SIMPLE_STRING, std::string("OK")};
}

RespObject CommandDispatcher::handle_del(const std::vector<RespObject> &args) {
    if (args.size() < 2)
        return {RespType::ERROR, std::string("ERR wrong number of arguments for DEL command")};
    int64_t count = 0;
    RespObject error_response{RespType::ERROR, std::string("")};
    for (size_t i = 1; i < args.size(); i++) {
        auto key = get_bulk_string_arg(args[i], error_response);
        if (!key) return error_response;
        if (db.del(*key))
            count++;
    }
    return {RespType::INTEGER, count};
}

RespObject CommandDispatcher::handle_exists(const std::vector<RespObject> &args) {
    if (args.size() < 2)
        return {RespType::ERROR, std::string("ERR wrong number of arguments for EXISTS command")};
    int64_t count = 0;
    RespObject error_response{RespType::ERROR, std::string("")};
    for (size_t i = 1; i < args.size(); i++) {
        auto key = get_bulk_string_arg(args[i], error_response);
        if (!key) return error_response;
        if (db.get(*key) != nullptr)
            count++;
    }
    return {RespType::INTEGER, count};
}

std::optional<int64_t> CommandDispatcher::get_integer_value(const std::string &key, RespObject &error_response) {
    StorageObject *object = db.get(key);
    if (!object)
        return 0;
    if (object->type != StorageDataType::STRING) {
        error_response = {RespType::ERROR, std::string("ERR wrongtype operation against a key holding a wrong kind of value")};
        return std::nullopt;
    }
    const std::string &value_str = std::get<std::string>(object->value);
    try {
        size_t position;
        int64_t value = std::stoll(value_str, &position);
        if (position != value_str.length())
            throw std::invalid_argument("");
        return value;
    } catch (const std::exception &) {
        error_response = {RespType::ERROR, std::string("ERR value is not an integer or out of range")};
        return std::nullopt;
    }
}

RespObject CommandDispatcher::handle_incr(const std::vector<RespObject> &args) {
    if (args.size() != 2)
        return {RespType::ERROR, std::string("ERR wrong number of arguments for INCR command")};
    if (args[1].type != RespType::BULK_STRING)
        return {RespType::ERROR, std::string("ERR key must be a bulk string")};

    const std::string &key = std::get<std::string>(args[1].value);
    RespObject error_response{RespType::ERROR, std::string("")};
    auto current = get_integer_value(key, error_response);
    if (!current)
        return error_response;

    if (*current == std::numeric_limits<int64_t>::max())
        return {RespType::ERROR, std::string("ERR increment will overflow")};

    int64_t value = *current + 1;
    db.set(key, {StorageDataType::STRING, std::to_string(value)});
    return {RespType::INTEGER, value};
}

RespObject CommandDispatcher::handle_decr(const std::vector<RespObject> &args) {
    if (args.size() != 2)
        return {RespType::ERROR, std::string("ERR wrong number of arguments for DECR command")};
    if (args[1].type != RespType::BULK_STRING)
        return {RespType::ERROR, std::string("ERR key must be a bulk string")};

    const std::string &key = std::get<std::string>(args[1].value);
    RespObject error_response{RespType::ERROR, std::string("")};
    auto current = get_integer_value(key, error_response);
    if (!current)
        return error_response;

    if (*current == std::numeric_limits<int64_t>::min())
        return {RespType::ERROR, std::string("ERR decrement will overflow")};

    int64_t value = *current - 1;
    db.set(key, {StorageDataType::STRING, std::to_string(value)});
    return {RespType::INTEGER, value};
}