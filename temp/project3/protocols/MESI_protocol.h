#ifndef _MESI_CACHE_H
#define _MESI_CACHE_H

#include "../sim/types.h"
#include "../sim/enums.h"
#include "../sim/module.h"
#include "../sim/mreq.h"
#include "protocol.h"

/** Cache states.  */
typedef enum {
    MESI_CACHE_I = 1, //invalid state
    MESI_CACHE_S, //shared state
    MESI_CACHE_E, //exclusive state
    MESI_CACHE_M, //modified state
    MESI_CACHE_ISE, //transitioning from invalid to shared OR exclusive state
    MESI_CACHE_IM, //transitioning from invalid to modified state
    MESI_CACHE_SM //transitioning from shared to modified state
} MESI_cache_state_t;

class MESI_protocol : public Protocol {
public:
    MESI_protocol (Hash_table *my_table, Hash_entry *my_entry);
    ~MESI_protocol ();

    MESI_cache_state_t state;
    
    void process_cache_request (Mreq *request);
    void process_snoop_request (Mreq *request);
    void dump (void);

    inline void do_cache_I (Mreq *request);
    inline void do_cache_S (Mreq *request);
    inline void do_cache_E (Mreq *request);
    inline void do_cache_M (Mreq *request);
    inline void do_cache_ISE (Mreq *request);
    inline void do_cache_IM (Mreq *request);
    inline void do_cache_SM (Mreq *request);

    inline void do_snoop_I (Mreq *request);
    inline void do_snoop_S (Mreq *request);
    inline void do_snoop_E (Mreq *request);
    inline void do_snoop_M (Mreq *request);
    inline void do_snoop_ISE (Mreq *request);
    inline void do_snoop_IM (Mreq *request);
    inline void do_snoop_SM (Mreq *request);
};

#endif // _MESI_CACHE_H
