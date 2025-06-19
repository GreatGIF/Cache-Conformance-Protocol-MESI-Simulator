#include "request.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

Request::Request(int processor_id, Operation op, uint16_t address, uint16_t write_data) 
    : processor_id(processor_id), op(op), address(address), write_data(write_data) {}

std::string Request::toString() const {
    std::stringstream ss;
    ss << "<P" << processor_id << ", ";
    if (op == READ) {
        ss << "read";
    } else if (op == WRITE) {
        ss << "write";
    } else {
        ss << "barrier";
    }
    if (op != BARRIER) {
        ss << ", " << std::dec << address << ", ";
    } else {
        ss << ", -, ";
    }
    if (op == WRITE) {
        ss << write_data;
    } else {
        ss << "-";
    }
    ss << ">";
    return ss.str();
}

Request parseRequest(const std::string &line) {
    std::istringstream iss(line);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(iss, token, ',')) {
        size_t start = token.find_first_not_of(" <>");
        size_t end = token.find_last_not_of(" <>");
        if (start != std::string::npos && end != std::string::npos) {
            token = token.substr(start, end - start + 1);
        }
        tokens.push_back(token);
    }

    if (tokens.size() != 4) {
        std::cerr << "Invalid request format: " << line << std::endl;
        exit(1);
    }

    int processor_id = std::stoi(tokens[0].substr(1));
    if (processor_id < 0 || processor_id > 3) {
        std::cerr << "Invalid processor ID: " << processor_id << std::endl;
        exit(1);
    }

    Operation op;
    if (tokens[1] == "read") {
        op = READ;
    } else if (tokens[1] == "write") {
        op = WRITE;
    } else if (tokens[1] == "barrier") {
        op = BARRIER;
    } else {
        std::cerr << "Invalid operation: " << tokens[1] << std::endl;
        exit(1);
    }

    uint16_t address = tokens[2] == "-" ? UINT16_MAX : std::stoul(tokens[2], nullptr, 10); // 十进制解析
    uint16_t write_data = tokens[3] == "-" ? UINT16_MAX : std::stoul(tokens[3], nullptr, 10); // 十进制解析

    return Request(processor_id, op, address, write_data);
}