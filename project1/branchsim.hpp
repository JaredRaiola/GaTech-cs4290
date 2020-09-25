#ifndef BRANCHSIM_HPP
#define BRANCHSIM_HPP

#include <cstdint>

struct branch_stats_t {
    std::uint64_t num_branches;
    std::uint64_t pred_taken;
    std::uint64_t pred_not_taken;
    std::uint64_t correct;
    double   misprediction_rate;
    std::uint64_t storage_overhead;
};

enum predictor_type {
    PTYPE_BIMODAL            = 'B',
    PTYPE_GSHARE             = 'G',
    PTYPE_LOCAL_HISTORY      = 'L',
    PTYPE_TWO_LEVEL_ADAPTIVE = 'T',
};

enum branch_dir {
    TAKEN       = 'T',
    NOT_TAKEN   = 'N',
};

void setup_predictor(predictor_type ptype, int num_entries, int counter_bits, int history_bits,
                     branch_stats_t* p_stats);
branch_dir predict_branch(std::uint64_t pc, branch_stats_t* p_stats);
void update_predictor(std::uint64_t pc, branch_dir actual, branch_dir predicted, branch_stats_t* p_stats);
void complete_predictor(branch_stats_t *p_stats);

#endif /* BRANCHSIM_HPP */
