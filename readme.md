# AI

This implements a STRIPS-like kernel intended for use in modeling behaviour of 
characters and computer controlled opponents in video games.


The gist is that the world is modeled by a number of arbitrary conditions. The
conditions of the world may be influenced by actions. Each action has a set of
entry conditions necessary to be executed, and describes the set of conditions
that it has changed post execution. Conditions and Actions together form a node
graph, where the conditions are the nodes and the actions are the edges. Given
a starting set of conditions, the kernel then finds the most optimal plan of
action to affect the set of goal conditions in the world.


The big gain from implementing AI in this way is reduced complexity: Actions 
are implemented as discrete, shareable units of logic instead of sprawling,
bloated state machines which are prone to error and instability as they grow
in complexity.


## Building

Run `premake5` to generate the project files for your tools of choice. This
library was written in C11 and compiled against GCC 8.3.0. Premake can be
gotten [here](https://premake.github.io/) if you have not it installed.

You may like to set options in `src/public/conf.h` to suit your use.
 

## Usage

Three objects provide most of the functionality in this library: `AI_action`,
`AI_mind` and`AI_plan`: 


`AI_action`s are user defined operators that, when given a set of 
preconditions, are invoked to mutate the world into an desired state. There are
two user defined callbacks that may be set: the `precondition` callback is 
invoked when the `AI_mind` is considering its use, and the `perform` callback
which is invoked by `ai_plan_step` when exeuting a plan.


`AI_mind`s describe the set of conditions and actions that define 
the node graph and other bits needed to function. They are meant to be shared 
between their instances, and are largely immutable once defined.


`AI_plan`s hold the result produced by an `AI_mind`, and are responsible for 
executing it through successive calls to `ai_plan_step`. Care should be paid
attention to the return code of `ai_plan_step` since plans may be interrupted
or require time to complete before advancing.


In addition, it is worth mentioning that the conditions used to model the world
are given symbolically as strings. This is because the conditions used by an
action may map to different values between minds.


There are other minor structures as well, but for the most part they stay out
of the way. The best way to understand them, and everything else said here, is
to look at `src/main.c` for a basic example.


## Further reading

https://en.wikipedia.org/wiki/Stanford_Research_Institute_Problem_Solver

https://alumni.media.mit.edu/~jorkin/gdc2006_orkin_jeff_fear.pdf

