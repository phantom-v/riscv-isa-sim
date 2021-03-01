#ifndef _RISCV_DEVICES_EXTENSION_UART_H
#define _RISCV_DEVICES_EXTENSION_UART_H

#include "devices.h"

class uart_t : public abstract_device_t {
   public:
    uart_t(plic_t* plic, bool diffTest, std::string file_path);
    bool load(reg_t addr, size_t len, uint8_t* bytes);
    bool store(reg_t addr, size_t len, const uint8_t* bytes);
    size_t size() { return UART_SIZE; }
    void check_int();

   private:
    bool diffTest;
    plic_t* plic;
    std::ifstream file_fifo;
    uint8_t uart_ier;
    uint8_t uart_isr;
    uint8_t uart_fcr;
    uint8_t uart_lcr;
    uint8_t uart_mcr;
    uint8_t uart_msr;
    uint8_t uart_spr;
    uint8_t uart_dll;
    uint8_t uart_dlm;
    uint8_t uart_psd;
};

#endif