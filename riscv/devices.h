#ifndef _RISCV_DEVICES_H
#define _RISCV_DEVICES_H

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "decode.h"
#include "mmio_plugin.h"

class processor_t;

class abstract_device_t {
   public:
    virtual bool load(reg_t addr, size_t len, uint8_t* bytes) = 0;
    virtual bool store(reg_t addr, size_t len, const uint8_t* bytes) = 0;
    virtual ~abstract_device_t() {}
};

class bus_t : public abstract_device_t {
   public:
    bool load(reg_t addr, size_t len, uint8_t* bytes);
    bool store(reg_t addr, size_t len, const uint8_t* bytes);
    void add_device(reg_t addr, abstract_device_t* dev);

    std::pair<reg_t, abstract_device_t*> find_device(reg_t addr);

   private:
    std::map<reg_t, abstract_device_t*> devices;
};

class rom_device_t : public abstract_device_t {
   public:
    rom_device_t(std::vector<char> data);
    bool load(reg_t addr, size_t len, uint8_t* bytes);
    bool store(reg_t addr, size_t len, const uint8_t* bytes);
    const std::vector<char>& contents() { return data; }

   private:
    std::vector<char> data;
};

class mem_t : public abstract_device_t {
   public:
    mem_t(size_t size) : len(size) {
        if (!size)
            throw std::runtime_error("zero bytes of target memory requested");
        data = (char*)calloc(1, size);
        if (!data)
            throw std::runtime_error("couldn't allocate " + std::to_string(size) + " bytes of target memory");
    }
    mem_t(const mem_t& that) = delete;
    ~mem_t() { free(data); }

    bool load(reg_t addr, size_t len, uint8_t* bytes) { return false; }
    bool store(reg_t addr, size_t len, const uint8_t* bytes) { return false; }
    char* contents() { return data; }
    size_t size() { return len; }

   private:
    char* data;
    size_t len;
};

class clint_t : public abstract_device_t {
   public:
    clint_t(std::vector<processor_t*>&, uint64_t freq_hz, bool real_time);
    bool load(reg_t addr, size_t len, uint8_t* bytes);
    bool store(reg_t addr, size_t len, const uint8_t* bytes);
    size_t size() { return CLINT_SIZE; }
    void increment(reg_t inc);
    uint64_t get_mtime() { return mtime; }

   private:
    typedef uint64_t mtime_t;
    typedef uint64_t mtimecmp_t;
    typedef uint32_t msip_t;
    std::vector<processor_t*>& procs;
    uint64_t freq_hz;
    bool real_time;
    uint64_t real_time_ref_secs;
    uint64_t real_time_ref_usecs;
    mtime_t mtime;
    std::vector<mtimecmp_t> mtimecmp;
};

#ifdef ZJV_DEVICE_EXTENSTION
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

class sdhc_t : public abstract_device_t {
   public:
    sdhc_t(plic_t* plic, std::string image_path);
    ~sdhc_t();
    bool load(reg_t addr, size_t len, uint8_t* bytes);
    bool store(reg_t addr, size_t len, const uint8_t* bytes);
    size_t size() { return SDHC_SIZE; }
    void check_int();
    void reset();

   private:
    plic_t* plic;

    reg_t sdcard_cur;
    reg_t sdcard_cap;
    std::fstream sdcard;

    #define SDHC_BLKLEN 512
    unsigned short bufptr;
    unsigned char  buffer[SDHC_BLKLEN + 2];

    unsigned int sdhc_sysaddr;            // 0x00
    unsigned short int sdhc_blksize;      // 0x04
    unsigned short int sdhc_blkcount;     // 0x06
    unsigned int sdhc_arg;                // 0x08
    unsigned short int sdhc_trnmode;      // 0x0c
    unsigned short int sdhc_cmd;          // 0x0e
    unsigned int sdhc_rsp[4];             // 0x10/14/18/1c
    unsigned int sdhc_bufdata;            // 0x20
    unsigned int sdhc_prnstate;           // 0x24
    unsigned char sdhc_hostctl1;          // 0x28
    unsigned char sdhc_pwrctl;            // 0x29
    unsigned char sdhc_bgapctl;           // 0x2a
    unsigned char sdhc_wkupctl;           // 0x2b
    unsigned short int sdhc_clockctl;     // 0x2c
    unsigned char sdhc_tmoutctl;          // 0x2e
    unsigned char sdhc_sftrst;            // 0x2f
    unsigned short int sdhc_norintsts;    // 0x30
    unsigned short int sdhc_errintsts;    // 0x32
    unsigned short int sdhc_norintstsen;  // 0x34
    unsigned short int sdhc_errintstsen;  // 0x36
    unsigned short int sdhc_norintsigen;  // 0x38
    unsigned short int sdhc_errintsigen;  // 0x3a
    unsigned short int sdhc_acmderrsts;   // 0x3c
    unsigned short int sdhc_hostctl2;     // 0x3e
    unsigned long int sdhc_capab;         // 0x40
    unsigned long int sdhc_maxcurcap;     // 0x48
    unsigned short int sdhc_version;      // 0xfe
};

#endif

class mmio_plugin_device_t : public abstract_device_t {
   public:
    mmio_plugin_device_t(const std::string& name, const std::string& args);
    virtual ~mmio_plugin_device_t() override;

    virtual bool load(reg_t addr, size_t len, uint8_t* bytes) override;
    virtual bool store(reg_t addr, size_t len, const uint8_t* bytes) override;

   private:
    mmio_plugin_t plugin;
    void* user_data;
};

#endif
