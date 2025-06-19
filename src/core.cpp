#include "core.hpp"
#include <iostream>

uint16_t public_sum = 0;

Core::Core(int id) : processor_id(id), cache(nullptr), barrier_flag(false) {}

void Core::executeRequest(Request &request, bool omp, bool reduction) {
    if (request.op == BARRIER) {
        barrier_flag = true;
        std::cout << "\nProcessing " << request.toString() 
                  << " (Set barrier for P" << processor_id << ")" << std::endl;
        cache->print_state();
    } else {
        if (barrier_flag) {
            request_queue.push(request);
            std::cout << "\nQueued " << request.toString() 
                      << " (Barrier active for P" << processor_id << ")" << std::endl;
        } else {
            if (request.op == READ) {
                uint16_t read_data = 0;
                cache->access(request.address, request.op, 0, &read_data);
                if (omp) {
                    private_sum = read_data;
                }
            } else {
                if (omp) {
                    if (reduction && request.address == PUBLIC_SUM_ADDR) {
                        public_sum += private_sum;
                        request.write_data = public_sum;
                    } else {
                        request.write_data = private_sum + i;
                        i++;
                    }
                }
                cache->access(request.address, request.op, request.write_data);
            }
            std::cout << "\nProcessing " << request.toString() 
                      << " (Priority: " << processor_id 
                      << ", Type: " << (request.op == WRITE ? "WRITE" : "READ") << ")" << std::endl;
            cache->print_state();
        }
    }

}

void Core:: enqueueRequest(const Request &request) {
    request_queue.push(request);
}

Request Core::dequeueRequest() {
    Request req = request_queue.front();
    request_queue.pop();
    return req;
}