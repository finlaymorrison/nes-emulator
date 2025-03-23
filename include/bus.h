#pragma once

#include "nlohmann/json.hpp"

#include <vector>

#include <cstdint>

enum BusOperationType
{
    READ,
    WRITE
};

struct BusOperation
{
    uint16_t addr;
    uint8_t val;
    BusOperationType type;
};

class RAM;

class Bus
{
private:
    RAM *ram;
    std::vector<BusOperation> operations;
    std::vector<int> conflict_log;
public:
    Bus(RAM *ram);
    int operation_count();
    void start_cycle();
    uint8_t get(uint16_t addr);
    void set(uint16_t addr, uint8_t val);
    bool verify_operations(nlohmann::json json);
    void analyse_operations(nlohmann::json json);
};
