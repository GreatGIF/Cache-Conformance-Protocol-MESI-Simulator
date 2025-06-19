#ifndef REQUEST_PARSER_HPP
#define REQUEST_PARSER_HPP

#include <string>
#include <vector>
#include "common.hpp"

struct Request {
    int processor_id;
    Operation op;
    uint16_t address;
    uint16_t write_data;

    Request(int processor_id, Operation op, uint16_t address = 0, uint16_t write_data = 0);
    std::string toString() const;
};

Request parseRequest(const std::string &line);

#endif