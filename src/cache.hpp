#ifndef CACHE_HPP
#define CACHE_HPP

#include <vector>
#include <cstdint>
#include "common.hpp"

class Bus;
class Memory;

class Cache {
private:
    struct Block {
        State state;
        uint16_t tag;
        uint32_t lru_counter;
        uint32_t data;

        Block() : state(INVALID), tag(0), lru_counter(0), data(0) {}

        bool isDirty() const { 
            return state == MODIFIED; 
        }

        bool writeTwoBytes(int offset, uint16_t data) {
            if (offset < 0 || offset > 1) {
                return false;
            }
            int shift = offset * 16;
            this->data = (this->data & ~(0xFFFF << shift)) | (data << shift);
            return true;
        }

        uint16_t readTwoBytes(int offset) const {
            if (offset < 0 || offset > 1) {
                return 0;
            }
            int shift = offset * 16;
            return (this->data >> shift) & 0xFFFF;
        }
    };

    struct Set {
        Block blocks[2];
    };
    
    std::vector<Set> cache;
    const int block_size = 4;
    int access_count = 0;
    Bus *bus;
    Memory *memory;

public:
    int processor_id;
    int read_hit_count = 0;
    int read_miss_count = 0;
    int write_hit_count = 0;
    int write_miss_count = 0;

    Cache(int id);
    uint32_t handleBusRequest(BusRequest request, uint16_t address, int source_id, bool *shared = nullptr);
    bool access(uint16_t address, Operation op, uint16_t write_data = 0, uint16_t* read_data = nullptr);
    void print_state();
    void setBus(Bus *b) { bus = b; }
    void setMemory(Memory *m) { memory = m; }
    int getProcessorId() const { return processor_id; }
};

#endif