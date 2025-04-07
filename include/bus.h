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

class AddressMappedDevice;

class Bus
{
private:
    struct Mapping
    {
        uint16_t start;
        uint16_t end;
        AddressMappedDevice *device;
    };

    std::vector<Mapping> mappings;
    std::vector<BusOperation> operations;
    std::vector<int> conflict_log;
public:
    Bus();
    void map_device(uint16_t start, uint16_t end, AddressMappedDevice *device);
    void start_cycle();
    uint8_t get(uint16_t addr);
    void set(uint16_t addr, uint8_t val);
    bool verify_operations(nlohmann::json json);
    void analyse_operations(nlohmann::json json);
};
