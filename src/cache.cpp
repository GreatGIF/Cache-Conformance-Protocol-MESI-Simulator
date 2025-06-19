#include "cache.hpp"
#include "memory.hpp"
#include "bus.hpp"
#include <iostream>
#include <iomanip>

Cache::Cache(int id) {
    processor_id = id;
    int blocks_num = 64 / block_size;
    int sets_num = blocks_num / 2;
    cache.resize(sets_num);
}

uint32_t Cache::handleBusRequest(BusRequest request, uint16_t address, int source_id, bool *shared) {
    if (source_id == processor_id) {return 0;}

    uint16_t index = (address >> 2) & 0x7;
    uint16_t tag = (address >> 5) & 0xFF;
    Set &set = cache[index];

    for (Block &block : set.blocks) {
        if (block.tag == tag) {
            switch (request) {
                case READ_MISS: {
                    if (block.state == SHARED || block.state == EXCLUSIVE) {
                        block.state = SHARED;
                        if (shared != nullptr) {
                            *shared = true;
                        }
                        return block.data;
                    } else if (block.state == MODIFIED) {
                        memory->writeOneBlock(address, block.data);
                        block.state = SHARED;
                        if (shared != nullptr) {
                            *shared = true;
                        }
                        return block.data;
                    }
                    break;
                }
                case WRITE_MISS: {
                    if (block.state == SHARED || block.state == EXCLUSIVE) {
                        block.state = INVALID;
                        return 0;
                    } else if (block.state == MODIFIED) {
                        memory->writeOneBlock(address, block.data);
                        block.state = INVALID;
                        return 0;
                    }
                    break;
                }
                case SET_INVALID: {
                    block.state = INVALID;
                    return 0;
                }
                default: break;
            }
        }
    }
    return 0;
}

bool Cache::access(uint16_t address, Operation op, uint16_t write_data, uint16_t* read_data) {
    access_count++;
    uint16_t offset = address & 0x3;
    uint16_t index = (address >> 2) & 0x7;
    uint16_t tag = (address >> 5) & 0xFF;

    Set &set = cache[index];
    bool hit = false;
    int block_index = -1;

    for (int i = 0; i < 2; i++) {
        if (set.blocks[i].state != INVALID && set.blocks[i].tag == tag) {
            hit = true;
            block_index = i;
            break;
        }
    }

    Block *block_ptr = nullptr;
    if (hit) {
        block_ptr = &set.blocks[block_index];
        block_ptr->lru_counter = access_count;
        if (op == READ) {
            if (block_ptr->state == SHARED || block_ptr->state == EXCLUSIVE || block_ptr->state == MODIFIED) {
                if (read_data != nullptr) {
                    *read_data = block_ptr->readTwoBytes(offset);
                }
            }
            read_hit_count++;
        } else {
            if (block_ptr->state == SHARED) {
                block_ptr->state = MODIFIED;
                block_ptr->writeTwoBytes(offset, write_data);
                bus->broadcast(SET_INVALID, address, processor_id);
            } else if (block_ptr->state == EXCLUSIVE || block_ptr->state == MODIFIED) {
                block_ptr->state = MODIFIED;
                block_ptr->writeTwoBytes(offset, write_data);
            }
            write_hit_count++;
        }
    } else {
        int lruTarget = 0;
        if (set.blocks[0].state == INVALID || set.blocks[1].state == INVALID) {
            lruTarget = (set.blocks[0].state == INVALID) ? 0 : 1;
        } else {
            lruTarget = (set.blocks[0].lru_counter < set.blocks[1].lru_counter) ? 0 : 1;
        }
        Block& victim = set.blocks[lruTarget];
        
        if (op == READ) {
            if (victim.state == MODIFIED) {
                memory->writeOneBlock(address, victim.data);
            }
            bool shared = false;
            uint32_t response_data = bus->broadcast(READ_MISS, address, processor_id, &shared);
            if (shared) {
                victim.state = SHARED;
                victim.tag = tag;
                victim.lru_counter = access_count;
                victim.data = response_data;
            } else {
                victim.state = EXCLUSIVE;
                victim.tag = tag;
                victim.lru_counter = access_count;
                victim.data = memory->readOneBlock(address);
            }
            if (read_data != nullptr) {
                *read_data = victim.readTwoBytes(offset);
            }
            read_miss_count++;
        } else {
            if (victim.state == MODIFIED) {
                memory->writeOneBlock(address, victim.data);
            }
            bus->broadcast(WRITE_MISS, address, processor_id);
            victim.state = MODIFIED;
            victim.tag = tag;
            victim.lru_counter = access_count;
            victim.data = memory->readOneBlock(address);
            victim.writeTwoBytes(offset, write_data);
            write_miss_count++;
        }
    }
    return hit;
}

void Cache::print_state() {
    std::cout << "Cache State (Processor " << processor_id << "):\n";
    for (int s = 0; s < cache.size(); s++) {
        std::cout << "Set " << s << ":\t";
        for (int b = 0; b < 2; b++) {
            Block &block = cache[s].blocks[b];
            if (block.state != INVALID) {
                // 将状态枚举转换为可读字符串
                std::string state_str;
                switch(block.state) {
                    case MODIFIED: state_str = "M"; break;
                    case EXCLUSIVE: state_str = "E"; break;
                    case SHARED: state_str = "S"; break;
                    default: state_str = "I";
                }
                
                // std::cout << "[T:0x" << std::hex << block.tag 
                //           << " S:" << state_str
                //           << " D:0x" << std::setw(8) << std::setfill('0') << block.data
                //           << " L:" << std::dec << block.lru_counter << "]\t";
                // D改为10进制
                std::cout << "[T:0x" << std::hex << block.tag 
                          << " S:" << state_str
                          << " D:" << std::dec << block.data
                          << " L:" << block.lru_counter << "]\t";
            } else {
                std::cout << "[INVALID]\t\t";
            }
        }
        std::cout << "\n";
    }
    
    // // 添加性能统计信息
    // std::cout << "Access Count: " << access_count << "\n";
    // std::cout << "Read Hits: " << read_hit_count 
    //           << ", Read Misses: " << read_miss_count << "\n";
    // std::cout << "Write Hits: " << write_hit_count 
    //           << ", Write Misses: " << write_miss_count << "\n";
    // std::cout << "Hit Rate: " 
    //           << (access_count ? (100.0 * (read_hit_count + write_hit_count) / access_count) : 0.0)
    //           << "%\n\n";
}