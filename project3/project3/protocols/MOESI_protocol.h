#ifndef _MOESI_CACHE_H
#define _MOESI_CACHE_H

#include "../sim/types.h"
#include "../sim/enums.h"
#include "../sim/module.h"
#include "../sim/mreq.h"
#include "protocol.h"

/** Cache states.  */
typedef enum {
    MOESI_CACHE_I = 1, //invalid state
    MOESI_CACHE_S, //shared state
    MOESI_CACHE_E, //exclusive state
    MOESI_CACHE_O, //owner state
    MOESI_CACHE_M, //modified state
    MOESI_CACHE_ISE, //transitioning from invalid to shared OR exclusive state
    MOESI_CACHE_IM, //transitioning from invalid to modified state
    MOESI_CACHE_SM, //transitioning from shared to modified state
    MOESI_CACHE_OM //transitioning from owner to modified state
} MOESI_cache_state_t;

class MOESI_protocol : public Protocol {
public:
    MOESI_protocol (Hash_table *my_table, Hash_entry *my_entry);
    ~MOESI_protocol ();

    MOESI_cache_state_t state;
    
    void process_cache_request (Mreq *request);
    void process_snoop_request (Mreq *request);
    void dump (void);

    inline void do_cache_I (Mreq *request);
    inline void do_cache_S (Mreq *request);
    inline void do_cache_E (Mreq *request);
    inline void do_cache_O (Mreq *request);
    inline void do_cache_M (Mreq *request);
    inline void do_cache_ISE (Mreq *request);
    inline void do_cache_IM (Mreq *request);
    inline void do_cache_SM (Mreq *request);
    inline void do_cache_OM (Mreq *request);

    inline void do_snoop_I (Mreq *request);
    inline void do_snoop_S (Mreq *request);
    inline void do_snoop_E (Mreq *request);
    inline void do_snoop_O (Mreq *request);
    inline void do_snoop_M (Mreq *request);
    inline void do_snoop_ISE (Mreq *request);
    inline void do_snoop_IM (Mreq *request);
    inline void do_snoop_SM (Mreq *request);
    inline void do_snoop_OM (Mreq *request);
};

#endif // _MOESI_CACHE_H
