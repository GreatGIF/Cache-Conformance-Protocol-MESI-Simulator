#include "memory.hpp"
#include <iostream>
#include <iomanip>

Memory::Memory() {
    int blocks_num = (size_KB * 1024) / block_size;
    data.resize(blocks_num, 0);
}

uint32_t Memory::readOneBlock(uint16_t address) {
    uint16_t block_addr = address >> 2;
    if (block_addr < data.size()) {
        return data[block_addr];
    }
    return 0;
}

void Memory::writeOneBlock(uint16_t address, uint32_t value) {
    uint16_t block_addr = address >> 2;
    if (block_addr < data.size()) {
        data[block_addr] = value;
    }
}

void Memory::printState(int start, int end) {
    std::cout << "Memory State (first " << end << " blocks):\n";
    for (int i = start; i < end && i < data.size(); i++) {
        std::cout << "Block 0x" << std::hex << (i << 2) 
                  << ": 0x" << std::setw(8) << std::setfill('0') << data[i] << "\n";
    }
}