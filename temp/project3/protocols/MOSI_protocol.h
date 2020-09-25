#ifndef _MOSI_CACHE_H
#define _MOSI_CACHE_H

#include "../sim/types.h"
#include "../sim/enums.h"
#include "../sim/module.h"
#include "../sim/mreq.h"
#include "protocol.h"

/** Cache states.  */
typedef enum {
    MOSI_CACHE_I = 1, //invalid state
    MOSI_CACHE_IS, //transitioning from invalid to shared state
    MOSI_CACHE_IM, //transitioning from invalid to modified state
    MOSI_CACHE_SM, //transitioning from shared to modified state
    MOSI_CACHE_OM, //transitioning from owner to modified state
    MOSI_CACHE_S, //shared state
    MOSI_CACHE_O, //owner state
    MOSI_CACHE_M //modified state
} MOSI_cache_state_t;

class MOSI_protocol : public Protocol {
public:
    MOSI_protocol (Hash_table *my_table, Hash_entry *my_entry);
    ~MOSI_protocol ();

    MOSI_cache_state_t state;
    
    void process_cache_request (Mreq *request);
    void process_snoop_request (Mreq *request);
    void dump (void);

    inline void do_cache_I (Mreq *request);
    inline void do_cache_IS (Mreq *request);
    inline void do_cache_IM (Mreq *request);
    inline void do_cache_S (Mreq * request);
    inline void do_cache_SM (Mreq * request);
    inline void do_cache_O (Mreq * request);
    inline void do_cache_OM (Mreq * request);
    inline void do_cache_M (Mreq *request);

    inline void do_snoop_I (Mreq *request);
    inline void do_snoop_IS (Mreq *request);
    inline void do_snoop_IM (Mreq *request);
    inline void do_snoop_S (Mreq *request);
    inline void do_snoop_SM (Mreq *request);
    inline void do_snoop_O (Mreq *request);
    inline void do_snoop_OM (Mreq *request);
    inline void do_snoop_M (Mreq *request);
};

#endif // _MOSI_CACHE_H
