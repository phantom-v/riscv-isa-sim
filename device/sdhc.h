#ifndef _RISCV_DEVICES_EXTENSION_SDHC_H
#define _RISCV_DEVICES_EXTENSION_SDHC_H

#include "devices.h"

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
    bool handle_cmd();
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