#ifndef __CHRONOLOGY_CONF_H__
#define __CHRONOLOGY_CONF_H__

#ifdef __cplusplus
    #define __CHRONOLOGY_BEGIN_DECLS extern "C" {
    #define __CHRONOLOGY_END_DECLS   }
#else
    #define __CHRONOLOGY_BEGIN_DECLS
    #define __CHRONOLOGY_END_DECLS
#endif

__CHRONOLOGY_BEGIN_DECLS;

#define CHRONOLOGY_VERSION_MAJOR 1
#define CHRONOLOGY_VERSION_MINOR 0
#define CHRONOLOGY_VERSION_DATE  "15.05.2026"

// # Dependencies
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define CHRONOLOGY_REALLOC         realloc
#define CHRONOLOGY_ASSERT          assert
#define CHRONOLOGY_QSORT           qsort
#define CHRONOLOGY_MEMSET          memset

#define CHRONO_LUA_EXTRA_ACTIONS   0
#define CHRONO_LUA_SPARE_TRIGGERS  16
#define CHRONO_LUA_DELAY_TOLERANCE 180

typedef uint32_t chrono_u32;

__CHRONOLOGY_END_DECLS;

#endif
