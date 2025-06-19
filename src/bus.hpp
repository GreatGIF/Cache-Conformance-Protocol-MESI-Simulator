#ifndef BUS_HPP
#define BUS_HPP

#include <vector>
#include "common.hpp"
#include "request.hpp"

class Core;
class Memory;

class Bus {
private:
    std::vector<Core *> cores;
    Memory *memory;
    std::vector<int> priorities;

public:
    Bus(std::vector<Core *> &cores, Memory *memory, const std::vector<int> &initial_priorities = {0, 1, 2, 3});
    uint32_t broadcast(BusRequest request, uint16_t address, int source_id, bool *shared = nullptr);
    void arbitrate(const std::vector<Request> &requests, bool omp = false, bool reduction = false);
    void setPriorities(const std::vector<int> &new_priorities);
    bool allBarriersSet() const;
};

#endif