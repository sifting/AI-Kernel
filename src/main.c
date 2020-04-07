#include <stdio.h>
#include <assert.h>
#include "ai/ai.h"

/*Condition atoms*/
#define NOT(x) "-"x
#define IS_HUNGRY "is_hungry"
#define HAS_NUMBER "has_number"
#define HAS_RECIPE "has_recipe"
#define HAS_TARGET "has_target"
#define HAS_FOOD "has_food"
#define HAS_PHONE "has_phone"
#define HAS_MONEY "has_money"
#define IS_DESPARATE "is_desparate"

typedef struct _AI_action_recipe
{
	const char *name;
	uint32_t cost;
	const char **entry;
	const char **exit;
	AI_precondition precondition;
	AI_perform perform;
}AI_action_recipe;

/*Debugging illustration: print out the plan*/
static void
print_plan (AI_plan *plan)
{
	uint32_t cost = 0;
	AI_conds dbg;
	for (uint32_t i = 0; i < ai_plan_length (plan); i++)
	{
		AI_action *action = ai_plan_action (plan, i);
		cost += action->cost;
		dbg = ai_conds_merge (&dbg, &action->exit);
		printf ("f = %i, %s, c = { ", cost, action->name);
		/*Print the conditions*/
		AI_condition mask = dbg.enabled;
		uint32_t i = 0;
		while (mask)
		{
			const char *cond = plan->mind->conds[i];
			if (mask&1)
			{
				if ((1<<i)&dbg.state) printf ("%s ", cond);
				else printf ("~%s ", cond);
			}
			mask >>= 1;
			i++;
		}
		printf ("}\n");			
	}
}
/*Helper to add actions to minds*/
static void
action_add (AI_mind *mind, AI_action_recipe *recipe)
{
	AI_action action;
	memset (&action, 0, sizeof (action));
	action.cost = recipe->cost;
	/*Add entry conditions*/
	for (uint32_t i = 0; recipe->entry[i] != NULL; i++)
	{
		const char *str = recipe->entry[i];
		bool enabled = true;
		if (*str == '-')
		{
			enabled = false;
			str++;
		}
		AI_condition b = ai_mind_condition_add (mind, str);
		ai_conds_write (&action.entry, b, enabled);
	}
	/*Add exit conditions*/
	for (uint32_t i = 0; recipe->exit[i] != NULL; i++)
	{
		const char *str = recipe->exit[i];
		bool enabled = true;
		if (*str == '-')
		{
			enabled = false;
			str++;
		}
		AI_condition b = ai_mind_condition_add (mind, str);
		ai_conds_write (&action.exit, b, enabled);
	}
	action.precondition = recipe->precondition;
	action.perform = recipe->perform;
	action.name = recipe->name;
	/*Teach the mind the new action*/
	ai_mind_action_add (mind, &action);
}

int
main (int argc, char **argv)
{/*Define a few recipes. Recipes get digested into mind-specific actions.
	This is because there is a limited number of conditions which we may
	use to model the world, so each mind only memorises the ones it needs
	to perform its actions*/
	AI_action_recipe order = {
		.name = "order pizza",
		.cost = 2,
		.entry = (const char *[]){IS_HUNGRY, HAS_NUMBER, HAS_PHONE, HAS_MONEY, NULL},
		.exit = (const char *[]){HAS_FOOD, NOT(HAS_MONEY), NULL},
		.precondition = NULL,
		.perform = NULL
	};
	AI_action_recipe book = {
		.name = "search phonebook",
		.cost = 2,
		.entry = (const char *[]){NOT(HAS_NUMBER), NULL},
		.exit = (const char *[]){HAS_NUMBER, NULL},
		.precondition = NULL,
		.perform = NULL
	};
	AI_action_recipe phone = {
		.name = "get phone",
		.cost = 1,
		.entry = (const char *[]){NOT(HAS_PHONE), NULL},
		.exit = (const char *[]){HAS_PHONE, NULL},
		.precondition = NULL,
		.perform = NULL
	};
	AI_action_recipe call = {
		.name = "call mom for recipe",
		.cost = 6,
		.entry = (const char *[]){HAS_PHONE, NULL},
		.exit = (const char *[]){HAS_RECIPE, NULL},
		.precondition = NULL,
		.perform = NULL
	};
	AI_action_recipe bake = {
		.name = "bake pie",
		.cost = 4,
		.entry = (const char *[]){IS_HUNGRY, HAS_RECIPE, NULL},
		.exit = (const char *[]){HAS_FOOD, NULL},
		.precondition = NULL,
		.perform = NULL
	};
	AI_action_recipe candy = {
		.name = "take candy from baby",
		.cost = 1,
		.entry = (const char *[]){HAS_TARGET, IS_DESPARATE, IS_HUNGRY, NULL},
		.exit = (const char *[]){HAS_FOOD, NULL},
		.precondition = NULL,
		.perform = NULL
	};
	AI_action_recipe find = {
		.name = "find baby",
		.cost = 1,
		.entry = (const char *[]){NOT(HAS_TARGET), NULL},
		.exit = (const char *[]){HAS_TARGET, NULL},
		.precondition = NULL,
		.perform = NULL
	};
	AI_action_recipe eat = {
		.name = "eat food",
		.cost = 1,
		.entry = (const char *[]){HAS_FOOD, NULL},
		.exit = (const char *[]){NOT(HAS_FOOD), NOT(IS_HUNGRY), NULL},
		.precondition = NULL,
		.perform = NULL
	};
	/*Initialise the AI library*/
	if (ai_init (NULL))
	{
		printf ("Failed to initialise AI library!\n");
		return EXIT_FAILURE;
	}
	/*Create a mind object and add some actions to it*/
	AI_mind *mind = ai_mind_create ();
	action_add (mind, &order);
	action_add (mind, &bake);
	action_add (mind, &candy);
	action_add (mind, &find);
	action_add (mind, &book);
	action_add (mind, &phone);
	action_add (mind, &call);
	action_add (mind, &eat);
	/*Ensure that there are flags supplied, or else print out available ones*/
	if (argc < 2)
	{
		printf ("Please supply a set of conditions.\n");
		printf ("Available conditions are:\n");
		for (uint32_t i = 0; i < ai_mind_condition_length (mind); i++)
		{
			printf ("\t%s\n", ai_mind_condition_atom (mind, i));
		}
		printf ("Conditions may be prefixed with - to negate them\n");
		printf ("Use : to switch write destination to goal\n");
		printf ("Ex. usage: is_hungry : -is_hungry\n");
		ai_mind_destroy (mind);
		ai_shutdown ();
		return EXIT_FAILURE;
	}
	/*Create condition sets*/
	AI_conds world, goal;
	ai_conds_clear (&world);
	ai_conds_clear (&goal);
	AI_conds *dst = &world;
	for (int i = 1; i < argc; i++)
	{
		const char *p = argv[i];
		bool flag = true;
		if (*p == ':')
		{
			dst = &goal;
			continue;
		}
		if (*p == '-')
		{
			flag = false;
			p++;
		}
		if (&world == dst) printf ("Writing condition to world: %s\n", argv[i]);
		else printf ("Writing condition to goal: %s\n", argv[i]);
		ai_conds_write (dst, ai_mind_condition_get (mind, p), flag);
	}
	/*Solve for a plan*/
	AI_plan *plan = ai_plan_create ();
	if (AI_INVALID == ai_mind_solve (mind, plan, world, goal, NULL))
	{
		printf ("No plan possible!\n");
		goto Cleanup;
	}
	/*Execute the plan*/
	printf ("Doing the plan...\n");
	AI_action *action = NULL;
Execute:
	action = ai_plan_action_peek (plan);
	switch (ai_plan_step (plan, NULL))
	{
	case AI_PLAN_CONTINUING:
		printf ("Executing action '%s'...\n", action->name);
		world = ai_conds_merge (&world, &action->exit);
		goto Execute;
	case AI_PLAN_COMPLETED:
		printf ("Plan was completed!\n");
		break;
	case AI_PLAN_INTERRUPTED:
		printf ("Plan was interrupted!\n");
		break;
	default:
		assert (0 && "Unknown return code!");
	}
	/*Print the plan just to show it in detail*/
	printf ("The plan in detail:\n");
	print_plan (plan);
Cleanup:
	/*Clean up everything*/
	ai_plan_destroy (plan);
	ai_mind_destroy (mind);
	ai_shutdown ();
	return EXIT_SUCCESS;
}
