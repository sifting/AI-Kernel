#pragma once
#include <stdnoreturn.h>
#include <assert.h>
#include "ai.h"

/*Set up thread local storage macro*/
#ifdef AI_USE_TLS
#	define AI_SHARED	_Thread_local
#else
#	define AI_SHARED
#endif
/*Macro this to something nice*/
#define AI_NORETURN		_Noreturn

/*A* state*/
typedef struct _AI_node
{
	struct _AI_node *parent;
	AI_conds cond;
	uint32_t g, f;
	uint32_t act;
}AI_node;

/*Shared routines*/
AI_NORETURN int ai_throw (uint32_t error);
void *ai_alloc (void *ptr, size_t size);
void ai_free (void *ptr);
