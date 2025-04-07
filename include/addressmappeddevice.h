#pragma once

#include <cstdint>

class AddressMappedDevice
{
public:
    virtual uint8_t get(uint16_t addr) = 0;
    virtual void set(uint16_t addr, uint8_t val) = 0;
    virtual ~AddressMappedDevice() = default;
};