#include "devices.h"
#include "processor.h"

// Based on samsung s3c6410 soc

sdhc_t::sdhc_t(plic_t* plic, std::string image_path) : plic(plic) {
    sdcard.open(image_path);
    if (sdcard.is_open()) {
        sdcard_cur = 0;
        sdcard_cap = sdcard.seekg(0, std::ios::end).tellg();
        printf("[SimSDHC] mount a SD card %s [%fKB]\n", image_path.c_str(), sdcard_cap/1024.0); 
    }

    reset();
}

sdhc_t::~sdhc_t() {
    sdcard.close();
    if (!sdcard.is_open()) {
        printf("[SimSDHC] unmount a SD card\n");
    }
}

void sdhc_t::reset() {
    bufptr = 0;

    sdhc_trnmode = 0;

    sdhc_rsp[0] = 0;
    sdhc_rsp[1] = 0;
    sdhc_rsp[2] = 0;
    sdhc_rsp[3] = 0;

    // Card state always stable and enable write
    sdhc_prnstate = 0x000a0000;

    // Don't support DMA, High Speed Mode, Suspend/Resume
    // Suport 1.8/33.V, maximum block size is 512 byte
    sdhc_capab = 0x05000080;
    sdhc_maxcurcap = 0;

    // support SDHC spec 2.0
    sdhc_version = 0x0401; 
}

void sdhc_t::check_int() {

}

// register map
#define SDHC_SYSADDR     0x000
#define SDHC_BLKSIZE     0x004
#define SDHC_BLKCOUNT    0x006
#define SDHC_ARG         0x008
#define SDHC_TRNMODE     0x00c
#define SDHC_CMD         0x00e
#define SDHC_RSP0        0x010
#define SDHC_RSP1        0x014
#define SDHC_RSP2        0x018
#define SDHC_RSP3        0x01c
#define SDHC_BUFDATA     0x020
#define SDHC_PRNSTATE    0x024
#define SDHC_HOSTCTL1    0x028
#define SDHC_PWRCTL      0x029
#define SDHC_CLOCKCTL    0x02c
#define SDHC_TMOUTCTL    0x02e
#define SDHC_SFTRST      0x02f
#define SDHC_NORINTSTS   0x030
#define SDHC_ERRINTSTS   0x032
#define SDHC_NORINTSTSEN 0x034
#define SDHC_ERRINTSTSEN 0x036
#define SDHC_NORINTSIGEN 0x038
#define SDHC_ERRINTSIGEN 0x03a
#define SDHC_CAPAB       0x040
#define SDHC_MAXCURCAP   0x048
#define SDHC_VERSION     0x0fe

// Transfer Mode Register Field
//      Multi/Single    Block Count
//      Block Select       Enable       Block Count     Function
//           0               /               /        Single Transfer
//           1               0               /        Infinite Transfer
//           1               1              !0        Multiple Transfer
//           1               1               0        Stop Multiple Transfer
#define SDHC_TRNMODE_MULTIBLK_SELECT    0x0002    
#define SDHC_TRNMODE_READ_SELECT        0x0001
#define SDHC_TRNMODE_ACMD_EN            0x0004
#define SDHC_TRNMODE_BLKCNT_EN          0x0002
#define SDHC_TRNMODE_DMA_EN             0x0001

// Block Size Register Field
#define SDHC_BLKSIZE_BS_MASK        0x0fff

// Normal Interrupt State Register Field
#define SDHC_NORINTSTS_ERROR_INT    0x8000
#define SDHC_NORINTSTS_CD_INT       0x0100
#define SDHC_NORINTSTS_CD_REMOVE    0x0080
#define SDHC_NORINTSTS_CD_INSERT    0x0040
#define SDHC_NORINTSTS_BUF_R_RDY    0x0020
#define SDHC_NORINTSTS_BUF_W_RDY    0x0010
#define SDHC_NORINTSTS_DMA_INT      0x0008
#define SDHC_NORINTSTS_BLK_GAP      0x0004
#define SDHC_NORINTSTS_TRN_CMPLT    0x0002
#define SDHC_NORINTSTS_CMD_CMPLT    0x0001

// Present State Register Field
#define SDHC_PRNSTATE_CD_INSERT     0x00010000
#define SDHC_PRNSTATE_RBUF_EN      0x00000800
#define SDHC_PRNSTATE_WBUF_EN      0x00000400
#define SDHC_PRNSTATE_RTRN_ACT     0x00000200
#define SDHC_PRNSTATE_WTRN_ACT     0x00000100
#define SDHC_PRNSTATE_DAT_ACT       0x00000004
#define SDHC_PRNSTATE_DAT_INHBT     0x00000002
#define SDHC_PRNSTATE_CMD_INHBT     0x00000001

// Normal Interrupt Status Register

// Normal Interrupt Status Enable Register Field
#define SDHC_NORINT_CARDINT         0x0100
#define SDHC_NORINT_REMOVE          0x0080
#define SDHC_NORINT_INSERT          0x0040
#define SDHC_NORINT_RBUFRDY         0x0020
#define SDHC_NORINT_WBUFRDY         0x0010
#define SDHC_NORINT_DMA             0x0008
#define SDHC_NORINT_TRSCMP          0x0002
#define SDHC_NORINT_CMDCMP          0x0001

#define sdhc_read(reg)          \
    assert(sizeof(reg) == len); \
    memcpy(bytes, &reg, len);

bool sdhc_t::load(reg_t addr, size_t len, uint8_t* bytes) {
    if (!sdcard.is_open()) {
        printf("[SimSDHC] no SD card to read\n");
        return false;
    }
        
    switch (addr) {
        case SDHC_SYSADDR:
            sdhc_read(sdhc_sysaddr);
            break;
        case SDHC_BLKSIZE:
            sdhc_read(sdhc_blksize);
            break;
        case SDHC_BLKCOUNT:
            sdhc_read(sdhc_blkcount);
            break;
        case SDHC_ARG:
            sdhc_read(sdhc_arg);
            break;
        case SDHC_TRNMODE:
            sdhc_read(sdhc_trnmode);
            break;
        case SDHC_CMD:
            sdhc_read(sdhc_cmd);
            break;
        case SDHC_RSP0 ... SDHC_RSP3 + 2:
            reg_t rsp = sdhc_rsp[addr >> 2 << 2];
            if ((len == 2) && (addr & 2))
                rsp = rsp >> 16;
            else if ((len == 2) && !(addr & 2))
                rsp = rsp & 15;
            memcpy(bytes, &rsp, len);
            break;
        case SDHC_BUFDATA:
            reg_t rsp = 0;
            if ((sdhc_prnstate & SDHC_PRNSTATE_RBUF_EN) == 0) {
                printf("[SimSDHC] buffer not ready now!");
                goto error;
            } else {
                for(unsigned int i = 0; i < 4; i++) 
                    rsp |= buffer[bufptr++] << (8*i);
                
                if ((bufptr) >= (sdhc_blksize & 0xFFF)) {
                    sdhc_prnstate &= ~SDHC_PRNSTATE_RBUF_EN; 
                    bufptr = 0; 

                    if (sdhc_trnmode & SDHC_TRNMODE_BLKCNT_EN)
                        sdhc_blkcount--;

                    if ((sdhc_trnmode & SDHC_TRNMODE_MULTIBLK_SELECT) == 0 ||
                        ((sdhc_trnmode & SDHC_TRNMODE_BLKCNT_EN) && (sdhc_blkcount == 0))) {
                        sdhc_prnstate &= ~(SDHC_PRNSTATE_RTRN_ACT | SDHC_PRNSTATE_DAT_INHBT | SDHC_PRNSTATE_DAT_ACT);
                            if (sdhc_norintstsen & SDHC_NORINT_TRSCMP) {
                                sdhc_norintsts |= SDHC_NORINT_TRSCMP;
                            }
                            check_int();
                    } else if ((sdcard_cur + 2 * SDHC_BLKLEN) <= sdcard_cap) {
                        sdcard_cur += SDHC_BLKLEN;
                        sdcard.seekg(sdcard_cur, std::ios::beg);
                        sdcard.read((char*)buffer, SDHC_BLKLEN);
                        sdhc_prnstate |= SDHC_PRNSTATE_RBUF_EN; 
                    } else {
                        error:
                            printf("[simSDHC] unknown case happens\n");
                            return false;
                    }
                }
            }

            memcpy(bytes, &rsp, len);
            break;
        case SDHC_PRNSTATE:
            sdhc_read(sdhc_prnstate);
            break;
        case SDHC_HOSTCTL1:
            sdhc_read(sdhc_hostctl1);
            break;
        case SDHC_PWRCTL:
            sdhc_read(sdhc_pwrctl);
            break;
        case SDHC_CLOCKCTL:
            sdhc_read(sdhc_clockctl);
            break;
        case SDHC_TMOUTCTL:
            sdhc_read(sdhc_tmoutctl);
            break;
        case SDHC_SFTRST:
            sdhc_read(sdhc_sftrst);
            break;
        case SDHC_NORINTSTS:
            sdhc_read(sdhc_norintsts);
            break;
        case SDHC_ERRINTSTS:
            sdhc_read(sdhc_errintsts);
            break;
        case SDHC_NORINTSTSEN:
            sdhc_read(sdhc_norintstsen);
            break;
        case SDHC_ERRINTSTSEN:
            sdhc_read(sdhc_errintstsen);
            break;
        case SDHC_NORINTSIGEN:
            sdhc_read(sdhc_norintsigen);
            break;
        case SDHC_ERRINTSIGEN:
            sdhc_read(sdhc_errintsigen);
            break;
        case SDHC_CAPAB:
            sdhc_read(sdhc_capab);
            break;
        case SDHC_MAXCURCAP:
            sdhc_read(sdhc_maxcurcap);
            break;
        case SDHC_VERSION:
            sdhc_read(sdhc_version);
            break;
    }

    return true;
}
#undef sdhc_read


#define sdhc_write(reg) // TODO

bool sdhc_t::store(reg_t addr, size_t len, const uint8_t* bytes) {
    if (!sdcard.is_open()) {
        printf("[SimSDHC] no SD card to write\n");
        return false;
    }
    switch (addr) {
        case SDHC_SYSADDR:
            sdhc_write(sdhc_sysaddr);
            break;
        case SDHC_BLKSIZE:
            sdhc_write(sdhc_blksize);
            break;
        case SDHC_BLKCOUNT:
            sdhc_write(sdhc_blkcount);
            break;
        case SDHC_ARG:
            sdhc_write(sdhc_arg);
            break;
        case SDHC_TRNMODE:
            sdhc_write(sdhc_trnmode);
            break;
        case SDHC_CMD:
            sdhc_write(sdhc_cmd);
            break;
        case SDHC_BUFDATA:
            sdhc_write(sdhc_bufdata);
            break;
        case SDHC_HOSTCTL1:
            sdhc_write(sdhc_hostctl1);
            break;
        case SDHC_PWRCTL:
            sdhc_write(sdhc_pwrctl);
            break;
        case SDHC_CLOCKCTL:
            sdhc_write(sdhc_clockctl);
            break;
        case SDHC_TMOUTCTL:
            sdhc_write(sdhc_tmoutctl);
            break;
        case SDHC_SFTRST:
            sdhc_write(sdhc_sftrst);
            break;
        case SDHC_NORINTSTS:
            sdhc_write(sdhc_norintsts);
            break;
        case SDHC_ERRINTSTS:
            sdhc_write(sdhc_errintsts);
            break;
        case SDHC_NORINTSTSEN:
            sdhc_write(sdhc_norintstsen);
            break;
        case SDHC_ERRINTSTSEN:
            sdhc_write(sdhc_errintstsen);
            break;
        case SDHC_NORINTSIGEN:
            sdhc_write(sdhc_norintsigen);
            break;
        case SDHC_ERRINTSIGEN:
            sdhc_write(sdhc_errintsigen);
            break;
    }

    return true;
}
#undef sdhc_write