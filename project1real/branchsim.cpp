#include "branchsim.hpp"

/**
 * XXX: You are welcome to define and set any global classes and variables as needed.
 */

//pointer to the pattern history table
int *pht;
//pointer to the history register table
int *hrt;
//global variable to track which predictor type to use at all stages
predictor_type currType;
//a range for the counter_bits. Ex: counter_bits 3 will set this number to 7 denoting 0-7
int counter_range;
//a range for the history_bits. Ex: history_bits 2 will set this number to 3 denoting 0-3
int history_range;
//index_mask initialized to keep uint64_t indexing within the length of however many bits num_entries is
static std::uint64_t index_mask;
//hist_value is a global variable used to track the current value of the bits in the hrt
int hist_value;

//length of the localHistory pht
int localHistoryPHTLength;
//length of the localHistory hrt
int localHistoryHRTLength;

/**
 * Subroutine for initializing the branch predictor. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @param[in]   ptype       The type of branch predictor to simulate
 * @param[in]   num_entries The number of entries a PC is hashed into
 * @param[in]   counter_bits The number of bits per counter
 * @param[in]   history_bits The number of bits per history
 * @param[out]  p_stats     Pointer to the stats structure
 */
void setup_predictor(predictor_type ptype, int num_entries, int counter_bits, int history_bits,
                     branch_stats_t* p_stats) {

	//initialize global variables across the board for all predictors
	//set currType to the predictor we are using
	currType = ptype;
	//set counter_range to the upper decimal number in the counter_bits range
	counter_range = (1 << counter_bits) - 1;
	//set history_range to the upper decimal number in the history_bits range
	history_range = history_bits - 1;
	//set up the index_mask so indexing will always be in range
	index_mask = (std::uint64_t)num_entries - 1;

	//if bimodal
	if (ptype == 'B') {
		//calculate storage_overhead -> size of pht
		p_stats->storage_overhead = num_entries * counter_bits;
		//initialize pht, we're using decimal numbers so single array.
		//set all values to -1 to represent no prior entry
		pht = new int [num_entries];
		for (int i = 0; i < num_entries; i++) {
			pht[i] = -1;
		}

		//if gshare
	} else if (ptype == 'G') {
		//if num_entries and history_bits are compatable sizes
		if (num_entries == 1 << history_bits) {
			//initialize pht and hrt to appropriate 1D arrays
			pht = new int [num_entries];
			hrt = new int [history_bits];
			//set pht values to -1 for no prior entry
			for (int i = 0; i < num_entries; i++) {
				pht[i] = -1;
			}
			//set hrt values to 0 for not taken
			for (int i = 0; i < history_range + 1; i++) {
				hrt[i] = 0;
			}
			//calculate gshare overhead -> size of pht + # of history bits
			p_stats->storage_overhead = num_entries * counter_bits + history_bits;
		}

		//if local history
	} else if (ptype == 'L') {
		//calculate local history overhead -> size of pht per # of history bits + # of history entries * history bits
		p_stats->storage_overhead = (1 << history_bits) * counter_bits * num_entries + history_bits * num_entries;

		//set global lengths for pht and hrt to be tracked
		localHistoryPHTLength = num_entries * (1 << history_bits) + 1;
		localHistoryHRTLength = num_entries * history_bits;

		//initialize pht and hrt
		pht = new int [localHistoryPHTLength];
		hrt = new int [localHistoryHRTLength];

		//set pht to -1 for no prior entry
		for (int i = 0; i < localHistoryPHTLength; i++) {
			pht[i] = -1;
		}
		//set hrt to 0 for not taken
		for (int i = 0; i < localHistoryHRTLength; i++) {
			hrt[i] = 0;
		}

		//if two level adaptive
	} else if (ptype == 'T') {
		//calculate overhead for two level adaptive -> size of hrt + size of prt (counter bits * # of combinations of history bits)
		p_stats->storage_overhead = counter_bits * (1 << history_bits) + history_bits * num_entries;

		//initialize pht and hrt
		pht = new int [(1 << history_bits)];
		hrt = new int [history_bits * num_entries];

		//set pht to -1 for no prior entry
		for (int i = 0; i < (1 << history_bits); i++) {
			pht[i] = -1;
		}

		//set hrt to 0 for not taken
		for (int i = 0; i < history_bits * num_entries; i++) {
			hrt[i] = 0;
		}
	}
}

/**
 * Subroutine that queries the branch predictor for the branch direction.
 * XXX: You're responsible for completing this routine
 *
 * @param[in]   pc          The PC value of the branch instruction.
 * @param[out]  p_stats     Pointer to the stats structure
 *
 * @return                  Either TAKEN ('T'), or NOT_TAKEN ('N')
 */
branch_dir predict_branch(std::uint64_t pc, branch_stats_t* p_stats) {
	//increase branches count in stats
	p_stats->num_branches++;

	//if bimodal
	if (currType == 'B') {
		//mask pc to ensure we don't index out of range
		std::uint64_t index = pc & index_mask;
		//grab element in pht at pc index
		int element = pht[index];

		//if this is the first time we're grabbing the element, set it to weakly taken and put weakly taken in the pht
		if (element == -1) {
			element = (counter_range / 2) + 1; //1 above half Ex: 4/7 -> weakly taken
			pht[index] = element;
		}
		// if element is holding a value that predicts taken (upper half), predict taken and update stat
		if (element > (counter_range / 2)) {
			p_stats->pred_taken++;
			return TAKEN;
			//else, predict not taken and update stat
		} else {
			p_stats->pred_not_taken++;
			return NOT_TAKEN;
		}

		// if gshare
	} else if (currType == 'G') {
		//set value from the history table to 0
		hist_value = 0;
		//loop through all bits located in the history table and perform leftshifts to get values of each bit
		//sum all values to get history value
		for (int i = history_range; i >= 0; i--) {
			hist_value += hrt[i] << i;
		}

		//take history value and xor with pc to get index for pht
		std::uint64_t index = (pc ^ hist_value) & index_mask;
		int element = pht[index];

		//if this is the first time we're grabbing the element, set it to weakly taken and put weakly taken in the pht
		if (element == -1) {
			element = (counter_range / 2) + 1; //1 above half Ex: 4/7 -> weakly taken
			pht[index] = element;
		}

		// if element is holding a value that predicts taken (upper half), predict taken and update stat
		if (element > (counter_range / 2)) {
			p_stats->pred_taken++;
			return TAKEN;
			//else, predict not taken and update stat
		} else {
			p_stats->pred_not_taken++;
			return NOT_TAKEN;
		}

	} else if (currType == 'L') {
		//set value from the history table to 0
		hist_value = 0;
		//initialize dist to the index of the current branch in the history table
		int dist = (pc * (history_range + 1)) & index_mask;
		//loop through all bits located in the history table and perform leftshifts to get values of each bit
		//sum all values to get history value
		for (int i = dist + history_range; i >= dist; i--) {
			hist_value += hrt[i] << (i - dist);
		}

		//initialize phtDist to the index of the current branch in the pht table and grab element
		int phtDist = (pc * localHistoryPHTLength) & index_mask;
		int element = pht[phtDist + hist_value];

		//if this is the first time we're grabbing the element, set it to weakly taken and put weakly taken in the pht
		if (element == -1) {
			element = (counter_range / 2) + 1; //1 above half Ex: 4/7 -> weakly taken
			pht[phtDist + hist_value] = element;
		}
		// if element is holding a value that predicts taken (upper half), predict taken and update stat
		if (element > (counter_range / 2)) {
			p_stats->pred_taken++;
			return TAKEN;
			//else, predict not taken and update stat
		} else {
			p_stats->pred_not_taken++;
			return NOT_TAKEN;

		}

	} else if (currType == 'T') {
		//set value from the history table to 0
		hist_value = 0;
		//initialize dist to the index of the current branch in the history table
		int dist = (pc * (history_range + 1)) & index_mask;
		for (int i = dist + history_range; i >= dist; i--) {
			hist_value += hrt[i] << (i - dist);
		}
		//grab element from pht at hist_value index
		int element = pht[hist_value];

		//if this is the first time we're grabbing the element, set it to weakly taken and put weakly taken in the pht
		if (element == -1) {
			element = (counter_range / 2) + 1;//1 above half Ex: 4/7 -> weakly taken
			pht[hist_value] = element;
		}
		// if element is holding a value that predicts taken (upper half), predict taken and update stat
		if (element > (counter_range / 2)) {
			p_stats->pred_taken++;
			return TAKEN;
			//else, predict not taken and update stat
		} else {
			p_stats->pred_not_taken++;
			return NOT_TAKEN;
		}

	}
	return TAKEN;
}

/**
 * Subroutine for updating the branch predictor. The branch predictor needs to be notified whether
 * it's prediction was right or wrong.
 * XXX: You're responsible for completing this routine
 *
 * @param[in]   pc          The PC value of the branch instruction.
 * @param[in]   actual      The actual direction of the branch
 * @param[in]   predicted   The predicted direction of the branch (from predict_branch)
 * @param[out]  p_stats     Pointer to the stats structure
 */
void update_predictor(std::uint64_t pc, branch_dir actual, branch_dir predicted, branch_stats_t* p_stats) {

	//check to see if prediction was correct, if it was, update p_stats
	if (actual == predicted) {
		p_stats->correct++;
	}

	//if bimodal
	if (currType == 'B') {
		//mask pc in order to obtain index for pht
		std::uint64_t index = pc & index_mask;
		//grab element from pht
		int element = pht[index];

		//if the branch wasn't actually taken and the value in the pht is above 0, decrease the value by one
		if (actual == NOT_TAKEN) {
			if (element > 0) {
				pht[index] = element - 1;
			}
			//else if the branch was taken and the value in the pht isn't the max value in the counter_range, increase value by 1
		} else if (actual == TAKEN) {
			if (element < counter_range) {
				pht[index] = element + 1;
			}
		}
	}

	//if gshare
	if (currType == 'G') {
		//set history register table value to 0
		hist_value = 0;
		//loop through hrt to find history branch value
		for (int i = history_range; i >= 0; i--) {
			hist_value += hrt[i] << i;
		}
		//update hrt by shifting the values to the next highest index while overwriting the highest index value
		for (int i = history_range; i > 0; i--) {
			hrt[i] = hrt[i-1];
		}
		//if branch wasn't taken, set hrt lowest index to 0
		if (actual == NOT_TAKEN) {
			hrt[0] = 0;
			//else set hrt lowest index to 1
		} else {
			hrt[0] = 1;
		}

		//set index for the pht to the xor of the hrt value and the pc, mask it in order to prevent indexing out of range
		std::uint64_t index = (pc ^ hist_value) & index_mask;
		//grab element in pht
		int element = pht[index];
		//if the branch wasn't actually taken and the value in the pht is above 0, decrease the value by one
		if (actual == NOT_TAKEN) {
			if (element > 0) {
				pht[index] = element - 1;
			}
			//else if the branch was taken and the value in the pht isn't the max value in the counter_range, increase value by 1
		} else if (actual == TAKEN) {
			if (element < counter_range) {
				pht[index] = element + 1;
			}
		}
	}

	//if local history architecture
	if (currType == 'L') {
		//set history register table value to 0
		hist_value = 0;
		//initialize dist to the index of the current branch in the history table
		int dist = (pc * (history_range + 1)) & index_mask;
		//loop through hrt to find history branch value
		for (int i = dist + history_range; i >= dist; i--) {
			hist_value += hrt[i] << (i - dist);
		}
		//update hrt by shifting the values to the next highest index while overwriting the highest index value
		for (int i = dist + history_range; i >= dist; i--) {
			hrt[i] = hrt[i-1];
		}
		//if branch wasn't taken, set hrt lowest index in that branch to 0
		if (actual == NOT_TAKEN) {
			hrt[dist] = 0;
			//else set hrt lowest index in that branch to 1
		} else {
			hrt[dist] = 1;
		}

		//set phtDist to the starting index of the pht related to the current branch
		int phtDist = (pc * localHistoryPHTLength) & index_mask;
		//grab element in pht
		int element = pht[phtDist + hist_value];

		//if the branch wasn't actually taken and the value in the pht is above 0, decrease the value by one
		if (actual == NOT_TAKEN) {
			if (element > 0) {
				pht[phtDist + hist_value] = element - 1;
			}
			//else if the branch was taken and the value in the pht isn't the max value in the counter_range, increase value by 1
		} else if (actual == TAKEN) {
			if (element < counter_range) {
				pht[phtDist + hist_value] = element + 1;
			}
		}
	}

	//if two level adaptive
	if (currType == 'T') {
		//set hrt variable to 0
		hist_value = 0;
		//obtain hrt starting, mask it to prevent indexing out of range
		int dist = (pc * (history_range + 1)) & index_mask;
		//loop through hrt to find history branch value
		for (int i = dist + history_range; i >= dist; i--) {
			hist_value += hrt[i] << (i - dist);
		}
		//update hrt by shifting the values to the next highest index while overwriting the highest index value
		for (int i = dist + history_range; i >= dist; i--) {
			hrt[i] = hrt[i-1];
		}
		//if branch wasn't taken, set hrt lowest index in that branch to 0
		if (actual == NOT_TAKEN) {
			hrt[dist] = 0;
			//else set hrt lowest index in that branch to 1
		} else {
			hrt[dist] = 1;
		}
		//grab element in pht
		int element = pht[hist_value];

		//if the branch wasn't actually taken and the value in the pht is above 0, decrease the value by one
		if (actual == NOT_TAKEN) {
			if (element > 0) {
				pht[hist_value] = element - 1;
			}
			//else if the branch was taken and the value in the pht isn't the max value in the counter_range, increase value by 1
		} else if (actual == TAKEN) {
			if (element < counter_range) {
				pht[hist_value] = element + 1;
			}
		}
	}
}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics.
 * XXX: You're responsible for completing this routine
 *
 * @param[out]  p_stats     Pointer to the statistics structure
 */
void complete_predictor(branch_stats_t *p_stats) {
	//correct/branches = prediction rate, so update misprediction rate to 1-prediction rate
	p_stats->misprediction_rate = 1 - ((double)p_stats->correct / (double)p_stats->num_branches);
	//set global variables back to 0
	counter_range = 0;
	history_range = 0;
	hist_value = 0;
}
