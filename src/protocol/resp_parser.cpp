#include "include/protocol/resp_parser.hpp"
#include <charconv>

size_t RespParser::get_bytes_consumed() const {
    return position;
}

void RespParser::reset() {
    position = 0;
}

std::optional<std::string> RespParser::read_until_crlf(const std::string &buffer) {
    size_t crlf_position = buffer.find("\r\n", position);
    if(crlf_position == std::string::npos)
        return std::nullopt;
    std::string result = buffer.substr(position, crlf_position - position);
    position = crlf_position + 2;
    return result;
}

std::optional<RespObject> RespParser::parse(const std::string &buffer) {
    if(position >= buffer.length())
        return std::nullopt;
    char type_char = buffer[position++];
    switch(type_char){
        case '+': return parse_simple_string(buffer);
        case '-': return parse_error(buffer);
        case ':': return parse_integer(buffer);
        case '$': return parse_bulk_string(buffer);
        case '*': return parse_array(buffer);
        default: return std::nullopt;
    }
}

std::optional<RespObject> RespParser::parse_simple_string(const std::string &buffer) {
    auto result = read_until_crlf(buffer);
    if(!result)
        return std::nullopt;
    return RespObject{RespType::SIMPLE_STRING, *result};
}

std::optional<RespObject> RespParser::parse_error(const std::string &buffer) {
    auto result = read_until_crlf(buffer);
    if(!result)
        return std::nullopt;
    return RespObject{RespType::ERROR, *result};
}
