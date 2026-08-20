#ifndef RESP_SERIALIZER_HPP
#define RESP_SERIALIZER_HPP

#include "resp_parser.hpp"

class RespSerializer {
  public:
    static std::string serialize(const RespObject &object);
};

#endif
