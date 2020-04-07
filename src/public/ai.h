#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "conf.h"

typedef struct _AI_action AI_action;

/*Define max number of modelling conditions. 
These are represented as bits internally.*/
#if defined (AI_EXTENDED_CONDITIONS)
#	define AI_MAX_CONDITIONS	64
	typedef uint64_t AI_condition;
#elif defined (AI_REDUCED_CONDITIONS)
#	define AI_MAX_CONDITIONS	16
	typedef uint16_t AI_condition;
#else
#	define AI_MAX_CONDITIONS	32
	typedef uint32_t AI_condition;
#endif

/*Adjust the node handles to be small as possible*/
#if AI_MAX_NODES <= UINT8_MAX
	typedef uint8_t AI_handle;
#elif AI_MAX_NODES <= UINT16_MAX
	typedef uint16_t AI_handle;
#else
	typedef uint32_t AI_handle;
#endif

/*Sentinel for bad values*/
#define AI_INVALID (~0)

/*Condition sets*/
typedef struct _AI_conds
{
	AI_condition state;
	AI_condition enabled; /*write to enable state bits, like an irq mask*/
}AI_conds;

static inline void
ai_conds_clear (AI_conds *conds)
{
	conds->state = 0;
	conds->enabled = 0;
}
static inline bool
ai_conds_compare (AI_conds *a, AI_conds *b, AI_condition enabled)
{
	return (a->state&enabled) == (b->state&enabled);
}
static inline void
ai_conds_write (AI_conds *dst, AI_condition set, bool state)
{
	if (state) dst->state |= set;
	else dst->state &= ~set;
	dst->enabled |= set;
} 
static inline AI_conds
ai_conds_merge (AI_conds *from, AI_conds *to)
{
	AI_conds u;
	AI_condition enabled = to->enabled;
	AI_condition disabled = enabled^(~0);
	u.state = (to->state&enabled)|(from->state&disabled);
	u.enabled = enabled|from->enabled;
	return u;
}

/*Plans store the result from minds*/
#define AI_PLAN_INTERRUPTED		-1
#define AI_PLAN_CONTINUING		0
#define AI_PLAN_COMPLETED		1
typedef struct _AI_plan
{
	struct _AI_mind *mind;
	uint32_t head;
	uint32_t nacts, used;
	AI_handle *acts;
}AI_plan;

AI_action *ai_plan_action (AI_plan *self, uint32_t index);
int ai_plan_step (AI_plan *self, void *user);
void ai_plan_reset (AI_plan *self);
AI_plan *ai_plan_create (void);
void ai_plan_destroy (AI_plan *self);

static inline uint32_t
ai_plan_length (AI_plan *self)
{
	return self->used;
}
static inline AI_action *
ai_plan_action_peek (AI_plan *self)
{
	return ai_plan_action (self, self->used - self->head);
}

/*Minds are the graphs defined by the conditions and actions known to it.
In the graph the conditions are the nodes, and the actions are the 
edges between them. It is worth noting that the edges are one to many,
instead of one to one as in a typical graph. Minds also map conditions to the
bits on AI_condition fields. Minds are shared state.
*/
typedef bool (*AI_precondition) (AI_action *, void *);
typedef int (*AI_perform) (AI_action *, void *);
typedef struct _AI_action
{
	uint32_t cost;
	AI_conds entry; /*Conditions needed to enter the action*/
	AI_conds exit; /*Conditions upon completion*/
	AI_precondition precondition;
	AI_perform perform;
	const char *name;
}AI_action;
typedef struct _AI_mind
{	/*List of available actions*/
	uint32_t nactions;
	AI_action *actions;
	/*List of known conditions*/
	uint32_t nconds;
	const char *conds[AI_MAX_CONDITIONS];
}AI_mind;

AI_mind *ai_mind_create (void);
void ai_mind_destroy (AI_mind *self);
AI_condition ai_mind_condition_get (AI_mind *self, const char *atom);
uint32_t ai_mind_condition_add (AI_mind *self, const char *atom);
AI_action *ai_mind_action_get (AI_mind *self, uint32_t index);
void ai_mind_action_add (AI_mind *self, AI_action *action);
uint32_t ai_mind_solve (
	AI_mind *self,
	AI_plan *plan,
	AI_conds world,
	AI_conds goal,
	void *user);
	
static inline uint32_t
ai_mind_condition_length (AI_mind *self)
{
	return self->nconds;
}
static inline const char *
ai_mind_condition_atom (AI_mind *self, uint32_t index)
{
	if (self->nconds <= index)
	{
		return "";
	}
	return self->conds[index];
}

/*Error handling*/
#define AI_ERR_NOMEM	0xdeaddead
#define AI_ERR_MAXCONDS	0xcafeca75
#define AI_ERR_MAXNODES	0xb19d00d
typedef void (*AI_panic) (uint32_t);

typedef struct _AI_allocator
{
	void *(*realloc) (void *, size_t);
	void (*free) (void *);
}AI_allocator;
typedef struct _AI_state
{
	AI_allocator mem;
	AI_panic panic;
	uint32_t nactions;
	AI_action *actions;
}AI_state;

extern AI_state *_ai;

int ai_init (AI_allocator *allocator);
void ai_init_from_pointer (AI_state *ais);
void ai_atpanic (AI_panic panic);
void ai_shutdown (void);
