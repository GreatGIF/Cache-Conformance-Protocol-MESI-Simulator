#include "bus.hpp"
#include "core.hpp"
#include "cache.hpp"
#include <iostream>
#include <algorithm>
#include <stdexcept>

Bus::Bus(std::vector<Core *> &cores, Memory *memory, const std::vector<int> &initial_priorities) {
    this->cores = cores;
    this->memory = memory;
    if (initial_priorities.size() != cores.size()) {
        throw std::invalid_argument("Priority list size must match number of cores");
    }
    this->priorities = initial_priorities;
    for (Core *core : cores) {
        core->prioritiy = &priorities[core->getProcessorId()];
    }
}

uint32_t Bus::broadcast(BusRequest request, uint16_t address, int source_id, bool *shared) {
    uint32_t response_data = 0;
    for (Core* core : cores) {
        if (core->getProcessorId() != source_id) {
            response_data = core->getCache()->handleBusRequest(request, address, source_id, shared);
            if (shared != nullptr && *shared && request == READ_MISS) {
                break;
            }
        }
    }
    return response_data;
}

bool Bus::allBarriersSet() const {
    for (const Core* core : cores) {
        if (!core->getBarrierFlag()) {
            return false;
        }
    }
    return true;
}

void Bus::arbitrate(const std::vector<Request> &requests, bool omp, bool reduction) {
    std::vector<Request> requests_tmp = requests;
    std::vector<Request> sorted_requests;
    // 将请求加入对应处理器的请求队列
    for (Request &request : requests_tmp) {
        Core *core = cores[request.processor_id];
        core->enqueueRequest(request);
        // 如果当前处理器已设置了barrier, 则打印
        if (core->getBarrierFlag() || core->getQueueSize() > 1) {
            std::cout << "\nEnqueued " << request.toString() 
                      << " (Priority: " << priorities[request.processor_id] 
                      << ", Type: " << (request.op == BARRIER ? "BARRIER" : 
                                        request.op == WRITE ? "WRITE" : "READ") << ")" << std::endl;
        }
    }
    // 遍历所有处理器，获取未设置barrier且队列不为空的请求
    for (Core* core : cores) {
        if (!core->getBarrierFlag() && !core->isQueueEmpty()) {
            sorted_requests.push_back(core->dequeueRequest());
        }
    }
    // 按 barrier > write > read 和处理器优先级排序新请求
    std::sort(sorted_requests.begin(), sorted_requests.end(), 
              [this](const Request &a, const Request &b) {
                  if (a.op != b.op) {
                      if (a.op == BARRIER) return true;
                      if (b.op == BARRIER) return false;
                      return a.op == WRITE;
                  }
                  return priorities[a.processor_id] < priorities[b.processor_id];
              });

    // 遍历排序后的请求
    for (Request &request : sorted_requests) {
        Core *core = cores[request.processor_id];
        // if (!core->getBarrierFlag() && core->isQueueEmpty()) {
        //     // 未设置 barrier 且队列为空，直接执行
        //     std::cout << "\nExecuting directly " << request.toString() 
        //               << " (Priority: " << priorities[request.processor_id] 
        //               << ", Type: " << (request.op == BARRIER ? "BARRIER" : 
        //                                 request.op == WRITE ? "WRITE" : "READ") << ")" << std::endl;
        //     core->executeRequest(request);
        // } else {
        //     // 否则压入队列
        //     core->enqueueRequest(request);
        //     std::cout << "\nEnqueued " << request.toString() 
        //               << " (Priority: " << priorities[request.processor_id] 
        //               << ", Type: " << (request.op == BARRIER ? "BARRIER" : 
        //                                 request.op == WRITE ? "WRITE" : "READ") << ")" << std::endl;
        //     // 如果未设置 barrier，执行队列中的第一条请求
        //     if (!core->getBarrierFlag() && !core->isQueueEmpty()) {
        //         Request req = core->dequeueRequest();
        //         core->executeRequest(req);
        //     }
        // }
        core->executeRequest(request, omp, reduction);
    }

    // 检查所有核心的 barrier 状态
    bool all_barriers = allBarriersSet();
    if (all_barriers) {
        std::cout << "\nAll barriers set, clearing barriers\n";
        for (Core* core : cores) {
            core->clearBarrier();
        }
    }
}

void Bus::setPriorities(const std::vector<int> &new_priorities) {
    if (new_priorities.size() != cores.size()) {
        throw std::invalid_argument("New priority list size must match number of cores");
    }
    priorities = new_priorities;
}