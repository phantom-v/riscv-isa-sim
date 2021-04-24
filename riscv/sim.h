// See LICENSE for license details.

#ifndef _RISCV_SIM_H
#define _RISCV_SIM_H

#include "debug_module.h"
#include "devices.h"
#include "log_file.h"
#include "processor.h"
#include "simif.h"

#include <fesvr/htif.h>
#include <fesvr/context.h>
#include <vector>
#include <string>
#include <memory>
#include <sys/types.h>

#ifdef ZJV_DEVICE_EXTENSION  
  #include "soc.h"
#endif

// TO DELETE
#include <iostream>
#include <stdlib.h>
#include <unistd.h>

class mmu_t;
class remote_bitbang_t;

// this class encapsulates the processors and memory in a RISC-V machine.
class sim_t : public htif_t, public simif_t
{
public:
  sim_t(const char* isa, const char* priv, const char* varch, size_t _nprocs,
        bool halted, bool real_time_clint,
        reg_t initrd_start, reg_t initrd_end, const char* bootargs,
        reg_t start_pc, std::vector<std::pair<reg_t, mem_t*>> mems,
        std::vector<std::pair<reg_t, abstract_device_t*>> plugin_devices,
        const std::vector<std::string>& args, const std::vector<int> hartids,
        const debug_module_config_t &dm_config, const char *log_path,
        bool dtb_enabled, const char *dtb_file, 
        bool diffTest, std::string uart_fifo);
  ~sim_t();

  // DiffTest
  void difftest_continue(size_t n) { 
    step(n, false); 
  }

  void difftest_checkINT() {
    step(1, true);
  }

  void sync_cycle() {
    if (current_step == INSNS_PER_RTC_TICK) {
        current_step = 0;
        clint->increment(1);
    }
    // fprintf(stderr, "mtime %lx current_step %lx\n", clint.get()->get_mtime(), current_step);
    current_step++; 
    state_t* state = get_core(0)->get_state();
    state->mcycle++;
  }  

  void set_state (state_t* new_state) {
    state_t* state = get_core(0)->get_state();
    for (int i = 0; i < 32; i++) {
      state->XPR.write(i, new_state->XPR[i]);
    }

    // TODO: else
  }

  void set_state (int index, reg_t new_value) {
    state_t* state = get_core(0)->get_state();
    state->XPR.write(index, new_value);
  }

  void difftest_setup() {
    start();

    state_t* state = get_core(0)->get_state();
    do {
      step(1, false);
    } while(state->pc != start_pc);

    state->XPR.reset();
  }

  state_t* get_state() {
    return get_core(0)->get_state();
  }

  // run the simulation to completion
  int run();
  void set_debug(bool value);
  void set_histogram(bool value);

  // Configure logging
  //
  // If enable_log is true, an instruction trace will be generated. If
  // enable_commitlog is true, so will the commit results (if this
  // build was configured without support for commit logging, the
  // function will print an error message and abort).
  void configure_log(bool enable_log, bool enable_commitlog);

  void set_procs_debug(bool value);
  void set_procs_diffTest(bool value);
  void set_remote_bitbang(remote_bitbang_t* remote_bitbang) {
    this->remote_bitbang = remote_bitbang;
  }
  const char* get_dts() { if (dts.empty()) reset(); return dts.c_str(); }
  processor_t* get_core(size_t i) { return procs.at(i); }
  unsigned nprocs() const { return procs.size(); }

  // Callback for processors to let the simulation know they were reset.
  void proc_reset(unsigned id);

private:
  std::vector<std::pair<reg_t, mem_t*>> mems;
  std::vector<std::pair<reg_t, abstract_device_t*>> plugin_devices;
  mmu_t* debug_mmu;  // debug port into main memory
  std::vector<processor_t*> procs;
  reg_t initrd_start;
  reg_t initrd_end;
  const char* bootargs;
  reg_t start_pc;
  std::string dts;
  std::string dtb;
  std::string dtb_file;
  bool dtb_enabled;
  std::unique_ptr<rom_device_t> boot_rom;
  std::unique_ptr<clint_t> clint;
#ifdef ZJV_DEVICE_EXTENSION  
  std::unique_ptr<uart_t> uart;
  std::unique_ptr<sdhc_t> sdhc;
  std::unique_ptr<plic_t> plic;
#endif
  bus_t bus;
  log_file_t log_file;

  processor_t* get_core(const std::string& i);
  void step(size_t n, bool check_int=true); // step through simulation
  static const size_t INTERLEAVE = 5000;
  static const size_t INSNS_PER_RTC_TICK = 100; // 10 MHz clock for 1 BIPS core
  static const size_t CPU_HZ = 1000000000; // 1GHz CPU
  size_t current_step;
  size_t current_proc;
  bool debug;
  bool histogram_enabled; // provide a histogram of PCs
  bool log;
  remote_bitbang_t* remote_bitbang;
  bool diffTest;

  // memory-mapped I/O routines
  char* addr_to_mem(reg_t addr);
  bool mmio_load(reg_t addr, size_t len, uint8_t* bytes);
  bool mmio_store(reg_t addr, size_t len, const uint8_t* bytes);
  void make_dtb();
  void set_rom();

  // presents a prompt for introspection into the simulation
  void interactive();

  // functions that help implement interactive()
  void interactive_help(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_quit(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_run(const std::string& cmd, const std::vector<std::string>& args, bool noisy);
  void interactive_run_noisy(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_run_silent(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_vreg(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_reg(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_freg(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_fregh(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_fregs(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_fregd(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_pc(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_mem(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_str(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_until(const std::string& cmd, const std::vector<std::string>& args, bool noisy);
  void interactive_until_silent(const std::string& cmd, const std::vector<std::string>& args);
  void interactive_until_noisy(const std::string& cmd, const std::vector<std::string>& args);
  reg_t get_reg(const std::vector<std::string>& args);
  freg_t get_freg(const std::vector<std::string>& args);
  reg_t get_mem(const std::vector<std::string>& args);
  reg_t get_pc(const std::vector<std::string>& args);

  friend class processor_t;
  friend class mmu_t;
  friend class debug_module_t;

  // htif
  friend void sim_thread_main(void*);
  void main();

  context_t* host;
  context_t target;
  void reset();
  void idle();
  void read_chunk(addr_t taddr, size_t len, void* dst);
  void write_chunk(addr_t taddr, size_t len, const void* src);
  size_t chunk_align() { return 8; }
  size_t chunk_max_size() { return 8; }

public:
  // Initialize this after procs, because in debug_module_t::reset() we
  // enumerate processors, which segfaults if procs hasn't been initialized
  // yet.
  debug_module_t debug_module;
};

extern volatile bool ctrlc_pressed;

#endif
