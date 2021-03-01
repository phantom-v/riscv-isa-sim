#ifndef _RISCV_DEVICES_EXTENSION_PLIC_H
#define _RISCV_DEVICES_EXTENSION_PLIC_H

#include "devices.h"

class plic_t : public abstract_device_t {
   public:
    plic_t(std::vector<processor_t*>& procs, size_t num_source, size_t num_context);
    bool load(reg_t addr, size_t len, uint8_t* bytes);
    bool store(reg_t addr, size_t len, const uint8_t* bytes);
    size_t size() { return PLIC_SIZE; }

    uint32_t plic_claim(uint32_t contextid);
    void plic_update();
    bool plic_int_check(uint32_t contextid);
    void plic_irq(uint32_t irq, bool level);

   private:
    size_t num_source;
    size_t num_context;
    typedef uint32_t plic_reg_t;
    std::vector<processor_t*>& procs;
    std::vector<plic_reg_t> priority;
    std::vector<std::vector<plic_reg_t> > ie;
    std::vector<plic_reg_t> ip;
    std::vector<plic_reg_t> threshold;
    std::vector<std::vector<plic_reg_t> > claimed;

    typedef struct {
        uint32_t hartid;
        char mode;
    } context_t;
    std::vector<context_t> context;
};

#endif