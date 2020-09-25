#ifndef BRANCHSIM_HPP
#define BRANCHSIM_HPP

#include <cstdint>
#include <vector>
#include <algorithm>

const static int INVALID_REG = -1;

enum rs_type {
    RSTYPE_UNIFIED      = 'U',
    RSTYPE_PER_FU       = 'F',
};

enum op_type {
    OP_INVALID          = -1,
    OP_ADD              = 0,
    OP_DIV              = 1,
    OP_MEM              = 2,
    NUM_OPS             = 3,
};

struct scheduler_stats_t {
    std::uint64_t num_insts;
    std::uint64_t num_cycles;
    double        ipc;
    std::uint64_t issue_stall;
    std::uint32_t max_fired;
    std::uint32_t max_completed;
    std::uint32_t max_active[NUM_OPS];
};

void scheduler_unified_init(int num_registers, int rs_size);
void scheduler_per_fu_init(int num_registers, int rs_sizes[]);
bool scheduler_try_issue(op_type op, int dest, int src1, int src2, scheduler_stats_t* p_stats);
void scheduler_step(scheduler_stats_t* p_stats);
void scheduler_start_ready(scheduler_stats_t* p_stats);
void scheduler_clear_completed(scheduler_stats_t* p_stats);
bool scheduler_completed();
void scheduler_complete(scheduler_stats_t* p_stats);

extern int current_cycle;

#endif /* BRANCHSIM_HPP */
