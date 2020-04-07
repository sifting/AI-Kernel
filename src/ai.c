#include "local.h"

AI_state *_ai;

AI_NORETURN int
ai_throw (uint32_t error)
{
	if (_ai->panic) _ai->panic (error);
	abort ();
}
void *
ai_alloc (void *ptr, size_t size)
{
	void *p = _ai->mem.realloc (ptr, size);
	if (NULL == p) ai_throw (AI_ERR_NOMEM);
	return p;
}
void
ai_free (void *ptr)
{
	_ai->mem.free (ptr);
}
int
ai_init (AI_allocator *allocator)
{
	static AI_allocator _default = {realloc, free};
	AI_state *ais = NULL;
	AI_allocator a;
	/*Select the proper allocator*/
	if (!allocator) a = _default;
	else a = *allocator;
	/*Create the global state*/
	ais = a.realloc (NULL, sizeof (*ais));
	if (!ais)
	{
		return -1;
	}
	memset (ais, 0, sizeof (*ais));
	ais->mem = a;
	/*Install the state*/
	_ai = ais;
	return 0;
}
void
ai_init_from_pointer (AI_state *ais)
{
	assert (ais != NULL && "Passed NULL to ai_init_from_pointer");
	_ai = ais;
}
void
ai_atpanic (AI_panic panic)
{
	_ai->panic = panic;
}
void
ai_shutdown (void)
{
	_ai->mem.free (_ai->actions);
	_ai->mem.free (_ai);
	_ai = NULL;
}
