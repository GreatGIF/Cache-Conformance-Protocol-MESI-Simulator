#ifndef CORE_HPP
#define CORE_HPP

#include <vector>
#include <queue>
#include "common.hpp"
#include "cache.hpp"
#include "request.hpp"

class Core {
private:
    int processor_id;
    Cache *cache;
    bool barrier_flag;
    std::queue<Request> request_queue;

public:
    int *prioritiy;
    int i = getProcessorId() * 16;
    uint16_t private_sum;
    const int private_sum_addr = getProcessorId() * 0x100;
    Core(int id);
    void setCache(Cache *c) { cache = c; }
    Cache *getCache() const { return cache; }
    int getProcessorId() const { return processor_id; }
    bool getBarrierFlag() const { return barrier_flag; }
    bool isQueueEmpty() const { return request_queue.empty(); }
    int getQueueSize() const { return request_queue.size(); }
    void executeRequest(Request &request, bool omp = false, bool reduction = false);
    void enqueueRequest(const Request &request);
    Request dequeueRequest();
    void clearBarrier() { barrier_flag = false; }
};

#endif