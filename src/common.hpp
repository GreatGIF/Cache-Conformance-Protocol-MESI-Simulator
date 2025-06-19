#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>

enum State {
    MODIFIED,       // 修改态
    EXCLUSIVE,      // 独占态
    SHARED,         // 共享态
    INVALID         // 无效态
};

enum Operation {
    READ,
    WRITE,
    BARRIER         // 新增 barrier 操作
};

enum BusRequest {
    READ_MISS,      // 读缺失
    WRITE_MISS,     // 写缺失
    SET_INVALID     // 无效
};

#define PUBLIC_SUM_ADDR 0x400

#endif