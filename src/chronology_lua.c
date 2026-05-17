#include "../include/chronology_lua.h"

// NOTE: kind of sad that l_ctx has to be a unique reference for every single object
//       not too effecient, probably only good for a sandbox

#define CHRONOLOGY_METATABLE "chrono.metatable"

static const luaL_Reg chronology_lib[] = {
    { "new",               lua_chrono_new              },
    { "repeat_monthly",    lua_chrono_repeat_monthly   },
    { "repeat_daily",      lua_chrono_repeat_daily     },
    { "repeat_sec",        lua_chrono_repeat_sec       },
    { "first_ocurrence",   lua_chrono_first_ocurrence  },
    { "next_ocurrence",    lua_chrono_next_ocurrence   },
    { "filter_test",       lua_chrono_filter_test      },
    { "ON_NTH_WDAY",       lua_chrono_ON_NTH_WDAY      },
    { "END_RELATIVE",      lua_chrono_END_RELATIVE     },
    {NULL, NULL}
};

static const luaL_Reg date_lib[] = {
    { "now",               lua_chrono_date_now        },
    { "parse",             lua_chrono_date_parse      },
    { "break_down",        lua_chrono_date_break_down },
    { "tostr",             lua_chrono_date_tostr      },
    {NULL, NULL}
};

static const luaL_Reg chronology_methods[] = {
    { "schedule",          lua_chrono_schedule      },
    { "cancel",            lua_chrono_cancel        },
    { "update",            lua_chrono_update        },
    { "next_trigtime",     lua_chrono_next_trigtime },
    { "__gc",              _lua_chrono_destroy      },
    {NULL, NULL}
};

LUAMOD_API int luaopen_chronology_lua(lua_State *lua){
	assert(lua != NULL);

    luaL_newlib(lua, chronology_lib);

    chrono_cyclicacy_t* c = (chrono_cyclicacy_t*)lua_newuserdatauv(lua, sizeof(chrono_cyclicacy_t), 0);
	assert(c != NULL);
    c->filter_mask = 0xFFFFFFFF;
    c->data_bundle = 0;
    lua_setfield(lua, -2, "DOES_NOT_REPEAT");

    lua_pushinteger(lua, 0);
    lua_setfield(lua, -2, "NO_ANCHOR");

    lua_pushinteger(lua, CHRONOC_MA_LastWDay << CHRONOC_MA_SHIFT);
    lua_setfield(lua, -2, "ON_LAST_WDAY");

    lua_pushinteger(lua, CHRONOC_MA_FirstWDay << CHRONOC_MA_SHIFT);
    lua_setfield(lua, -2, "ON_FIRST_WDAY");

    lua_pushinteger(lua, 0);
    lua_setfield(lua, -2, "ALLOW");

    lua_pushinteger(lua, 0xFFFFFFFF);
    lua_setfield(lua, -2, "DENY");

    lua_pushinteger(lua, CHRONOC_FF_MDay);
    lua_setfield(lua, -2, "MDAY");

    lua_pushinteger(lua, CHRONOC_FF_Month);
    lua_setfield(lua, -2, "MONTH");

    lua_pushinteger(lua, CHRONOC_FF_WDay);
    lua_setfield(lua, -2, "WDAY");

    lua_setglobal(lua, "chrono");

    luaL_newmetatable(lua, CHRONOLOGY_METATABLE);

    lua_pushstring(lua, "__index");
    lua_pushvalue(lua, -2); // Push the metatable itself onto the stack
    lua_settable(lua, -3); // metatable.__index = metatable

    luaL_setfuncs(lua, chronology_methods, 0);

    lua_pop(lua, 1); // pop the metatable

    luaL_newlib(lua, date_lib);
    lua_setglobal(lua, "date");

    return 0;
}

int lua_chrono_new(lua_State* lua){
    chronology_t* chrono = (chronology_t*)lua_newuserdatauv(lua, sizeof(chronology_t), 0);

    int arg_num = lua_gettop(lua);

    CHRONOLOGY_MEMSET(chrono, 0, sizeof(*chrono));

    chrono->trigSpareLimit = CHRONO_LUA_SPARE_TRIGGERS;
    chrono->actionNum      = 1 + CHRONO_LUA_EXTRA_ACTIONS;
    chrono->actionTab      = (trig_action_t*)CHRONOLOGY_REALLOC(NULL, sizeof(trig_action_t) * chrono->actionNum);
    chrono->actionTab[0]   = _lua_chrono_action_fn;
    chrono->g_ctx          = (void*)lua;
    chrono->onTrigDel      = _lua_chrono_on_trig_del;
    chrono->delayTolerance = arg_num > 1 ? lua_tointeger(lua, 2) : CHRONO_LUA_DELAY_TOLERANCE;

    luaL_getmetatable(lua, CHRONOLOGY_METATABLE);
    lua_setmetatable(lua, -2);

    return 1;
}

int lua_chrono_repeat_monthly(lua_State* lua){
    int n = lua_gettop(lua);

    if(n != 1 && n != 4){
        luaL_argerror(lua, 1, "invalid number of arguments to function");
    }

    chrono_cyclicacy_t* c = (chrono_cyclicacy_t*)lua_newuserdatauv(lua, sizeof(chrono_cyclicacy_t), 0);

    c->filter_mask = 0xFFFFFFFF;
    c->data_bundle = CHRONOC_Monthly | lua_tointeger(lua, 1) /* anchor */;

    if(n > 1){
        _lua_chrono_bake_repeat_filter(lua, c);
    }

    return 1;
}

int lua_chrono_repeat_daily(lua_State* lua){
    int n = lua_gettop(lua);

    if(n != 1 && n != 4){
        luaL_argerror(lua, 1, "invalid number of arguments to function");
    }

    chrono_cyclicacy_t* c = (chrono_cyclicacy_t*)lua_newuserdatauv(lua, sizeof(chrono_cyclicacy_t), 0);

    c->filter_mask = 0xFFFFFFFF;
    c->data_bundle = CHRONOC_SECOND_MASK & (lua_tointeger(lua, 1) * 3600 * 24) /* seconds */;

    if(n > 1){
        _lua_chrono_bake_repeat_filter(lua, c);
    }

    return 1;
}

int lua_chrono_repeat_sec(lua_State* lua){
    int n = lua_gettop(lua);

    if(n != 1 && n != 4){
        luaL_argerror(lua, 1, "invalid number of arguments to function");
    }

    chrono_cyclicacy_t* c = (chrono_cyclicacy_t*)lua_newuserdatauv(lua, sizeof(chrono_cyclicacy_t), 0);

    c->filter_mask = 0xFFFFFFFF;
    c->data_bundle = CHRONOC_SECOND_MASK & lua_tointeger(lua, 1) /* seconds */;

    if(n > 1){
        _lua_chrono_bake_repeat_filter(lua, c);
    }

    return 1;
}

int lua_chrono_first_ocurrence(lua_State* lua){
    if(lua_gettop(lua) != 3){
        luaL_error(lua, "%s", "invalid number of arguments");
    }

    chrono_cyclicacy_t* c = lua_touserdata(lua, 1);
    time_t start = lua_tointeger(lua, 2);
    time_t now   = lua_tointeger(lua, 3);

    luaL_argcheck(lua, c != NULL, 1, "result of a chrono.repeat_xxxxx() function is expected as an argument");

    time_t first = chrono_cyclical_first(c, start, now);

    lua_pushinteger(lua, first);

    return 1;
}

int lua_chrono_next_ocurrence(lua_State* lua){
    if(lua_gettop(lua) != 2){
        luaL_error(lua, "%s", "invalid number of arguments");
    }

    chrono_cyclicacy_t* c = lua_touserdata(lua, 1);
    time_t now            = lua_tointeger(lua, 2);

    luaL_argcheck(lua, c != NULL, 1, "result of a chrono.repeat_xxxxx() function is expected as an argument");

    time_t next = chrono_cyclical_next(c, now);
    lua_pushinteger(lua, next);

    return 1;
}

int lua_chrono_filter_test(lua_State* lua){
    if(lua_gettop(lua) != 2){
        luaL_error(lua, "%s", "invalid number of arguments");
    }

    chrono_cyclicacy_t* c = lua_touserdata(lua, 1);
    time_t now   = lua_tointeger(lua, 2);

    luaL_argcheck(lua, c != NULL, 1, "result of a chrono.repeat_xxxxx() function is expected as an argument");

    int result = chrono_cyclical_filter_test(c, now);
    lua_pushboolean(lua, result);

    return 1;
}

int lua_chrono_ON_NTH_WDAY(lua_State* lua){
    luaL_checktype(lua, 1, LUA_TNUMBER);

    lua_pushinteger(lua, (lua_tointeger(lua, 1) & CHRONOC_SCALAR_MASK) | (CHRONOC_MA_NthWDay << CHRONOC_MA_SHIFT));

    return 1;
}

int lua_chrono_END_RELATIVE(lua_State* lua){
    luaL_checktype(lua, 1, LUA_TNUMBER);

    lua_pushinteger(lua, (lua_tointeger(lua, 1) & CHRONOC_SCALAR_MASK) | (CHRONOC_MA_EndRel << CHRONOC_MA_SHIFT));

    return 1;
}

int lua_chrono_date_parse(lua_State* lua){
    luaL_checktype(lua, 1, LUA_TSTRING);

    struct tm time_st;
    CHRONOLOGY_MEMSET(&time_st, 0, sizeof(time_st));

	const char* timestr = lua_tolstring(lua, 1, NULL);
    sscanf(timestr, "%u.%u.%u %u:%u:%u", &time_st.tm_mday, &time_st.tm_mon, &time_st.tm_year, &time_st.tm_hour, &time_st.tm_min, &time_st.tm_sec);

    time_st.tm_isdst = -1; // signal to libc to handle this value
    time_st.tm_mon--;
    time_st.tm_year -= 1900;

	time_t result = mktime(&time_st);

	//LOG("%20s => {%02u, %02u, %4u, %02u, %02u, %02u} (%lu)", timestr, time_st.tm_mday, time_st.tm_mon, time_st.tm_year, time_st.tm_hour, time_st.tm_min, time_st.tm_sec, result);

    lua_pushinteger(lua, result);

    return 1;
}

int lua_chrono_date_now(lua_State* lua){
    lua_pushinteger(lua, time(NULL));
    return 1;
}

int lua_chrono_date_break_down(lua_State* lua){
    luaL_checktype(lua, 1, LUA_TNUMBER);

    time_t t = lua_tointeger(lua, 1);
    struct tm time_st;

    localtime_r(&t, &time_st);

    lua_newtable(lua);
    lua_pushinteger(lua, time_st.tm_sec);
    lua_setfield(lua, -2, "sec");
    lua_pushinteger(lua, time_st.tm_min);
    lua_setfield(lua, -2, "min");
    lua_pushinteger(lua, time_st.tm_hour);
    lua_setfield(lua, -2, "hour");
    lua_pushinteger(lua, time_st.tm_mday);
    lua_setfield(lua, -2, "mday");
    lua_pushinteger(lua, time_st.tm_mon + 1);
    lua_setfield(lua, -2, "mon");
    lua_pushinteger(lua, time_st.tm_year + 1900);
    lua_setfield(lua, -2, "year");
    lua_pushinteger(lua, time_st.tm_wday);
    lua_setfield(lua, -2, "wday");

    return 1;
}

int lua_chrono_date_tostr(lua_State* lua){
    luaL_checktype(lua, 1, LUA_TNUMBER);

    char strbuf[64];

    time_t t = lua_tointeger(lua, 1);
    struct tm time_st;

    localtime_r(&t, &time_st);

    strftime(strbuf, sizeof(strbuf), "%d.%m.%Y %H:%M:%S", &time_st);

    lua_pushstring(lua, strbuf);

    return 1;
}

int lua_chrono_schedule(lua_State* lua){
    chronology_t* chrono = _lua_chrono_check(lua);
    luaL_checktype(lua, 2, LUA_TTABLE);

    chrono_u32 n     = lua_rawlen(lua, 2);
    chrono_u32 start = chrono->trigNum;

    chrono->trigNum += n;
    chrono->trigList = (chrono_trigger_t*)CHRONOLOGY_REALLOC(chrono->trigList, sizeof(chrono_trigger_t) * chrono->trigNum);

    time_t now = time(NULL);

    for(chrono_u32 i = 0; i < n; i++){
        chrono_trigger_t* trig = &chrono->trigList[start + i];
        trig->actionIdx = 0; // always the same

        lua_rawgeti(lua, 2, i + 1);

        lua_getfield(lua, -1, "start");
        if(!lua_isnumber(lua, -1)){
            luaL_error(lua, "incorrect data at trigger #%u", i);
        }
        trig->nextProc = lua_tointeger(lua, -1);
        lua_pop(lua, 1); // pop the `start` off the stack

        lua_getfield(lua, -1, "recurrence");
        if(!lua_isuserdata(lua, -1)){
            luaL_error(lua, "incorrect data at trigger #%u", i);
        }
        chrono_cyclicacy_t* c = (chrono_cyclicacy_t*)lua_touserdata(lua, -1);
        trig->cyclicacy = *c;
        trig->nextProc  = chrono_cyclical_first(c, trig->nextProc, now);
        lua_pop(lua, 1); // pop the `recurrence` off the top

        lua_getfield(lua, -1, "action");
        if(!lua_isfunction(lua, -1)){
            luaL_error(lua, "incorrect data at trigger #%u", i);
        }
        trig->l_ctx = luaL_ref(lua, LUA_REGISTRYINDEX);
        lua_pop(lua, 1); // pop the current item off
    }

    CHRONOLOGY_QSORT(chrono->trigList, chrono->trigNum, sizeof(chrono->trigList[0]), __chrono_cmp_triggers);
    __chronology_cleanup(chrono, now);

    return 0;
}

int lua_chrono_cancel(lua_State* lua){
    chronology_t* chrono = _lua_chrono_check(lua);
    luaL_checktype(lua, 2, LUA_TNUMBER);

    chronology_cancel(chrono, lua_tointeger(lua, 2));

    return 0;
}

int lua_chrono_update(lua_State* lua){
    chronology_t* chrono = _lua_chrono_check(lua);
    luaL_checktype(lua, 2, LUA_TNUMBER);

    chronology_update(chrono, lua_tointeger(lua, 2));

    return 0;
}

int lua_chrono_next_trigtime(lua_State* lua){
    chronology_t* chrono = _lua_chrono_check(lua);

    lua_pushinteger(lua, chrono->trigNum ? chrono->trigList[0].nextProc : 0);

    return 1;
}

int _lua_chrono_destroy(lua_State* lua){
    chronology_t* chrono = _lua_chrono_check(lua);

    chronology_free(chrono);

    return 0;
}

chronology_t* _lua_chrono_check(lua_State* lua){
    void *ud = luaL_checkudata(lua, 1, CHRONOLOGY_METATABLE);
    luaL_argcheck(lua, ud != NULL, 1, "a `chronology` object expected");
    return (chronology_t*)ud;
}

void _lua_chrono_bake_repeat_filter(lua_State* lua, chrono_cyclicacy_t* c){
    chrono_u32 filter_type  = lua_tointeger(lua, 2);
    chrono_u32 filter_field = lua_tointeger(lua, 3);
    c->data_bundle |= (filter_field & ((1 << CHRONOC_FF_BITS) - 1)) << CHRONOC_FF_SHIFT;
    c->filter_mask = 0;

    luaL_checktype(lua, 4, LUA_TTABLE);
    chrono_u32 n = lua_rawlen(lua, 4);
    for(chrono_u32 i = 0; i < n; i++){
        lua_rawgeti(lua, 4, i + 1);
        c->filter_mask |= 1 << lua_tointeger(lua, -1);
        lua_pop(lua, 1);
    }
    c->filter_mask ^= filter_type;
}

void _lua_chrono_action_fn(chronology_t* chrono, chrono_u32 trigIdx, time_t timeNow){
    lua_State* lua = (lua_State*)chrono->g_ctx;
    chrono_trigger_t* trig = &chrono->trigList[trigIdx];
    lua_rawgeti(lua, LUA_REGISTRYINDEX, trig->l_ctx);
    lua_pushinteger(lua, trigIdx);
    lua_pushinteger(lua, timeNow);
    lua_call(lua, 2, 0);
}

void _lua_chrono_on_trig_del(chronology_t* chrono, chrono_trigger_t* start, chrono_u32 n){
    lua_State* lua = (lua_State*)chrono->g_ctx;
    while(n--){
        luaL_unref(lua, LUA_REGISTRYINDEX, start->l_ctx);
        start++;
    }
}
