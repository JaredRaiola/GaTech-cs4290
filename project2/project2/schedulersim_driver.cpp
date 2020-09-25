#include <cstdio>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "schedulersim.hpp"

int lineCounter = 1;

void print_help_and_exit() {
    printf("schedulersim [OPTIONS] < traces/file.trace\n");
    printf("  -r [NUM_REGS]\tNumber of registers in the trace file\n");
    printf("  -u [RS_SIZE]\tUse Unified RS, and set the size\n");
    printf("  -a [RS_SIZE]\tSet the size of the ADD RS\n");
    printf("  -d [RS_SIZE]\tSet the size of the DIV RS\n");
    printf("  -m [RS_SIZE]\tSet the size of the MEM RS\n");
    printf("  -h\t\t\tThis helpful output\n");

    exit(0);
}

void print_statistics(scheduler_stats_t* p_stats);
bool process_trace(op_type* p_op, int* p_dest, int* p_src1, int* p_src2);

int main(int argc, char* argv[]) {
    int opt;
    int num_registers = 32;
    int rs_sizes[] = {4, 4, 2};
    int rs_size = 4;
    rs_type rs = RSTYPE_PER_FU;

    // Process arguments
    while(-1 != (opt = getopt(argc, argv, "r:u:a:d:m:h"))) {
        switch(opt) {
        case 'r':
            num_registers = atoi(optarg);
            break;
        case 'a':
            rs_sizes[OP_ADD] = atoi(optarg);
            break;
        case 'd':
            rs_sizes[OP_DIV] = atoi(optarg);
            break;
        case 'm':
            rs_sizes[OP_MEM] = atoi(optarg);
            break;
        case 'u':
            rs = RSTYPE_UNIFIED;
            rs_size = atoi(optarg);
            break;
        case 'h':
            // Fall through
        default:
            print_help_and_exit();
            break;
        }
    }

    printf("Scheduler Settings\n");
    printf("Scheduler Type: %c\n", static_cast<char>(rs));
    printf("# Registers: %d\n", num_registers);
    if(rs == RSTYPE_UNIFIED) {
        printf("Reservation Station: %d entries\n", rs_size);
    } else {
        printf("Reservation Station ADD: %d entries\n", rs_sizes[OP_ADD]);
        printf("Reservation Station DIV: %d entries\n", rs_sizes[OP_DIV]);
        printf("Reservation Station MEM: %d entries\n", rs_sizes[OP_MEM]);
    }
    printf("\n");

    // Do some setup
    if(rs == RSTYPE_UNIFIED) {
        scheduler_unified_init(num_registers, rs_size);
    } else {
        scheduler_per_fu_init(num_registers, rs_sizes);
    }


    scheduler_stats_t stats;
    memset(&stats, 0, sizeof(scheduler_stats_t));
    // For each trace in the file
    while (!feof(stdin)) { 
        op_type op = OP_INVALID;
        int dest = INVALID_REG;
        int src1 = INVALID_REG;
        int src2 = INVALID_REG;

        if(process_trace(&op, &dest, &src1, &src2)) {
            // Retry the same instruction until we're successful
            bool success = false;
            do {
                success = scheduler_try_issue(op, dest, src1, src2, &stats);
                scheduler_step(&stats);
            } while(!success);
        }
        lineCounter++;
    }
    // Wait for the pipeline to drain
    while(!scheduler_completed()) {
        scheduler_step(&stats);
    }
    scheduler_complete(&stats);
    print_statistics(&stats);

    return 0;
}

void print_statistics(scheduler_stats_t* p_stats) {
    printf("Scheduler Statistics\n");
    printf("# Insts: %" PRIu64 "\n", p_stats->num_insts);
    printf("# Cycles: %" PRIu64 "\n", p_stats->num_cycles);
    printf("# IPC: %f\n", p_stats->ipc);
    printf("# Issue Stall: %" PRIu64 "\n", p_stats->issue_stall);
    printf("# Max Insts Fired: %" PRIu32 "\n", p_stats->max_fired);
    printf("# Max Insts Completed: %" PRIu32 "\n", p_stats->max_completed);
    printf("ADD FU Max Active: %" PRIu32 "\n", p_stats->max_active[OP_ADD]);
    printf("DIV FU Max Active: %" PRIu32 "\n", p_stats->max_active[OP_DIV]);
    printf("MEM FU Max Active: %" PRIu32 "\n", p_stats->max_active[OP_MEM]);
}

bool process_trace(op_type* p_op, int* p_dest, int* p_src1, int* p_src2) {
    char op[16];

    int ret = std::fscanf(stdin, "%s %d %d %d\n", op, p_dest, p_src1, p_src2);
    if(ret == 4) {
        *p_op = OP_INVALID;
        if(strcmp(op, "ADD") == 0) {
            *p_op = OP_ADD;
        } else if(strcmp(op, "DIV") == 0) {
            *p_op = OP_DIV;
        } else if(strcmp(op, "MEM") == 0) {
            *p_op = OP_MEM;
        }
        return true;
    } else {
        return false;
    }
}
