#ifndef __CHRONOLOGY_LUA_H__
#define __CHRONOLOGY_LUA_H__

#include "chronology.h"
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>

__CHRONOLOGY_BEGIN_DECLS;

LUAMOD_API int luaopen_chronology_lua(lua_State *L);

// ========================== chrono.xxxxx() functions =======================

int lua_chrono_new(lua_State* lua);

int lua_chrono_repeat_monthly(lua_State* lua);
int lua_chrono_repeat_daily(lua_State* lua);
int lua_chrono_repeat_sec(lua_State* lua);
int lua_chrono_first_ocurrence(lua_State* lua);
int lua_chrono_next_ocurrence(lua_State* lua);
int lua_chrono_filter_test(lua_State* lua);
int lua_chrono_ON_NTH_WDAY(lua_State* lua);
int lua_chrono_END_RELATIVE(lua_State* lua);

// ========================== date.xxxxx() functions =======================

int lua_chrono_date_now(lua_State* lua);
int lua_chrono_date_parse(lua_State* lua);
int lua_chrono_date_break_down(lua_State* lua);
int lua_chrono_date_tostr(lua_State* lua);

// ========================== chrono:xxxxx() methods =======================

// chrono:schedule()
int lua_chrono_schedule(lua_State* lua);
int lua_chrono_cancel(lua_State* lua);
int lua_chrono_update(lua_State* lua);
int lua_chrono_next_trigtime(lua_State* lua);
int _lua_chrono_destroy(lua_State* lua);

// ========================== internals =======================

chronology_t* _lua_chrono_check(lua_State* lua);
void _lua_chrono_bake_repeat_filter(lua_State* lua, chrono_cyclicacy_t* c);
void _lua_chrono_action_fn(chronology_t* chrono, chrono_u32 trigIdx, time_t timeNow);
void _lua_chrono_on_trig_del(chronology_t* chrono, chrono_trigger_t* start, chrono_u32 n);

__CHRONOLOGY_END_DECLS;

#endif
