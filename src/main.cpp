#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include "cache.hpp"
#include "memory.hpp"
#include "bus.hpp"
#include "request.hpp"
#include "core.hpp"
#include "generate_request.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 4) {
        std::cerr << "Usage: ./sim [-omp] [-r] filename" << std::endl;
        return 1;
    }

    bool omp_flag = false;
    bool reduction_flag = false;
    std::string filename;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-omp") {
            omp_flag = true;
        } else if (arg == "-r") {
            reduction_flag = true;
        } else {
            filename = arg;
        }
    }

    if (filename.empty()) {
        std::cerr << "Error: No filename provided." << std::endl;
        return 1;
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "File does not exist. Creating and writing to " << filename << std::endl;
        generateOmpRequest(filename, reduction_flag);
        file.open(filename);
    }

    std::vector<Core *> cores;
    for (int i = 0; i < 4; i++) {
        Core *core = new Core(i);
        Cache *cache = new Cache(i);
        core->setCache(cache);
        cores.push_back(core);
    }

    std::vector<int> initial_priorities = {0, 1, 2, 3};
    Memory memory;
    Bus bus(cores, &memory, initial_priorities);
    for (Core* core : cores) {
        core->getCache()->setBus(&bus);
        core->getCache()->setMemory(&memory);
    }

    std::string line;
    int cycle = 0;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string request_str;
        std::vector<Request> requests;

        while (std::getline(iss, request_str, ';')) {
            request_str.erase(0, request_str.find_first_not_of(" \t"));
            request_str.erase(request_str.find_last_not_of(" \t") + 1);
            if (request_str != "NULL") {
                Request request = parseRequest(request_str);
                requests.push_back(request);
            }
        }
        std::cout << "\n----------Cycle " << cycle++ << "----------\n" << std::endl;
        if (!requests.empty()) {
            bus.arbitrate(requests, omp_flag, reduction_flag);
        }
    }

    bool queue_empty = false;
    while (!queue_empty) {
        queue_empty = true;
        for (Core *core : cores) {
            if (!core->isQueueEmpty()) {
                queue_empty = false;
                break;
            }
        }
        if (!queue_empty) {
            std::cout << "\n----------Cycle " << cycle++ << "----------\n" << std::endl;
            bus.arbitrate(std::vector<Request>(), omp_flag, reduction_flag);
        }
    }

    file.close();
    for (Core* core : cores) {
        delete core->getCache();
        delete core;
    }

    // generateRequestWithReduction("reduction.txt");

    return 0;
}