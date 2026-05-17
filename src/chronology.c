#include "../include/chronology.h"

time_t chrono_cyclical_first(chrono_cyclicacy_t* c, time_t start_time, time_t now_time){
	if(start_time > now_time){
		// NOTE: sadly no tests on the data (or maybe not a bug but a feature?)
		return start_time;
	}

    time_t result;

	if(c->data_bundle & CHRONOC_Monthly){
        int anchor       = (c->data_bundle & CHRONOC_MA_MASK) >> CHRONOC_MA_SHIFT;
        chrono_u32 scalar  = c->data_bundle & CHRONOC_SCALAR_MASK;
		struct tm start, now;
        int days_cur_mon;

		localtime_r(&start_time, &start);
		localtime_r(&now_time, &now);
		start.tm_mon   = now.tm_mon;
		start.tm_year  = now.tm_year;
        start.tm_mday  = now.tm_mday;
		start.tm_isdst = -1; // kindly as the system to guess :)

		days_cur_mon = __CHRONO_DAYS_IN_MONTH(now.tm_mon, now.tm_year);

        switch(anchor){
            case CHRONOC_MA_FirstWDay:
                // NOTE: this algorithm can be improved by using bit-masks

                // 1. adjust the wday to the correct one
                // 2. add 7 to avoid negative values
                // 3. scale down by 7
                // 4. -1 in '()' and +1 to avoid 0, and false 1
			    start.tm_mday = (now.tm_mday + 7 + start.tm_wday - now.tm_wday - 1) % 7 + 1;
            break;
            case CHRONOC_MA_LastWDay:
                // 1. adjust the wday
                // 2. scale up to the maximum

                // NOTE: don't care if the value is negative
                start.tm_mday = now.tm_mday + start.tm_wday - now.tm_wday;
                anchor = days_cur_mon - start.tm_mday; // reuse the variable
                start.tm_mday += anchor - anchor % 7; // remove whatever is extra
            break;
            case CHRONOC_MA_NthWDay:
                // 1. get the very first wday
                // 2. scale up

			    start.tm_mday = (now.tm_mday + 7 + start.tm_wday - now.tm_wday - 1) % 7 + 1;
                start.tm_mday += (scalar - 1) * 7;
            break;
            case CHRONOC_MA_EndRel:
			    start.tm_mday = days_cur_mon - scalar + 1;
            break;
        }
		result = mktime(&start);
		if(result < now_time){
			result = chrono_cyclical_next(c, result);
		}
	} else {
		unsigned long scalar = c->data_bundle & CHRONOC_SECOND_MASK;
        time_t diff = now_time - start_time + scalar - 1;
        result = scalar ? start_time + diff - diff % scalar : start_time;
	}
    return result;
}

time_t chrono_cyclical_next(chrono_cyclicacy_t* c, time_t ref_now){
	// NOTE: to account for DST, I have to actually do localtime_r() and grab the is_dst from there
	struct tm trig;
    localtime_r(&ref_now, &trig);
	if(c->data_bundle & CHRONOC_Monthly){
        int anchor       = (c->data_bundle & CHRONOC_MA_MASK) >> CHRONOC_MA_SHIFT;
        chrono_u32 scalar  = c->data_bundle & CHRONOC_SCALAR_MASK;
        int D, D2, d;

		trig.tm_year += 1900;
		D  = __CHRONO_DAYS_IN_MONTH(trig.tm_mon, trig.tm_year);
		trig.tm_mon += 1;
		trig.tm_year += trig.tm_mon / 12;
		trig.tm_mon = trig.tm_mon % 12;
		D2 = __CHRONO_DAYS_IN_MONTH(trig.tm_mon, trig.tm_year);
		d = trig.tm_mday;
		trig.tm_year -= 1900;

        switch(anchor){
            case CHRONOC_MA_FirstWDay:
			    d = 7 - (D - d) % 7;
            break;
            case CHRONOC_MA_LastWDay:
                d = D2 - (D2 + D - d) % 7;
            break;
            case CHRONOC_MA_NthWDay:
                d = scalar * 7 - (D - d) % 7;
            break;
            case CHRONOC_MA_EndRel:
			    d = D2 - scalar + 1;
            break;
        }
		trig.tm_mday = d;
	} else {
		#define DAY_IN_SECONDS 24 * 3600
		unsigned long long advance = c->data_bundle & CHRONOC_SECOND_MASK;
		trig.tm_mday += advance / DAY_IN_SECONDS;
		trig.tm_sec  += advance % DAY_IN_SECONDS;

        #undef DAY_IN_SECONDS
	}

	trig.tm_isdst = -1;
	return mktime(&trig);
}

int chrono_cyclical_filter_test(chrono_cyclicacy_t* c, time_t now){
    struct tm tm;
    localtime_r(&now, &tm);

    return (1 << (&tm.tm_sec)[((c->data_bundle & CHRONOC_FF_MASK) >> CHRONOC_FF_SHIFT) + CHRONOC_FF_TMADJUST]) & c->filter_mask;
}

// TODO: invalid actionIdx

#define ASSIGN_TRIGGER(A, B)       \
    (A).nextProc  = (B).nextProc;  \
    (A).actionIdx = (B).actionIdx; \
    (A).l_ctx     = (B).l_ctx;     \
    (A).cyclicacy = (B).cyclicacy

#define SWAP_TRIGGERS(A, B) {     \
    chrono_trigger_t T;           \
    ASSIGN_TRIGGER(T, A);         \
    ASSIGN_TRIGGER(A, B);         \
    ASSIGN_TRIGGER(B, T);         \
}

// NOTE: This function expects a sorted trigList
void __chronology_cleanup(chronology_t* chrono, time_t ref_now){
    chrono_u32 i;
    // do soft remove on every "stale" trigger so that the user can then handle them and the update code doesn't have to "false-fire" every time
    for(i = 0; i < chrono->trigNum && chrono->trigList[i].nextProc + chrono->delayTolerance < ref_now; i++){
        chrono->trigNum--;
        chrono->trigSpareSlots++;
        SWAP_TRIGGERS(chrono->trigList[i], chrono->trigList[chrono->trigNum]); 
    }
    if(i){
        if(chrono->onTrigDel){
            chrono->onTrigDel(chrono, &chrono->trigList[chrono->trigNum], i);
        } 
        qsort(chrono->trigList, chrono->trigNum, sizeof(chrono->trigList[0]), __chrono_cmp_triggers); 
        chrono->flags |= Chronology_TrigListUpdated;
    }

    if(chrono->trigSpareSlots > chrono->trigSpareLimit){
        chrono->trigSpareSlots = chrono->trigSpareLimit;
        chrono->trigList = (chrono_trigger_t*)CHRONOLOGY_REALLOC(chrono->trigList, sizeof(chrono_trigger_t) * (chrono->trigNum + chrono->trigSpareSlots));
        CHRONOLOGY_ASSERT(chrono->trigList != NULL);
    }
}

void chronology_init(chronology_t* chrono, time_t ref_now){
    CHRONOLOGY_ASSERT(chrono != NULL);

    chrono_u32 i;
    for(i = 0; i < chrono->trigNum; i++){
        chrono_trigger_t* trig = &chrono->trigList[i];
        trig->nextProc = chrono_cyclical_first(&trig->cyclicacy, trig->nextProc, ref_now);
    }

    qsort(chrono->trigList, chrono->trigNum, sizeof(chrono->trigList[0]), __chrono_cmp_triggers); 
    __chronology_cleanup(chrono, ref_now);
}

chrono_u32 chronology_update(chronology_t* chrono, time_t ref_now){
    CHRONOLOGY_ASSERT(chrono != NULL);

    chrono_u32 procs = 0;
iterate:
    chrono->flags &= ~Chronology_TrigListUpdated;
    for(chrono_u32 i = 0; i < chrono->trigNum && chrono->trigList[i].nextProc <= ref_now; i++){
        chrono_trigger_t* trig = &chrono->trigList[i];
        time_t procTime = trig->nextProc;
        // update now, since the code action code might modify everything
        if(trig->cyclicacy.data_bundle & CHRONOC_MASK){
            trig->nextProc = chrono_cyclical_next(&trig->cyclicacy, trig->nextProc);
        }else{
            // mark for removal later
            trig->nextProc = 0;
        }
        if(procTime + chrono->delayTolerance >= ref_now && chrono_cyclical_filter_test(&trig->cyclicacy, procTime)){
            procs++;
            chrono->actionTab[trig->actionIdx](chrono, i, ref_now);
            if(chrono->flags & Chronology_TrigListUpdated){
                goto iterate;
            }
        }
    }
    
    qsort(chrono->trigList, chrono->trigNum, sizeof(chrono->trigList[0]), __chrono_cmp_triggers); 
    __chronology_cleanup(chrono, ref_now);

    return procs;
}

void chronology_free(chronology_t* chrono){
    CHRONOLOGY_ASSERT(chrono != NULL);

    if(chrono->trigList){
        (void)CHRONOLOGY_REALLOC(chrono->trigList, 0);
    }
    if(chrono->actionTab){
        (void)CHRONOLOGY_REALLOC(chrono->actionTab, 0);
    }
    chrono->trigList = NULL;
    chrono->actionTab = NULL;
    chrono->trigNum = chrono->actionNum = 0;
}

void chronology_schedule(chronology_t* chrono, time_t nextProc, chrono_u32 actionIdx, chrono_u32 l_ctx, chrono_cyclicacy_t cyclicacy){
    CHRONOLOGY_ASSERT(chrono != NULL);

    if(chrono->trigSpareSlots == 0){
        chrono->trigSpareSlots = chrono->trigSpareLimit;
        chrono->trigList = CHRONOLOGY_REALLOC(chrono->trigList, sizeof(chrono_trigger_t) * (chrono->trigNum + chrono->trigSpareSlots)); 
        CHRONOLOGY_ASSERT(chrono->trigList != NULL);
    }

    chrono_trigger_t* trig = &chrono->trigList[chrono->trigNum];
    trig->nextProc  = nextProc;
    trig->actionIdx = actionIdx;
    trig->l_ctx     = l_ctx;
    trig->cyclicacy = cyclicacy;

    chrono->trigNum++;
    
    chrono->flags |= Chronology_TrigListUpdated;
    qsort(chrono->trigList, chrono->trigNum, sizeof(chrono->trigList[0]), __chrono_cmp_triggers); 
}

void chronology_cancel(chronology_t* chrono, chrono_u32 trigIdx){
    CHRONOLOGY_ASSERT(chrono != NULL);

    chrono->trigNum--;
    if(chrono->onTrigDel){
        chrono->onTrigDel(chrono, &chrono->trigList[trigIdx], 1);
    }
    ASSIGN_TRIGGER(chrono->trigList[trigIdx], chrono->trigList[chrono->trigNum]);
    chrono->flags |= Chronology_TrigListUpdated;

    qsort(chrono->trigList, chrono->trigNum, sizeof(chrono->trigList[0]), __chrono_cmp_triggers); 
    __chronology_cleanup(chrono, time(NULL));
}

chrono_u32 chronology_filter(chronology_t* chrono, int (*filter_fn)(const chrono_trigger_t* trig, void* g_ctx, void* arg), void* arg){
    CHRONOLOGY_ASSERT(chrono    != NULL);
    CHRONOLOGY_ASSERT(filter_fn != NULL);

    chrono_u32 removed = 0; 
    for(chrono_u32 i = 0; i < chrono->trigNum; i++){
        if(filter_fn(&chrono->trigList[i], chrono->g_ctx, arg) == 0){
            chrono->trigNum--;
            SWAP_TRIGGERS(chrono->trigList[i], chrono->trigList[chrono->trigNum]);
            removed++;
            i--;
        }
    } 

    if(removed){
        if(chrono->onTrigDel){
            chrono->onTrigDel(chrono, &chrono->trigList[chrono->trigNum], removed);
        }
        qsort(chrono->trigList, chrono->trigNum, sizeof(chrono->trigList[0]), __chrono_cmp_triggers); 
        __chronology_cleanup(chrono, time(NULL));
    }
    return removed;
}

int __chrono_cmp_triggers(const void* _a, const void* _b){
    time_t a = ((const chrono_trigger_t*)_a)->nextProc;
    time_t b = ((const chrono_trigger_t*)_b)->nextProc;
    // NOTE: the most resilliant version with no side-effects
    //       most of the adequate compilers manage to optimize this part nicely
    return (a > b) - (a < b);
}
