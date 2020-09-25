#include "schedulersim.hpp"
#include <cstdio>
#include <vector>
using namespace std;

//tracks the current cycle the scheduler is on
int current_cycle = 1;

//struct tracking important values to each instruction scheduled
struct instruction {
    op_type opcode; //type of instruction
    int destination; //destination register of instruction
    int src1; //register 1 of instruction
    int src2; //register 2 of instruction
    int cyclesLeft; //how many cycles the instruction has left to operate
    int cycleIssued; //what cycle the instruction was issued
    bool active; //was the instruction fired
};

//struct tracking individual reservation stations for perFU implementation
struct rStation {
    int maxAddSize; //max possible size of add reservation station
    int currAddSize; //current size of add reservation station
    int maxDivSize; //max possible size of div reservation station
    int currDivSize; //current size of div reservation station
    int maxMemSize; //max possible size of mem reservation station
    int currMemSize; //current size of mem reservation station
    instruction *addRS; //add reservation station pointer
    instruction *divRS; //div reservation station pointer
    instruction *memRS; //mem reservation station pointer
};

//reservation station array for unified
instruction *resStation;
//max reservation station size for unified
int maxResSize;
//current reservation station size for unified
int currResSize = 0;

//instance of rStation for perFU
rStation instance;
//pointer to instance of rStation perFU
rStation *rs = &instance;

//register alias table pointer
int *rat;
//type of scheduler (0 for unified, 1 for perFU)
int rsType = -1;
//current size of how many instructions are being fired perFU
int currPipelineSize = 0;
//tracks how many instructions are fired per cycle for perFU
int numFired_FU = 0;

/**
 * Subroutine that returns how long it takes to complete a specific operation type
 *
 * @return                      Instruction latency in cycles
 */
int get_inst_latency(op_type op) {
    if(op == OP_ADD) {
        return 2;
    } else if(op == OP_DIV) {
        return 15;
    } else if(op == OP_MEM) {
        return 20;
    }
    std::printf("Invalid OP type:%d\n", op);
    std::exit(1);
}

/**
 * XXX: You are welcome to define and set any global classes and variables as needed.
 */

/**
 * Subroutine for initializing the scheduler (unified reservation station type).
 * You may initalize any global or heap variables as needed.
 * XXX You're responsible for completing this routine.
 *
 * @param[in]   num_registers   The number of registers in the instructions
 * @param[in]   rs_size         The number of entries for the unified RS
 */
void scheduler_unified_init(int num_registers, int rs_size) {
  //register allocation table to size of how many registers there are
  rat = new int [num_registers];
  //clear out garbage memory values that could be in RAT
  for (int i = 0; i < num_registers; i++) {
  	rat[i] = 0;
  }
  //set max reservation station size to rs_size
  maxResSize = rs_size;
  //initialize an array of instructions of rs_size
  resStation = new instruction [rs_size];
  //set rsType to unified
  rsType = 0;
}

/**
 * Subroutine for initializing the scheduler (per-functional unit reservation station type).
 * You may initalize any global or heap variables as needed.
 * XXX You're responsible for completing this routine.
 *
 * @param[in]   num_registers   The number of registers in the instructions
 * @param[in]   rs_sizes        An array of size 3 that contains the number of entries for each
 *                              op_type
 *                              rs_sizes = [4,2,1] means 4 ADD RS, 2 DIV RS, 1 MEM RS
 */
void scheduler_per_fu_init(int num_registers, int rs_sizes[]) {
  //set add reservation max size to the first value in rs_sizes array
  rs->maxAddSize = rs_sizes[0];
  //set current add reservation size to 0
  rs->currAddSize = 0;
  //initialize add reservation station to array of instructions of size rs_sizes[0]
  rs->addRS = new instruction [rs_sizes[0]];

  //set div reservation max size to the second value in rs_sizes array
  rs->maxDivSize = rs_sizes[1];
  //set current div reservation size to 0
  rs->currDivSize = 0;
  //initialize div reservation station to array of instructions of size rs_sizes[1]
  rs->divRS = new instruction [rs_sizes[1]];

  //set mem reservation max size to the third value in rs_sizes array
  rs->maxMemSize = rs_sizes[2];
  //set current mem reservation size to 0
  rs->currMemSize = 0;
  //initialize mem reservation station to array of instructions of size rs_sizes[2]
  rs->memRS = new instruction [rs_sizes[2]];

  //register allocation table to size of how many registers there are
  rat = new int [num_registers];
  //clear out possible garbage values in RAT
  for (int i = 0; i < num_registers; i++) {
    rat[i] = 0;
  }
  rsType = 1;
}

/**
 * Subroutine that tries to issue an instruction to the reservation station. You need to 
 * choose the appropriate RS depending on the RS type and op_type and update the RAT.
 * XXX You're responsible for completing this routine.
 *
 * @param[in]   op_type         The FU to use
 * @param[in]   dest            The destination register
 * @param[in]   src1            The first source register (-1 if unused)
 * @param[in]   src2            The seconde source register (-1 if unused)
 * @param[out]  p_stats         Pointer to the stats structure
 *
 * @return                      true if successful, false if we failed
 */
bool scheduler_try_issue(op_type op, int dest, int src1, int src2, scheduler_stats_t* p_stats) {
  //if unified
  if (rsType == 0) {
    //if we have space in reservation station
    if (currResSize < maxResSize) {
      //get the number of cycles this instruction will run for
      int c = get_inst_latency(op);
      //add instruction to reservation station
      resStation[currResSize] = {op, dest, src1, src2, c, current_cycle, false};
      //increase number of instructions
      p_stats->num_insts++;
      //add one to current size of reservation station
      currResSize++;
      //successfully scheduled
      return true;
    } else {
      //no space, add a stall
      p_stats->issue_stall++;
      return false;
    }
  //if perFU
  } else if (rsType == 1) {
    //if add instruction
    if (op == 0) {
      //if space in add reservation station
      if (rs->currAddSize < rs->maxAddSize) {
        //get the number of cycles this instruction will run for
        int c = get_inst_latency(op);
        //add instruction to add reservation station
        rs->addRS[rs->currAddSize] = {op, dest, src1, src2, c, current_cycle, false};
        //increase num instructions, current add reservation size and current pipeline size
        p_stats->num_insts++;
        rs->currAddSize++;
        currPipelineSize++;
        //succesfully scheduled
        return true;
      } else {
        //no space, increase stall counter
        p_stats->issue_stall++;
        return false;
      }
    //if div instruction
    } else if (op == 1) {
      //if space in div reservation station
      if (rs->currDivSize < rs->maxDivSize) {
        //get the number of cycles this instruction will run for
        int c = get_inst_latency(op);
        //add instruction to div reservation station
        rs->divRS[rs->currDivSize] = {op, dest, src1, src2, c, current_cycle, false};
        //increase num instructions, current div reservation size and current pipeline size
        p_stats->num_insts++;
        rs->currDivSize++;
        currPipelineSize++;
        //succesfully scheduled
        return true;
      } else {
        //no space, increase stall counter
        p_stats->issue_stall++;
        return false;
      }
    //if mem instruction
    } else if (op == 2) {
      // if space in mem reservation station
      if (rs->currMemSize < rs->maxMemSize) {
        //get the number of cycles this instruction will run for
        int c = get_inst_latency(op);
        //add instruction to mem reservation station
        rs->memRS[rs->currMemSize] = {op, dest, src1, src2, c, current_cycle, false};
        //increase num instructions, current mem reservation size and current pipeline size
        p_stats->num_insts++;
        rs->currMemSize++;
        currPipelineSize++;
        //succesfully scheduled
        return true;
      } else {
        //no space, increase stall counter
        p_stats->issue_stall++;
        return false;
      }
    } 
  }
  //didn't correctly send an instruction, stall
  return false;
}

/**
 * Subroutine that checks if all instructions have been drained from the pipeline
 *
 * @return                      true if no instructions are left
 */
bool scheduler_completed() {
  //if unified
	if (rsType == 0) {
    //true when nothing is left in reservation station
    return currResSize == 0;
  //if perFU
	} else if (rsType == 1) {
    //true if pipeline is drained
		return currPipelineSize == 0;
	}
  //scheduler wasn't set correctly
  return false;
}

/**
 * Subroutine that increments the clock cycle and updates any system state
 *
 * @param[out]  p_stats         Pointer to the stats structure
 */
void scheduler_step(scheduler_stats_t* p_stats) {
  //increment cycle counters
	p_stats->num_cycles++;
  current_cycle++;
  scheduler_clear_completed(p_stats);
  scheduler_start_ready(p_stats);
}

/**
 * Helper function used to check for RAT collisions, outputs a boolean to 
 * indicate whether a collision has happened or not
 *
 * @param  rs         instruction being checked against RAT
 */
bool ratCollisionCheck(instruction rs) {
  //if WAW hazard
	if (rat[rs.destination - 1] == 1) {
		return true;
	}
  //if RAW hazard
	if (rat[rs.src1 - 1] == 1) {
		return true;
	}
  //if RAW hazard
	if (rat[rs.src2 - 1] == 1) {
		return true;
	}
  //no collision
	return false;
}

/**
 * Helper function for special case when all three reservation stations for 
 * perFU have the ability to fire.
 *
 * @param  addInactive        location in the add reservation station where   
 *                            the next inactive instruction is
 * @param  divInactive        location in the div reservation station where   
 *                            the next inactive instruction is
 * @param  memInactive        location in the mem reservation station where   
 *                            the next inactive instruction is
 */
bool checkAllThree(int addInactive, int divInactive, int memInactive) {
  //if the add reservation station has the earliest issued instruction
  if (rs->addRS[addInactive].cycleIssued <= rs->divRS[divInactive].cycleIssued && rs->addRS[addInactive].cycleIssued <= rs->memRS[memInactive].cycleIssued) {
    //check to see if there's a RAT collision
    if (!ratCollisionCheck(rs->addRS[addInactive])) {
      //set the register being written to as busy in the RAT
      rat[rs->addRS[addInactive].destination - 1] = 1;
      //set instruction to being fired
      rs->addRS[addInactive].active = true;
      //increase number of instructions fired this cycle counter
      numFired_FU++;
      //try to keep scheduling this cycle
      return false;
    } else {
      //we had a collision, stop scheduling for this cycle
      return true;
    }
  //if the div reservation station has the earliest issued instruction
  } else if (rs->divRS[divInactive].cycleIssued <= rs->addRS[addInactive].cycleIssued && rs->divRS[divInactive].cycleIssued <= rs->memRS[memInactive].cycleIssued) {
    //check to see if there's a RAT collision
    if (!ratCollisionCheck(rs->divRS[divInactive])) {
      //set the register being written to as busy in the RAT
      rat[rs->divRS[divInactive].destination - 1] = 1;
      //set instruction to being fired
      rs->divRS[divInactive].active = true;
      //increase number of instructions fired this cycle counter
      numFired_FU++;
      //try to keep scheduling this cycle
      return false;
    } else {
      //we had a collision, stop scheduling for this cycle
      return true;
    }
  //if the mem reservation station has the earliest issued instruction
  } else if (rs->memRS[memInactive].cycleIssued <= rs->addRS[addInactive].cycleIssued && rs->memRS[memInactive].cycleIssued <= rs->divRS[divInactive].cycleIssued){
    //check to see if there's a RAT collision
    if (!ratCollisionCheck(rs->memRS[memInactive])) {
      //set the register being written to as busy in the RAT
      rat[rs->memRS[memInactive].destination - 1] = 1;
      //set instruction to being fired
      rs->memRS[memInactive].active = true;
      //increase number of instructions fired this cycle counter
      numFired_FU++;
      //try to keep scheduling this cycle
      return false;
    } else {
      //we had a collision, stop scheduling for this cycle
      return true;
    }
  }
  //parameter was not set correctly, stop scheduling
  return true;
}

/**
 * Helper function to determine earliest issued instruction for perFU  
 * scheduler
 */
bool functionalHelper() {
  //set locations of inactive instructions to -1 (not existing)
  int divInactive = -1;
  int memInactive = -1;
  int addInactive = -1;
  //location counter variable j
  int j = 0;
  //while we haven't looked through all indexes or found an inactive index
  while (j < rs->currAddSize && addInactive == -1) {
    //if inactive
    if (!rs->addRS[j].active) {
      //found, break while loop
      addInactive = j;
    } else {
      j++;
    }
  }

  j = 0;
  //while we haven't looked through all indexes or found an inactive index
  while (j < rs->currDivSize && divInactive == -1) {
    //if inactive
    if (!rs->divRS[j].active) {
      //found, break while loop
      divInactive = j;
    } else {
      j++;
    }
  }

  j = 0;
  //while we haven't looked through all indexes or found an inactive index
  while (j < rs->currMemSize && memInactive == -1) {
    // if inactive
    if (!rs->memRS[j].active) {
      //found, break while loop
      memInactive = j;
    } else {
      j++;
    }
  }

  //if nothing is inactive, stop trying to fire
  if (divInactive == -1 && memInactive == -1 && addInactive == -1) {
    return true;
  //if nothing is inactive in div and mem, try to schedule add
  } else if (divInactive == -1 && memInactive == -1) {
    //check to see if there's a RAT collision
    if (!ratCollisionCheck(rs->addRS[addInactive])) {
      //set the register being written to as busy in the RAT
      rat[rs->addRS[addInactive].destination - 1] = 1;
      //set instruction to being fired
      rs->addRS[addInactive].active = true;
      //increase number of instructions fired this cycle counter
      numFired_FU++;
      //try to keep scheduling this cycle
      return false;
    } else {
      //we had a collision, stop scheduling for this cycle
      return true;
    }
  //if nothing is inactive in add and mem, try to schedule div
  } else if (addInactive == -1 && memInactive == -1) {
    //check to see if there's a RAT collision
    if (!ratCollisionCheck(rs->divRS[divInactive])) {
      //set the register being written to as busy in the RAT
      rat[rs->divRS[divInactive].destination - 1] = 1;
      //set instruction to being fired
      rs->divRS[divInactive].active = true;
      //increase number of instructions fired this cycle counter
      numFired_FU++;
      //try to keep scheduling this cycle
      return false;
    } else {
      //we had a collision, stop scheduling for this cycle
      return true;
    }
  //if nothing is inactive in add and div, try to schedule mem
  } else if (divInactive == -1 && addInactive == -1) {
    //check to see if there's a RAT collision
    if (!ratCollisionCheck(rs->memRS[memInactive])) {
      //set the register being written to as busy in the RAT
      rat[rs->memRS[memInactive].destination - 1] = 1;
      //set instruction to being fired
      rs->memRS[memInactive].active = true;
      //increase number of instructions fired this cycle counter
      numFired_FU++;
      //try to keep scheduling this cycle
      return false;
    } else {
      //we had a collision, stop scheduling for this cycle
      return true;
    }
  //if nothing is inactive in mem, pick earliest issued instruction between add and div
  } else if (memInactive == -1) {
    //if add instruction was issued earlier than div
    if (rs->addRS[addInactive].cycleIssued <= rs->divRS[divInactive].cycleIssued) {
      //check to see if there's a RAT collision
      if (!ratCollisionCheck(rs->addRS[addInactive])) {
        //set the register being written to as busy in the RAT
        rat[rs->addRS[addInactive].destination - 1] = 1;
        //set instruction to being fired
        rs->addRS[addInactive].active = true;
        //increase number of instructions fired this cycle counter
        numFired_FU++;
        //try to keep scheduling this cycle
        return false;
      } else {
        //we had a collision, stop scheduling for this cycle
        return true;
      }
    //else, div was earlier than add
    } else {
      //check to see if there's a RAT collision
      if (!ratCollisionCheck(rs->divRS[divInactive])) {
        //set the register being written to as busy in the RAT
        rat[rs->divRS[divInactive].destination - 1] = 1;
        //set instruction to being fired
        rs->divRS[divInactive].active = true;
        //increase number of instructions fired this cycle counter
        numFired_FU++;
        //try to keep scheduling this cycle
        return false;
      } else {
        //we had a collision, stop scheduling for this cycle
        return true;
      }
    }
  //if nothing is inactive in div, pick earliest issued instruction between add and mem
  } else if (divInactive == -1) {
    //if mem instruction was issued earlier than add
    if (rs->memRS[memInactive].cycleIssued <= rs->addRS[addInactive].cycleIssued) {
      //check to see if there's a RAT collision
      if (!ratCollisionCheck(rs->memRS[memInactive])) {
        //set the register being written to as busy in the RAT
        rat[rs->memRS[memInactive].destination - 1] = 1;
        //set instruction to being fired
        rs->memRS[memInactive].active = true;
        //increase number of instructions fired this cycle counter
        numFired_FU++;
        //try to keep scheduling this cycle
        return false;
      } else {
        //we had a collision, stop scheduling for this cycle
        return true;
      }
    //else, add was earlier than mem
    } else {
      //check to see if there's a RAT collision
      if (!ratCollisionCheck(rs->addRS[addInactive])) {
        //set the register being written to as busy in the RAT
        rat[rs->addRS[addInactive].destination - 1] = 1;
        //set instruction to being fired
        rs->addRS[addInactive].active = true;
        //increase number of instructions fired this cycle counter
        numFired_FU++;
        //try to keep scheduling this cycle
        return false;
      } else {
        //we had a collision, stop scheduling for this cycle
        return true;
      }
    }
  //if nothing is inactive in add, pick earliest issued instruction between div and mem
  } else if (addInactive == -1) {
    //if mem instruction was issued earlier than div
    if (rs->memRS[memInactive].cycleIssued <= rs->divRS[divInactive].cycleIssued) {
      //set the register being written to as busy in the RAT
      if (!ratCollisionCheck(rs->memRS[memInactive])) {
        //set the register being written to as busy in the RAT
        rat[rs->memRS[memInactive].destination - 1] = 1;
        //set instruction to being fired
        rs->memRS[memInactive].active = true;
        numFired_FU++;
        //try to keep scheduling this cycle
        return false;
      } else {
        //we had a collision, stop scheduling for this cycle
        return true;
      }
    //else, div was earlier than add
    } else {
      //set the register being written to as busy in the RAT
      if (!ratCollisionCheck(rs->divRS[divInactive])) {
        //set the register being written to as busy in the RAT
        rat[rs->divRS[divInactive].destination - 1] = 1;
        //set instruction to being fired
        rs->divRS[divInactive].active = true;
        //increase number of instructions fired this cycle counter
        numFired_FU++;
        //try to keep scheduling this cycle
        return false;
      } else {
        //we had a collision, stop scheduling for this cycle
        return true;
      }
    }
  } else {
    //special case, checkAllThree function
    return checkAllThree(addInactive, divInactive, memInactive);
  }
  //didn't hit any case, something went wrong, stop trying to fire
  return true;
}

/**
 * Subroutine for firing (start executing) any ready instructions.
 * XXX You're responsible for completing this routine.
 *
 * @param[out]  p_stats         Pointer to the stats structure
 */
void scheduler_start_ready(scheduler_stats_t* p_stats) {
  //if unified scheduler
  if (rsType == 0) {
    //did we check all reservation station locations
    bool checkAll = false;
    //how many instructions were fired this cycle
    int numFired = 0;
    //last started optimization, since we check from 0 to size, we can restart from the last instruction that was started
    int lastStarted = 0;
    //how many instructions are active perFU
    int numAddActive = 0;
    int numDivActive = 0;
    int numMemActive = 0;
    //loop through all instructions, decrement cycles left for active instructions and count how many instructions are active for each functional unit
    for (int i = 0; i < currResSize; i++) {
      if (resStation[i].active) {
        resStation[i].cyclesLeft--;
        if (resStation[i].opcode == 0) {
          numAddActive++;
        } else if (resStation[i].opcode == 1) {
          numDivActive++;
        } else if (resStation[i].opcode == 2) {
          numMemActive++;
        }
      }
    }
    //update p_stats structure with new max_active values for each functional unit ONLY if they are higher
    if (numAddActive > (int)p_stats->max_active[0]) {
      p_stats->max_active[0] = numAddActive;
    }
    if (numDivActive > (int)p_stats->max_active[1]) {
      p_stats->max_active[1] = numDivActive;
    }
    if (numMemActive > (int)p_stats->max_active[2]) {
      p_stats->max_active[2] = numMemActive;
    }
  	//loop and try to keep scheduling
    while (!checkAll) {
      //set counter to where we left off last cycle
    	int j = lastStarted;
      //set inactive instruction to not existing
    	int inactive = -1;
      //have we found an inactive value
      int found = 0;
      //while we haven't checked every reservation station instruction OR found an inactive instruction
    	while (j < currResSize && inactive == -1) {
        //if index is inactive
    		if (!resStation[j].active) {
          //update inactive and set found to 1
    			inactive = j;
          found = 1;
    		} else {
    			j++;
    		}
    	}
      //if we found an inactive instruction in the reservation station, check for RAT collision
    	if (found == 1 && !ratCollisionCheck(resStation[j])) {
    		//set the register being written to as busy in the RAT
    		rat[resStation[j].destination - 1] = 1;
        //set instruction to fired
    		resStation[j].active = true;
        //set lastStarted to last index we set to fired
        lastStarted = j;
        //increment the amount of instructions fired this cycle
        numFired++;
    	} else {
        //we've looked at all instructions OR hit a collision, stop scheduling
        checkAll = true;
      }
    }
    //if we've fired more instructions this round than previous, update p_stats
    if (numFired > (int)p_stats->max_fired) {
      p_stats->max_fired = numFired;
    }
  // else scheduler type is 1, perFU
  } else {
    //did we check all reservation station locations
    bool checkAll = false;
    //how many instructions are active perFU
    int numAddActive = 0;
    int numDivActive = 0;
    int numMemActive = 0;
    //loop through all instructions, decrement cycles left for active instructions and count how many instructions are active for each functional unit
    for (int i = 0; i < rs->currAddSize; i++) {
      if (rs->addRS[i].active) {
        rs->addRS[i].cyclesLeft--;
        numAddActive++;
      }
    }
    for (int i = 0; i < rs->currDivSize; i++) {
      if (rs->divRS[i].active) {
        rs->divRS[i].cyclesLeft--;
        numDivActive++;
      }
    }
    for (int i = 0; i < rs->currMemSize; i++) {
      if (rs->memRS[i].active) {
        rs->memRS[i].cyclesLeft--;
        numMemActive++;
      }
    }
    //update p_stats structure with new max_active values for each functional unit ONLY if they are higher
    if (numAddActive > (int)p_stats->max_active[0]) {
      p_stats->max_active[0] = numAddActive;
    }
    if (numDivActive > (int)p_stats->max_active[1]) {
      p_stats->max_active[1] = numDivActive;
    }
    if (numMemActive > (int)p_stats->max_active[2]) {
      p_stats->max_active[2] = numMemActive;
    }
    //reset number of instructions fired this cycle to 0
    numFired_FU = 0;
    //while we haven't exhausted all firing options
    while (!checkAll) {
      //try to fire instructions
      checkAll = functionalHelper();
    }
    //if we've fired more instructions this cycle, update p_stats
    if (numFired_FU > (int)p_stats->max_fired) {
      p_stats->max_fired = numFired_FU;
    }
  }
}

/**
 * Subroutine for clearing any completed instructions.
 * XXX You're responsible for completing this routine.
 *
 * @param[out]  p_stats         Pointer to the stats structure
 */
void scheduler_clear_completed(scheduler_stats_t* p_stats) {
  //if unified scheduler
  if (rsType == 0) {
    //check all reservation station instructions
  	bool checkAll = false;
    //counter for how many instructions are completed this cycle
    int numCompleted = 0;
    //while we haven't checked all instructions in the reservation station
  	while (!checkAll) {
  		int i = 0;
      //set location of a finished instruction to not existing
  		int finishedInst = -1;
      //while we haven't looked at all instructions in the reservation station OR we haven't found one that is completed
  		while (i < currResSize && finishedInst == -1) {
        //if the instruction has one cycle left, there isn't only one instruction in the reservation station
  			if (resStation[i].cyclesLeft == 1 && currResSize != 1) {
          //the instruction is finished
  				finishedInst = i;
        //if the instruction has no cycles left
  			} else if (resStation[i].cyclesLeft == 0) {
          //the instruction is finished
          finishedInst = i;
        }
  			i++;
  		}
      //if there are no finished instructions
  		if (finishedInst == -1) {
        //we've checked all, stop clearing
  			checkAll = true;
      //remove finished instruction
  		} else {
        //set RAT value to not busy
  			rat[resStation[finishedInst].destination - 1] = 0;
        //shift all values after the finished instruction in the reservation station one index down
  			for (int j = finishedInst; j < currResSize - 1; j++) {
  				resStation[j] = resStation[j+1];
  			}
        //decrease current size of reservation station
  			currResSize--;
        //increment number of instructions finished this cycle
	  		numCompleted++;
  		}
  	}
    //if we've finished more instructions this cycle than previously, update p_stats
    if (numCompleted > (int)p_stats->max_completed) {
      p_stats->max_completed = numCompleted;
    }
  //else if perFU scheduler
  } else if (rsType == 1) {
    //check all reservation station instructions
    bool checkAll = false;
    //counter for how many instructions are completed this cycle
    int numCompleted = 0;
    //while we haven't checked all instructions in add reservation station
    while (!checkAll) {
      int i = 0;
      //set location of a finished instruction to not existing
      int finishedInst = -1;
      //while we haven't looked at all instructions in the add reservation station OR we haven't found one that is completed
      while (i < rs->currAddSize && finishedInst == -1) {
        //if the instruction has one cycle left, there isn't only one instruction in the reservation station
        if (rs->addRS[i].cyclesLeft == 1 && currPipelineSize != 1) {
          //the instruction is finished
          finishedInst = i;
        //if the instruction has no cycles left
        } else if (rs->addRS[i].cyclesLeft == 0) {
          //the instruction is finished
          finishedInst = i;
        }
        i++;
      }
      //if there are no finished instructions
      if (finishedInst == -1) {
        //we've checked all, stop clearing
        checkAll = true;
      //remove finished instruction
      } else {
        //set RAT value to not busy
        rat[rs->addRS[finishedInst].destination - 1] = 0;
        //shift all values after the finished instruction in the add reservation station one index down
        for (int j = finishedInst; j < rs->currAddSize - 1; j++) {
          rs->addRS[j] = rs->addRS[j+1];
        }
        //decrease current size of pipeline
        currPipelineSize--;
        //decrease current size of add reservation station
        rs->currAddSize--;
        //increment number of instructions finished this cycle
        numCompleted++;
      }
    }
    checkAll = false;
    //while we haven't checked all instructions in div reservation station
    while (!checkAll) {
      int i = 0;
      //set location of a finished instruction to not existing
      int finishedInst = -1;
      //while we haven't looked at all instructions in the div reservation station OR we haven't found one that is completed
      while (i < rs->currDivSize && finishedInst == -1) {
        //if the instruction has one cycle left, there isn't only one instruction in the reservation station
        if (rs->divRS[i].cyclesLeft == 1 && currPipelineSize != 1) {
          //the instruction is finished
          finishedInst = i;
        //if the instruction has no cycles left
        } else if (rs->divRS[i].cyclesLeft == 0) {
          //the instruction is finished
          finishedInst = i;
        }
        i++;
      }
      //if there are no finished instructions
      if (finishedInst == -1) {
        //we've checked all, stop clearing
        checkAll = true;
      //remove finished instruction
      } else {
        //set RAT value to not busy
        rat[rs->divRS[finishedInst].destination - 1] = 0;
        //shift all values after the finished instruction in the div reservation station one index down
        for (int j = finishedInst; j < rs->currDivSize - 1; j++) {
          rs->divRS[j] = rs->divRS[j+1];
        }
        //decrease current size of pipeline
        currPipelineSize--;
        //decrease current size of div reservation station
        rs->currDivSize--;
        //increment number of instructions finished this cycle
        numCompleted++;
      }
    }
    checkAll = false;
    //while we haven't checked all instructions in mem reservation station
    while (!checkAll) {
      int i = 0;
      //set location of a finished instruction to not existing
      int finishedInst = -1;
      //while we haven't looked at all instructions in the mem reservation station OR we haven't found one that is completed
      while (i < rs->currMemSize && finishedInst == -1) {
        //if the instruction has one cycle left, there isn't only one instruction in the reservation station
        if (rs->memRS[i].cyclesLeft == 1 && currPipelineSize != 1) {
          //the instruction is finished
          finishedInst = i;
        //if the instruction has no cycles left
        } else if (rs->memRS[i].cyclesLeft == 0) {
          //the instruction is finished
          finishedInst = i;
        }
        i++;
      }
      //if there are no finished instructions
      if (finishedInst == -1) {
        //we've checked all, stop clearing
        checkAll = true;
      //remove finished instruction
      } else {
        //set RAT value to not busy
        rat[rs->memRS[finishedInst].destination - 1] = 0;
        //shift all values after the finished instruction in the mem reservation station one index down
        for (int j = finishedInst; j < rs->currMemSize - 1; j++) {
          rs->memRS[j] = rs->memRS[j+1];
        }
        //decrease current size of pipeline
        currPipelineSize--;
        //decrease current size of div reservation station
        rs->currMemSize--;
        //increment number of instructions finished this cycle
        numCompleted++;
      }
    }
    //if we've finished more instructions this cycle than previously, update p_stats
    if (numCompleted > (int)p_stats->max_completed) {
      p_stats->max_completed = numCompleted;
    }
  }
}

/**
 * Subroutine for completing the scheduler and getting any final stats
 * XXX You're responsible for completing this routine.
 *
 * @param[out]  p_stats         Pointer to the stats structure
 */
void scheduler_complete(scheduler_stats_t* p_stats) {
  //calculate instructions per cycle
  p_stats->ipc = (double)p_stats->num_insts/(double)p_stats->num_cycles;
}