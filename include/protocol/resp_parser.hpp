#ifndef RESP_PARSER_HPP
#define RESP_PARSER_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

enum class RespType { SIMPLE_STRING, ERROR, INTEGER, BULK_STRING, ARRAY, NULL_BULK_STRING };

enum class StorageDataType {
    STRING,
    LIST,
    HASH,
    SET,
    ZSET,
    BITMAP,
    STREAM,
};

enum class CommandType {
    SET,
    GET,
    DEL,
    EXISTS,
    EXPIRE,
    TTL,
    INCR,
    DECR,
    LPUSH,
    RPUSH,
    LPOP,
    RPOP,
    LLEN,
    LRANGE,
    HSET,
    HGET,
    HDEL,
    HEXISTS,
    HKEYS,
    HGETALL,
    SADD,
    SREM,
    SMEMBERS,
    ZADD,
    ZREM,
    ZRANGE,
    SETBIT,
    GETBIT,
    XADD,
    XREAD,
    UNKNOWN
};

struct RespObject {
    RespType type;
    std::variant<std::string, int64_t, std::vector<RespObject>> value;
};

class RespParser {
  public:
    std::optional<RespObject> parse(const std::string &buffer);
    size_t get_bytes_consumed() const;
    void reset();

  private:
    size_t position = 0;
    /* parsers */
    std::optional<RespObject> parse_simple_string(const std::string &buffer);
    std::optional<RespObject> parse_error(const std::string &buffer);
    std::optional<RespObject> parse_integer(const std::string &buffer);
    std::optional<RespObject> parse_bulk_string(const std::string &buffer);
    std::optional<RespObject> parse_array(const std::string &buffer);
    /* read sequence of bytes till carriage return line feed */
    std::optional<std::string> read_until_crlf(const std::string &buffer);
};

#endif
