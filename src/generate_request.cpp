#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <queue>
#include <algorithm>
#include <chrono> // 添加时间头文件
#include "request.hpp"
#include "generate_request.hpp"
#include "common.hpp"

// 全局随机数生成器，使用时间作为种子
std::mt19937 gen(static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
std::uniform_int_distribution<> dis;

// 生成指定范围的随机整数
int generateRandomInt(int min, int max) {
    dis.param(std::uniform_int_distribution<>::param_type(min, max));
    return dis(gen);
}

// 从给定的 vector 中随机抽取指定数量的元素
std::vector<int> getRandomElements(const std::vector<int>& vec, int num) {
    if (num <= 0) {
        std::cerr << "Number of elements to extract must be positive." << std::endl;
        return {};
    }
    if (num > vec.size()) {
        std::cerr << "Number of elements to extract exceeds the size of the vector." << std::endl;
        return {};
    }

    // 打乱然后返回指定数量的元素
    std::vector<int> tmp = vec;
    std::shuffle(tmp.begin(), tmp.end(), gen);
    return std::vector<int>(tmp.begin(), tmp.begin() + num);
}

void generateOmpRequest(const std::string& filename, bool reduction) {
    std::vector<std::queue<Request>> requests(4);
    if (reduction) {
        for (int i = 0; i < 64; i++) {
            int processor_id = i / 16;
            requests[processor_id].push(Request(processor_id, READ, processor_id * 0x100, 0));
            requests[processor_id].push(Request(processor_id, WRITE, processor_id * 0x100, 0));
        }
        for (int i = 0; i < 4; i++) {
            requests[i].push(Request(i, BARRIER, 0, 0));
        }
        for (int i = 0; i < 4; i++) {
            requests[0].push(Request(0, READ, i * 0x100, 0));
            requests[0].push(Request(0, WRITE, PUBLIC_SUM_ADDR, 0));
        }
    } else {
        for (int i = 0; i < 64; i++) {
            int processor_id = i / 16;
            requests[processor_id].push(Request(processor_id, READ, PUBLIC_SUM_ADDR, 0));
            requests[processor_id].push(Request(processor_id, WRITE, PUBLIC_SUM_ADDR, 0));
        }
    }
    std::ofstream file(filename, std::ios::out | std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Failed to open or create file: " << filename << std::endl;
        return;
    }

    int ins_count = 0;
    std::vector<int> non_empty_cores = {0, 1, 2, 3};
    while (!non_empty_cores.empty()) {
        int num_active = generateRandomInt(1, non_empty_cores.size());
        ins_count += num_active;
        std::vector<int> active_cores = getRandomElements(non_empty_cores, num_active);
        std::vector<std::string> line_requests(4, "NULL");
        for (int core : active_cores) {
            if (!requests[core].empty()) {
                line_requests[core] = requests[core].front().toString();
                requests[core].pop();
            }
        }

        for (auto it = non_empty_cores.begin(); it != non_empty_cores.end(); ) {
            if (requests[*it].empty()) {
                it = non_empty_cores.erase(it);
            } else {
                ++it;
            }
        }

        for (int i = 0; i < 4; i++) {
            file << line_requests[i];
            if (i < 3) file << ";";
            file << "\t";
        }
        file << "\n";
    }

    file.close();
    std::cout << "There are " << ins_count << " instructions" << std::endl;
    std::cout << "Generated " << filename << " successfully" << std::endl;
}
