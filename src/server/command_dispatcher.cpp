#include "../../include/server/command_dispatcher.hpp"
#include "../../include/protocol/resp_serializer.hpp"
#include <algorithm>

CommandDispatcher::CommandDispatcher() {}

std::string CommandDispatcher::to_upper(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
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
        return {RespType::BULK_STRING, std::string("WRONGTYPE Operation against a key holding a wrong kind of value")};
    return {RespType::BULK_STRING, std::get<std::string>(object->value)};
}

RespObject CommandDispatcher::handle_set(const std::vector<RespObject> &args) {
    if (args.size() != 3)
        return {RespType::ERROR, std::string("ERR wrong number of arguments for SET command")};
    std::string key = std::get<std::string>(args[1].value);
    std::string value = std::get<std::string>(args[2].value);
    db.set(key, {StorageDataType::STRING, value});
    return {RespType::SIMPLE_STRING, std::string("OK")};
}

RespObject CommandDispatcher::handle_del(const std::vector<RespObject> &args) {
    if (args.size() < 2)
        return {RespType::ERROR, std::string("ERR wrong number of arguments for DEL command")};
    int64_t count = 0;
    for (size_t i = 1; i < args.size(); i++) {
        std::string key = std::get<std::string>(args[i].value);
        if (db.del(key))
            count++;
    }
    return {RespType::INTEGER, count};
}

RespObject CommandDispatcher::handle_exists(const std::vector<RespObject> &args) {
    if(args.size() < 2)
        return {RespType::ERROR, std::string("ERR wrong number of arguments for EXISTS command")};
    int64_t count = 0;
    for(size_t i = 1; i < args.size(); i++) {
        std::string key = std::get<std::string>(args[i].value);
        if(db.get(key) != nullptr)
            count++;
    }
    return {RespType::INTEGER, count};
}

