#include "bus.h"
#include "ram.h"

#include <string>
#include <iostream>
#include <iomanip>

Bus::Bus(RAM *ram) :
    ram{ram}, operations{}, conflict_log{}
{}

uint8_t Bus::get(uint16_t addr)
{
    conflict_log.back()++;
    uint8_t val = ram->get(addr);
    operations.emplace_back(BusOperation{addr, val, BusOperationType::READ});
    return val;
}

void Bus::start_cycle()
{
    conflict_log.push_back(0);
}

void Bus::set(uint16_t addr, uint8_t val)
{
    conflict_log.back()++;
    ram->set(addr, val);
    operations.emplace_back(BusOperation{addr, val, BusOperationType::WRITE});
}

int Bus::operation_count()
{
    return operations.size();
}

bool Bus::verify_operations(nlohmann::json json)
{
    for (size_t i = 0; i < operations.size(); ++i)
    {
        if (operations[i].addr != json[i][0] ||
            operations[i].val != json[i][1])
        {
            return false;
        }

        BusOperationType type = (json[i][2].get<std::string>() == "read")
            ? BusOperationType::READ : BusOperationType::WRITE;
        
        if (operations[i].type != type)
        {
            return false;
        }
    }

    for (int accesses : conflict_log)
    {
        if (accesses != 1)
        {
            return false;
        }
    }

    return true;
}

void Bus::analyse_operations(nlohmann::json json)
{
    if (verify_operations(json)) return;

    std::cerr << std::dec;
    if (operations.size() != json.size())
    {
        std::cerr << "Bus operation count mismatch: Expected "
            << json.size() << ", got " << operations.size() << std::endl;
    }

    for (size_t i = 0; i < operations.size(); ++i)
    {
        if (i < json.size())
        {
            BusOperationType type = (json[i][2].get<std::string>() == "read")
            ? BusOperationType::READ : BusOperationType::WRITE;
            
            std::cerr << "Expected: " << "[" << ((type == BusOperationType::READ) ? "read" : "write")
                << "] A=" << json[i][0] << " V=" << json[i][1] << "  ";
        }
        else
        {
            std::cerr << "Expected: [EMPTY] ";
        }
        
        std::cerr << "Observed: " << "[" << ((operations[i].type == BusOperationType::READ) ? "read" : "write")
            << "] A=" << operations[i].addr << " V=" << (int)operations[i].val << std::endl;
    }

    for (int i = 0; i < conflict_log.size(); ++i)
    {
        if (conflict_log[i] != 1)
        {
            std::cerr << "Bus Conflict on Cycle " << i << " with " << conflict_log[i] << " Operations" << std::endl;
        }
    }
}