#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <vector>
#include <cstdint>

class Memory {
private:
    std::vector<uint32_t> data;
    const int size_KB = 8;
    const int block_size = 4;
    
public:
    Memory();
    uint32_t readOneBlock(uint16_t address);
    void writeOneBlock(uint16_t address, uint32_t value);
    void printState(int start = 0, int end = 10);
};

#endif