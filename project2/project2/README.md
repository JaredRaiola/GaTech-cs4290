Georgia Institute of Technology
School of Computer Science
CS 4290: Summer, 2019
Project 2: Instruction Scheduler
DUE: 07/09/2019, 11:55 pm, EST

Rules
-----
This is the second project for the course. As usual, here are the rules:
1. Sharing of code between students is viewed as cheating and will receive appropriate action in accordance with
   University policy. 
2. It is acceptable for you to compare your results, and only your results, with other students to help debug your
   program. It is not acceptable to collaborate either on the code development or on the final experiments. 
3. You should do all your work in the C or C++ programming language, and should be written according to the C99 or C++11
   standards, using only the standard libraries.
4. Unfortunately experience has shown that there is a very high chance that there are errors in this project
   description.  The online version will be updated as errors are discovered.  It is your responsibility to check the
   website often and download new versions of this project description as they become available. 
5. A Makefile with the frontend will be given to you; you will only need to fill in the empty functions and any
   additional subroutines you will be using. You will also need to fill in the statistics structure that will be used to
   output the results.

Project Description
--------------------
In this project, we will be designing a simulator that implements a simplified version of Tomasulo's algorithm. You are
tasked to design the register alias table and reservation station. 

The trace file contains the following lines:
OP RD RS RT

* OP indicates the operation type. There will be multiple different types of instructions, each with a different latency
 (i.e. ADD takes 2 cycles, DIV takes 15 cycles, MEM takes 20).
* RD is the destination register number (i.e. 1 for R1)
* RS in the first source register number. If there is none (for example, an immediate value), the "number" is -1.
* RT is the second source register number.

You will be provided driver code that will convert the traces to instructions. You are only responsible for filling in
the following functions (along with any other classes and variables as needed).

Specification of simulator
---------------------------
Explanation of Functions you need to fill in:
``void scheduler_unified_init(int num_registers, int rs_size)``
This function is used if the type of scheduler uses a unified reservation station.

``void scheduler_per_fu_init(int num_registers, int rs1, int rs2, int rs3)``
This function is used if the type of scheduler uses per-FU reservation stations.

``bool scheduler_try_issue(op_type op, int dest, int src1, int src2)``
This function tries to issue a new instruction with the given arguments. If successful, return true, if not, return
false (i.e the RS is full).

``void scheduler_step()``
Increment the clock by 1 step

``void scheduler_start_ready()``
Start any instructions

``void scheduler_clear_completed()``
Clear completed instructions

``bool scheduler_completed()``
Return true if all instructions are completed and cleared.

``void scheduler_complete()``
Complete the scheduler and get any final stats


Statistics (output)
--------------------
The simulator outputs the following statistics after completion of the run:
1. Number of instructions
2. Number of cycles
3. Instructions Per Cycle (IPC)
4. Number of times the issue was stalled (issue failed)
5. Maximum number of instructions started at once
6. Maximum number of instructions completed (wrote back) at once
7. Maximum number of instructions active per FU

Validation
----------
Several test traces will be provided along with the correct output. You must run your simulator and debug it until it
matches 100% all the statistics in the validation outputs posted on the website. It is highly recommended that you first
try to work through each trace for each predictor type by hand.

Experiments
-----------
For each benchmark in the traces directory, design a instruction scheduler subject to the following goals: 
1. You have a total budget of 10 entries bits for the reservation stations (10 all in the unified RS, or X for
each RS that sums up to 10 entries)
2. The predictor should have the lowest possible IPC at the end.  You may vary any parameter (RS type, 
   entries per RS).

What to hand in via T-Square:
-----------------------------
1. The commented source code for the simulator program itself (you only need to provide branchsim.hpp, branchsim.cpp).
2. A document with the design results of the experiments for each trace file, with a persuasive argument of the choices
   that were made. (An argument may be as simple as an explanation of the search procedure used to find the designs and
   a statement about why the procedure is complete.) This argument should include output from runs of your program.
   (There are multiple answers for each trace file, so I will know which students have "collaborated" inappropriately!) 
3. Late submissions will be deducted 25%p per day

Grading
-------
0%    You do not hand in anything
+50%    Your simulator doesn't run, does not work, but you hand in significant commented code 
+35%    Your simulator matches the validation outputs
+15%    You ran all experiments and found the best-performing schedulers
