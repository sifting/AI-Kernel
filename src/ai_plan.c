#include "local.h"

AI_action *
ai_plan_action (AI_plan *self, uint32_t index)
{
	if (self->used <= index)
	{
		return NULL;
	}
	if (NULL == self->mind)
	{
		return NULL;
	}
	return self->mind->actions + self->acts[self->used - index - 1];
}
int
ai_plan_step (AI_plan *self, void *user)
{	/*Is the plan empty?*/
	if (!self->head)
	{
		return AI_PLAN_COMPLETED;
	}
	/*Perform the action*/
	uint32_t index = self->acts[--self->head];
	AI_action *act = &self->mind->actions[index];
	if (act->perform)
	{
		return act->perform (act, user);
	}
	/*No routine to perform, so what else?*/
	return AI_PLAN_CONTINUING;
}
void
ai_plan_reset (AI_plan *self)
{
	self->nacts = AI_MIN_PLAN;
	self->acts = ai_alloc (self->acts, AI_MIN_PLAN); 
}
AI_plan *
ai_plan_create (void)
{
	AI_plan *self = ai_alloc (NULL, sizeof (*self));
	memset (self, 0, sizeof (*self));
	ai_plan_reset (self);
	return self;
}
void
ai_plan_destroy (AI_plan *self)
{
	ai_free (self->acts);
	ai_free (self);
}
