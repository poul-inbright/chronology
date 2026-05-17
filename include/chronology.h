#ifndef __CHRONOLOGY_H__
#define __CHRONOLOGY_H__

#include "conf.h"

__CHRONOLOGY_BEGIN_DECLS;

// # `cyclicacy_t`:
// - `filter_mask` a mask used by `cyclical_filter_test()` to check whether an occurrence is 
typedef struct chrono_cyclicacy {
    chrono_u32 filter_mask; 
    chrono_u32 data_bundle;
} chrono_cyclicacy_t;

// u32 month_adjust = (1 <<  0) | (0 <<  1) | (1 <<  2) | (0 <<  3) |\
// 				      (1 <<  4) | (0 <<  5) | (1 <<  6) | (1 <<  7) |\
// 				      (0 <<  8) | (1 <<  9) | (0 << 10) | (1 << 11);
#define __CHRONO_MONTH_ADJUST 0x00000ad5
#define __CHRONO_DAYS_IN_MONTH(M, Y) (30 + ((__CHRONO_MONTH_ADJUST >> (M)) & 1) + ((M) != 1 ? 0 : -2 + ((Y) % 100 == 0 ? ((Y) % 400 == 0) : ((Y) % 4 == 0))))

enum CHRONOC_FilterField {
	CHRONOC_FF_MDay  = 0,
	CHRONOC_FF_Month = 1,
	CHRONOC_FF_WDay  = 3,

    // - the code casts struct tm into an array
    // - to correctly index we need to adjust the value above by this value
    CHRONOC_FF_TMADJUST = 3,

    CHRONOC_FF_BITS     = 2
};

enum CHRONOC_MonthlyAnchor {
    CHRONOC_MA_NONE      = 0,
    CHRONOC_MA_FirstWDay = 1,
    CHRONOC_MA_LastWDay  = 2,
    CHRONOC_MA_NthWDay   = 3,  // NOTE: [1, ...] not [0, ...]
    CHRONOC_MA_EndRel    = 4,

    CHRONOC_MA_BITS      = 3
};

enum CHRONOC_BundleBits {
    CHRONOC_BUNDLE_SIZE = 32,

    CHRONOC_FF_SHIFT    = CHRONOC_BUNDLE_SIZE - CHRONOC_FF_BITS,
    CHRONOC_FF_MASK     = ((1 << CHRONOC_FF_BITS) - 1) << CHRONOC_FF_SHIFT,

    CHRONOC_Monthly     = 1 << (CHRONOC_FF_SHIFT - 1),

    CHRONOC_MA_SHIFT    = CHRONOC_FF_SHIFT - 1 - CHRONOC_MA_BITS,
    CHRONOC_MA_MASK     = ((1 << CHRONOC_MA_BITS) - 1) << CHRONOC_MA_SHIFT,

    CHRONOC_SCALAR_MASK = (1 << CHRONOC_MA_SHIFT) - 1,
    CHRONOC_SECOND_MASK = CHRONOC_Monthly - 1,

    CHRONOC_MASK        = CHRONOC_Monthly | CHRONOC_SECOND_MASK
};

// calculate when the first event trigger should trigger
//
// ARGUMENTS:
//     - cyclicacy: a valid cyclicacy_t structure with all the neccessary info
//     - start    : date and time when the very first instance should happen/should've happened
//     - now      : reference point from which to calculate
//
// RETURN: the first occurence of the event after 'now'
time_t chrono_cyclical_first(chrono_cyclicacy_t* cyclicacy, time_t start, time_t now);
time_t chrono_cyclical_next(chrono_cyclicacy_t* cyclicacy, time_t prev);
int    chrono_cyclical_filter_test(chrono_cyclicacy_t* cyclicacy, time_t now);

struct chronology;
struct chrono_trigger;

typedef void (*trig_action_t)(struct chronology* chrono, uint32_t trigIdx, time_t timeNow);
typedef void (*trig_del_cb_t)(struct chronology* chrono, struct chrono_trigger* start, uint32_t n);

struct chrono_trigger {
    time_t              nextProc;
    uint32_t            actionIdx;
    uint32_t            l_ctx;
    chrono_cyclicacy_t  cyclicacy;
};

typedef struct chrono_trigger chrono_trigger_t;

enum Chronology_Flags {
    Chronology_TrigListUpdated = 0x01
};

struct chronology {
    chrono_trigger_t*  trigList; 
    trig_action_t*     actionTab;
    trig_del_cb_t      onTrigDel;
    void*              g_ctx;
    uint32_t           trigNum;
    uint32_t           actionNum;
    uint8_t            delayTolerance;
    uint8_t            trigSpareLimit;
    uint8_t            trigSpareSlots;
    uint8_t            flags;
};

typedef struct chronology chronology_t;

void     chronology_init(chronology_t* chronology, time_t ref_now);
uint32_t chronology_update(chronology_t* chronology, time_t ref_now);
void     chronology_free(chronology_t* chronology);

void     chronology_schedule(chronology_t* chronology, time_t nextProc, uint32_t actionIdx, uint32_t l_ctx, chrono_cyclicacy_t cyclicacy);
void     chronology_cancel(chronology_t* chronology, uint32_t trigIdx);
uint32_t chronology_filter(chronology_t* chronology, int (*filter_fn)(const chrono_trigger_t* trig, void* g_ctx, void* arg), void* arg);

//  ===========================  Internal stuff =================================
//  (provided for extensibility)

// used for qsort internally
int      __chrono_cmp_triggers(const void* a, const void* b);

// NOTE: expects a sorted trigList in chronology
void     __chronology_cleanup(chronology_t* chronology, time_t ref_now);

__CHRONOLOGY_END_DECLS;

#endif
