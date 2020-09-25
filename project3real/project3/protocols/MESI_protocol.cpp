#include "MESI_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MESI_protocol::MESI_protocol (Hash_table *my_table, Hash_entry *my_entry)
		: Protocol (my_table, my_entry)
{
	//initialize state to invalid
	this->state = MESI_CACHE_I;
}

MESI_protocol::~MESI_protocol ()
{    
}

void MESI_protocol::dump (void)
{
		const char *block_states[5] = {"X","I","S","E","M"};
		fprintf (stderr, "MESI_protocol - state: %s\n", block_states[state]);
}

//check for valid cache state/call proper corresponding request function
void MESI_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
		case MESI_CACHE_I:  do_cache_I (request); break;
		case MESI_CACHE_ISE:  do_cache_ISE (request); break;
		case MESI_CACHE_IM:  do_cache_IM (request); break;
		case MESI_CACHE_SM:  do_cache_SM (request); break;
		case MESI_CACHE_S:  do_cache_S (request); break;
		case MESI_CACHE_E:  do_cache_E (request); break;
		case MESI_CACHE_M:  do_cache_M (request); break;
		default:
				fatal_error ("Invalid Cache State for MESI Protocol\n");
		}
}

//check for valid cache snoop state/call proper corresponding request function
void MESI_protocol::process_snoop_request (Mreq *request)
{
	switch (state) {
		case MESI_CACHE_I:  do_snoop_I (request); break;
		case MESI_CACHE_ISE:  do_snoop_ISE (request); break;
		case MESI_CACHE_IM:  do_snoop_IM (request); break;
		case MESI_CACHE_SM:  do_snoop_SM (request); break;
		case MESI_CACHE_S:  do_snoop_S (request); break;
		case MESI_CACHE_E:  do_snoop_E (request); break;
		case MESI_CACHE_M:  do_snoop_M (request); break;
		default:
			fatal_error ("Invalid Cache State for MESI Protocol\n");
		}
}

//cache request in the invalid state
inline void MESI_protocol::do_cache_I (Mreq *request)
{
	switch (request->msg) {
		// If we get a request from the processor we need to get the data
		case LOAD:
			//read
			/* Line up the GETS in the Bus' queue */
			send_GETS(request->addr);
			/* The IM state means that we have sent the GET message and we are now waiting
			 * on DATA
			 */
			state = MESI_CACHE_ISE;
			//  This is a cache miss 
			Sim->cache_misses++;
			break;
		case STORE:
			//write
			/* Line up the GETM in the Bus' queue */
			send_GETM(request->addr);
			/* The IM state means that we have sent the GET message and we are now waiting
			 * on DATA
			 */
			state = MESI_CACHE_IM;
			/* This is a cache miss */
			Sim->cache_misses++;
			break;
		default:
			request->print_msg (my_table->moduleID, "ERROR");
			fatal_error ("Client: I state shouldn't see this message\n");
		}
}

//cache request in the shared state
inline void MESI_protocol::do_cache_S (Mreq *request)
{
	switch (request->msg) {

	case LOAD:
    // if we need to read the data, set the line as shared and send to processor
		set_shared_line();
    send_DATA_to_proc(request->addr);
    break;
  case STORE:
  	//if we need to write the data
    /* Line up the GETM in the Bus' queue */
    send_GETM(request->addr);
    /* The SM state means that we have sent the GETM message and we are now waiting
     * on DATA, so we can respond back with our own data.
     */
    state = MESI_CACHE_SM;
    /* This is a cache miss */
    Sim->cache_misses++;
    break;
	default:
			request->print_msg (my_table->moduleID, "ERROR");
			fatal_error ("Client: S state shouldn't see this message\n");
	}
}

//cache request in the exclusive state
inline void MESI_protocol::do_cache_E (Mreq *request)
{
	switch (request->msg) {

	case LOAD:
    // if we need to read the data, send to processor
    send_DATA_to_proc(request->addr);
    break;
  case STORE:
    /* Line up the GETM in the Bus' queue */
    send_DATA_to_proc(request->addr);
    /* The SM state means that we have sent the GETM message and we are now waiting
     * on DATA, so we can respond back with our own data.
     */
    //begin transitioning to modified state
    state = MESI_CACHE_M;
    //this upgrade is not sent on the bus
    Sim->silent_upgrades++;
    break;
	default:
			request->print_msg (my_table->moduleID, "ERROR");
			fatal_error ("Client: S state shouldn't see this message\n");
	}
}

//cache request in the modified state
inline void MESI_protocol::do_cache_M (Mreq *request)
{
	switch (request->msg) {
	/* The M state means we have the data and we can modify it.  Therefore any request
	 * from the processor (read or write) can be immediately satisfied.
	 */
	case LOAD:
	case STORE:
		// Write or read, send data to processor
		send_DATA_to_proc(request->addr);
		break;
	default:
			request->print_msg (my_table->moduleID, "ERROR");
			fatal_error ("Client: M state shouldn't see this message\n");
	}
}

//cache request in the invalid state transitioning to shared state OR the exclusive state
inline void MESI_protocol::do_cache_ISE (Mreq *request)
{
	//do nothing, UNLESS we get a write or read request. If we recieve that request, error
	switch (request->msg) {
	case LOAD:
	case STORE:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor! IS");
	default:
				request->print_msg (my_table->moduleID, "ERROR");
				fatal_error ("Client: IS state shouldn't see this message\n");
	}
}

//cache request in the invalid state transitioning to modified state
inline void MESI_protocol::do_cache_IM (Mreq *request)
{
	//do nothing, UNLESS we get a write or read request. If we recieve that request, error
	switch (request->msg) {
	case LOAD:
	case STORE:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor! IS");
	default:
				request->print_msg (my_table->moduleID, "ERROR");
				fatal_error ("Client: IS state shouldn't see this message\n");
	}
}

//cache request in the shared state transitioning to modified state
inline void MESI_protocol::do_cache_SM (Mreq *request)
{
	//do nothing, UNLESS we get a write or read request. If we recieve that request, error
	switch (request->msg) {
	case LOAD:
	case STORE:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error("Should only have one outstanding request per processor! IS");
	default:
				request->print_msg (my_table->moduleID, "ERROR");
				fatal_error ("Client: IS state shouldn't see this message\n");
	}
}


//====================================================
//====================================================
//======================SNOOP=========================
//====================================================
//====================================================

//snoop request in the invalid state
inline void MESI_protocol::do_snoop_I (Mreq *request)
{
	//do nothing
	switch (request->msg) {
	case GETS:
	case GETM:
	case DATA:
		break;
	default:
			request->print_msg (my_table->moduleID, "ERROR");
			fatal_error ("Client: I state shouldn't see this message\n");
	}
}

//snoop request in the shared state
inline void MESI_protocol::do_snoop_S (Mreq *request)
{
	switch (request->msg) {
	//read on the bus
	case GETS:
		//set the line to shared and stay in shared state
		set_shared_line();
		break;
	case GETM:
		//bus write, change to invalid state
		state = MESI_CACHE_I;
		break;
	case DATA:
		fatal_error ("Should not see data for this line!  I have the line!");
		break;
	default:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error ("Client: S state shouldn't see this message\n");
	}
}

//snoop request in the exclusive state
inline void MESI_protocol::do_snoop_E (Mreq *request)
{
	switch (request->msg) {
	case GETS:
		//bus read, set shared line and send the data on the bus
		set_shared_line();
		send_DATA_on_bus(request->addr,request->src_mid);
		//transition to the shared state
		state = MESI_CACHE_S;
		break;
	case GETM:
		//bus write, send the data on the bus
		send_DATA_on_bus(request->addr,request->src_mid);
		//transition to the invalid state
		state = MESI_CACHE_I;
		break;
	case DATA:
		fatal_error ("Should not see data for this line!  I have the line!");
		break;
	default:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error ("Client: S state shouldn't see this message\n");
	}
}

//snoop request in the modified state
inline void MESI_protocol::do_snoop_M (Mreq *request)
{
	switch (request->msg) {
	case GETS:
		//bus read, set shared line and send the data on the bus
		set_shared_line();
		send_DATA_on_bus(request->addr,request->src_mid);
		//transition to the shared state
		state = MESI_CACHE_S;
		break;
	case GETM:
		//bus write, send the data on the bus
		send_DATA_on_bus(request->addr,request->src_mid);
		//transition to the invalid state
		state = MESI_CACHE_I;
		break;
	case DATA:
		fatal_error ("Should not see data for this line!  I have the line!");
		break;
	default:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error ("Client: M state shouldn't see this message\n");
	}
}

//snoop request in the invalid state transitioning to the shared state OR exclusive state
inline void MESI_protocol::do_snoop_ISE (Mreq *request)
{
	switch (request->msg) {
	case GETS:
	case GETM:
		/** While in IM we will see our own GETS or GETM on the bus.  We should just
		 * ignore it and wait for DATA to show up.
		 */
		break;
	case DATA:
		//send data to processor
		send_DATA_to_proc(request->addr);
		//if the line is shared
		if (get_shared_line()) {
			//transition to shared state
			state = MESI_CACHE_S;
		} else {
			//otherwise transition to exclusive state
			state = MESI_CACHE_E;
		}
		break;
	default:
		request->print_msg (my_table->moduleID, "ERROR");
		fatal_error ("Client: I state shouldn't see this message\n");
	}
}

//snoop request in the invalid state transitioning to the modified state
inline void MESI_protocol::do_snoop_IM (Mreq *request)
{
	switch (request->msg) {
		case GETS:
		case GETM:
			/** While in IM we will see our own GETS or GETM on the bus.  We should just
			 * ignore it and wait for DATA to show up.
			 */
			break;
		case DATA:
			send_DATA_to_proc(request->addr);
			state = MESI_CACHE_M;
			break;
		default:
					request->print_msg (my_table->moduleID, "ERROR");
					fatal_error ("Client: IM state shouldn't see this message\n");
	}
}

//snoop request in the shared state transitioning to the modified state
inline void MESI_protocol::do_snoop_SM (Mreq *request)
{
	switch (request->msg) {
	  case GETS:
	  	//set the line as shared
	    set_shared_line();
	    break;
	  case GETM:
	    /**
	     * Another cache wants the data so we send it to them and transition to
	     * Invalid since they will be transitioning to M.  When we send the DATA
	     * it will go on the bus the next cycle and the memory will see it and cancel
	     * its lookup for the DATA.
	     */
	    state = MESI_CACHE_IM;
	    break;
	  case DATA:
	  	//we recieved the data, send it to the processor and transition to modified state
	    send_DATA_to_proc(request->addr);
	    state = MESI_CACHE_M;
	    break;
	  default:
	    request->print_msg (my_table->moduleID, "ERROR");
	    fatal_error ("Client: SM state shouldn't see this message\n");
  }
}