#include "local.h"

AI_mind *
ai_mind_create (void)
{
	AI_mind *self = ai_alloc (NULL, sizeof (*self));
	memset (self, 0, sizeof (*self));
	return self;
}
void
ai_mind_destroy (AI_mind *self)
{
	ai_free (self);
}
static bool
condition_find (AI_mind *self, const char *atom, uint32_t *index)
{
	for (uint32_t i = 0; i < self->nconds; i++)
	{
		if (strcmp (self->conds[i], atom))
		{
			continue;
		}
		*index = i;
		return true;
	}
	*index = 0;
	return false;
}
AI_condition
ai_mind_condition_get (AI_mind *self, const char *atom)
{
	uint32_t index = 0;
	if (condition_find (self, atom, &index))
	{
		return 1<<index;
	}
	return AI_INVALID;
}
AI_condition
ai_mind_condition_add (AI_mind *self, const char *atom)
{
	uint32_t index = 0;
	if (condition_find (self, atom, &index))
	{
		return 1<<index;
	}
	if (AI_MAX_CONDITIONS <= self->nconds)
	{
		return ai_throw (AI_ERR_MAXCONDS);
	}
	index = self->nconds;
	self->conds[index] = atom;
	self->nconds++;
	return 1<<index;
}
AI_action *
ai_mind_action_get (AI_mind *self, uint32_t index)
{
	if (self->nactions <= index)
	{
		return NULL;
	}
	return &self->actions[index];
}
void
ai_mind_action_add (AI_mind *self, AI_action *action)
{	/*Allocate space for the new action and copy it*/
	uint32_t index = self->nactions++;
	self->actions = ai_alloc (self->actions, self->nactions*sizeof (*action));
	self->actions[index] = *action;
}

/*TODO: these should be dynamic, but TLS complicates things a bit
with tear down. Worst case would be to pack these into an object and 
pass it to relevant functions and forego TLS all together. Could allocate
node memory from a special global allocator and clean up that way too.*/
AI_SHARED uint32_t _nnodes;
AI_SHARED AI_node _nodes[AI_MAX_NODES];
AI_SHARED uint32_t _nopened;
AI_SHARED AI_handle _opened[AI_MAX_NODES];
AI_SHARED uint32_t _nclosed;
AI_SHARED AI_handle _closed[AI_MAX_NODES];

/*Returns the number of unset bits between start and goal. This is analogous
to computing the linear distance between two points*/
static uint32_t
heuristic (AI_conds start, AI_conds goal)
{
	AI_condition x = (start.state&goal.enabled);
	AI_condition y = (goal.state&goal.enabled);
	AI_condition delta = x^y;
	uint32_t n = AI_MAX_CONDITIONS;
	while (delta)
	{
		delta >>= 1;
		n--;
	}
	return n;
}
static AI_node *
node_alloc (void)
{
	AI_node *n = NULL;
	if (AI_MAX_NODES <= _nnodes)
	{
		ai_throw (AI_ERR_MAXNODES);
	}
	n = _nodes + _nnodes++;
	return n;
}
static uint32_t
node_find (AI_handle *set, uint32_t num, AI_conds cond)
{
#ifdef AI_USE_MIN_HEAP
	for (uint32_t i = 0; i < num; i++)
#else
	for (uint32_t i = 0; i < num; i++)
#endif
	{
		if (!ai_conds_compare (&_nodes[set[i]].cond, &cond, ~0)) continue;
		return i;
	}
	return AI_INVALID;
}
static void
node_insert (AI_handle *set, uint32_t *num, AI_node *node)
{
#ifdef AI_USE_MIN_HEAP
	uint32_t len = *num;
	/*Climb the parents and swap them to maintain the heap as needed*/
	set[len] = (AI_handle)((ptrdiff_t)(node - _nodes));
	uint32_t n = ++len;
	while (n)
	{
		uint32_t p = (--n)>>1;
		if (_nodes[set[n]].f < _nodes[set[p]].f)
		{
			uint32_t swap = set[n];
			set[n] = set[p];
			set[p] = swap;
			n = p;
		}
		else break;
	}
	*num = len;
#else
	uint32_t len = *num;
	set[len++] = (AI_handle)((ptrdiff_t)(node - _nodes));
	*num = len;
#endif
}
static void
node_remove (AI_handle *set, uint32_t *num, AI_node *node)
{
#ifdef AI_USE_MIN_HEAP
	/*Move last element into the root position and sift down to restore
	the min heap invariant*/
	uint32_t len = *num;
	set[0] = set[--len];
	uint32_t i = 0;
	while (1)
	{
		uint32_t min = i;
		uint32_t l = (i<<1) + 1;
		uint32_t r = (i<<1) + 2;
		if (l < len && _nodes[set[l]].f < _nodes[set[min]].f) min = l;
		if (r < len && _nodes[set[r]].f < _nodes[set[min]].f) min = r;
		if (min != i)
		{
			uint32_t swap = set[min];
			set[min] = set[i];
			set[i] = swap;
			i = min;
		}
		else break;
	}
#else
	uint32_t len = *num;
	uint32_t ndx = node_find (set, len, node->cond);
	if (AI_INVALID == ndx)
	{
		return;
	}
	/*Remove from the set*/
	set[ndx] = set[--len];
	*num = len;
#endif
}
static AI_node *
node_min (AI_handle *set, uint32_t *num)
{
#ifdef AI_USE_MIN_HEAP
	return &_nodes[set[0]];
#else
	/*Scan for the lowest cost element*/
	uint32_t len = *num;
	uint32_t best = UINT32_MAX;
	AI_node *ret = NULL;
	for (uint32_t i = 0; i < len; i++)
	{
		AI_node *n = &_nodes[set[i]];
		uint32_t f = n->f;
		if (f < best)
		{
			best = f;
			ret = n;
		}
	}
	return ret;
#endif
}
uint32_t
ai_mind_solve (
	AI_mind *self,
	AI_plan *plan,
	AI_conds world,
	AI_conds goal,
	void *user
){	
	AI_node *root = NULL;
	/*Ensure there is actual work to do*/
	if (ai_conds_compare (&world, &goal, goal.enabled))
	{
		plan->mind = self;
		plan->head = 0;
		plan->used = 0;
		return 0;
	}
	/*Clear the node state*/
	_nnodes = 0;
	_nopened = 0;
	_nclosed = 0;
	/*Add initial node and begin solving*/
	root = node_alloc ();
	root->parent = NULL;
	root->cond = world;
	root->act = AI_INVALID;
	root->g = 0;
	root->f = heuristic (world, goal);	
	node_insert (_opened, &_nopened, root);
	while (_nopened != 0)
	{
		AI_node *n = node_min (_opened, &_nopened);
		/*Have we reached the goal?*/
		if (ai_conds_compare (&n->cond, &goal, goal.enabled))
		{/*Walk backward to the goal, adding each action into the plan
		as we go. NB: No attempt to reverse the order is made here, instead
		when executing the plan we read it backward. simple, right?*/ 
			AI_node *node = n;
			uint32_t i = 0;
			do
			{/*Ensure there is space for each addition, growing as needed*/
				if (plan->nacts <= i)
				{
					const size_t size = sizeof (plan->acts[0]);
					uint32_t nacts = plan->nacts + AI_PLAN_GRANULARITY;
					plan->acts = ai_alloc (plan->acts, size*nacts);
					plan->nacts = nacts;
				}
				plan->acts[i++] = (AI_handle)node->act;
				node = node->parent;
			}
			while (node->parent != NULL);
			plan->mind = self;
			plan->head = i;
			plan->used = i;
			return n->f;
		}
		node_remove (_opened, &_nopened, n);
		node_insert (_closed, &_nclosed, n);
		/*Check all edges from this node...
		There are two ways of interpretting this:
		
		1. the graph is implicit, containing all possible combinations of the 
		conditions as its nodes
		
		2. the nodes are the conditions, and we operate over sets of them
		
		Assuming 1., this way is probably the best, since the combinations
		can become large and maintaining explicit edges from all nodes is
		insane. If assuming 2., then the state becomes more manageable and
		this can be rewritten to possibly be more efficient*/
		for (uint32_t i = 0; i < self->nactions; i++)
		{
			AI_action *act = self->actions + i;
			/*Is this action a connecting edge?*/
			if (!ai_conds_compare (
				&act->entry, &n->cond, act->entry.enabled))
			{
				continue;
			}
			/*Ensure this action is possible*/
			if (act->precondition && !act->precondition (act, user))
			{
				continue;
			}
			uint32_t cost = n->g + act->cost;
			AI_conds entry = ai_conds_merge (&n->cond, &act->exit);
			/*Find the neighbour
			We have to search the closed and opened sets for this. Bummer.*/
			AI_node *node = NULL;
			uint32_t next = node_find (_opened, _nopened, entry);
			if (AI_INVALID != next) node = &_nodes[_opened[next]];
			else
			{
				next = node_find (_closed, _nclosed, entry);
				if (AI_INVALID != next) node = &_nodes[_closed[next]];
				else
				{/*This node hasn't been visited before*/
					node = node_alloc ();
					node->act = i;
					
					node->cond = entry;
					node->parent = n;
					node->g = cost;
					node->f = cost + heuristic (entry, goal);
					
					node_insert (_opened, &_nopened, node);
					continue;
				}
			}
			/*Take this node if it yields a cheaper path*/
			if (cost < node->g)
			{
				node->cond = entry;
				node->parent = n;
				node->g = cost;
				node->f = cost + heuristic (entry, goal);
				node = NULL;
			}
		}
	}
	/*No possible path*/
	return AI_INVALID;
}

