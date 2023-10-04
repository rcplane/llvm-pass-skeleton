## Summary

[mem2reg pass](https://github.com/rcplane/llvm-pass-skeleton/blob/master/skeleton/Skeleton.cpp)

## Implementation

- We adapted the code from [here](https://www.zzzconsulting.se/2018/07/16/llvm-exercise.html#experiment) to develop a rudimentary `mem2reg` pass that promotes stack slots to variables. Specifically, many compiler frontends emit LLVM IR emulating mutation by reading and writing to a stack
  slot standing in for a temporary. This is technically SSA because, although it mutates memory, it does not reassign any variable.
- An `alloca` instruction stands in for a temporary if its only uses are stores; this ensures that no other memory location aliases it.
- After collecting all of the `alloca` instructions, which stand in for temporaries, we then perform phi insertion to insert phi nodes for the
  `alloca` to be converted into a temporary. We treat the initial set of defining blocks for an `alloca` as the set of blocks that write to that location, and use LLVM's `DominatorTreeAnalysis` pass and `ForwardIDFCalculator` to compute the dominance frontier; we then insert phi nodes into these blocks.
- We then proceed with the renaming step of SSA conversion, which is simplified by the fact that the code is already in SSA; stacks of variable names are replaced by stacks of values stored to a stack slot. For example, when we encounter `store data, loc`, where `loc` is known to be an `alloca`, we push `data` onto the stack of definitions for `loc`. Then, when we encounter `x = load loc`, where `loc` is known to be an `alloca`, we replace all uses of `x` with the top of the definitions stack for `loc`.

## Difficulties

One source of problems was dealing with LLVM. The skeleton pass we started with
was modeled as a module pass, but the mem2reg pass we wanted to implement was
per-function. There was a lot of googling involved to figure out the right way
to write our pass. This was made worse by the fact that LLVM had come out with a
"new" Pass Manager system, which rendered many resources we found (in the
context of the legacy PM) unhelpful.

We also ran into issues while programming that were exacerbated by the fact that
we were using C++. Anything in the pass that broke during execution would simply
be a segfault, maybe accompanied by a sometimes-useful stacktrace. There were
several bugs where the fix was easy and most of the time was spent tracking down
the problem.