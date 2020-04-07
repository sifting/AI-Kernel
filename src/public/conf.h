#pragma once

/*Memory constraints for plans, in elements*/
#define AI_MIN_PLAN 32
#define AI_PLAN_GRANULARITY 16 /*Additions per resize*/

/*Memory constraints for search nodes, in elements*/
#define AI_MIN_NODES 64
#define AI_MAX_NODES 256 /*Logically the upper bound on a plan too*/
#define AI_NODES_GRANULARITY 32 /*Additions per resize*/

/*Define this if you need 64 conditions (def=32)*/
//#define AI_EXTENDED_CONDITIONS 1

/*Define this if you need 16 conditions*/
//#define AI_REDUCED_CONDITIONS 1

/*Search code will be implemented using a min heap when defined. For most 
usages this is ideal, but smaller problem sets may be faster without it*/
#define AI_USE_MIN_HEAP 1

/*When set the library will use thread local storage to be thread-friendly.
Without this set all thread state becomes global state, and execution should
be limited to a single thread*/
#define AI_USE_TLS 1
