#ifndef COMMAND_DISPATCHER_HPP
#define COMMAND_DISPATCHER_HPP

#include "../../include/protocol/resp_parser.hpp"
#include "../../include/storage/dict.hpp"
#include <string>
#include <vector>

class CommandDispatcher {
  public:
    CommandDispatcher();
    std::string execute(const RespObject &request);

  private:
    Dict db;
    std::string to_upper(const std::string &str);
    std::optional<std::string> get_bulk_string_arg(const RespObject &arg, RespObject &error_response);
    RespObject handle_get(const std::vector<RespObject> &args);
    RespObject handle_set(const std::vector<RespObject> &args);
    RespObject handle_del(const std::vector<RespObject> &args);
    RespObject handle_exists(const std::vector<RespObject> &args);
    std::optional<int64_t> get_integer_value(const std::string &key, RespObject &error_response);
    RespObject handle_incr(const std::vector<RespObject> &args);
    RespObject handle_decr(const std::vector<RespObject> &args);
};

#endif
