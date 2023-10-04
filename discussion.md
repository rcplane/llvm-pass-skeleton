## Summary

@rcplane , @Enochen , and @zachary-kent collaborated on this project.

[mem2reg pass](https://github.com/rcplane/llvm-pass-skeleton/blob/master/skeleton/Skeleton.cpp)

We were successfully able to modify C programs built with clang to [reduce](https://github.com/rcplane/llvm-pass-skeleton/blob/master/test/array_alloc_print.reduce) emitted llvm ir instruction count converting unnecessary memory stores to register usage while preserving execution correctness.

## Implementation

- We adapted the code from [here](https://www.zzzconsulting.se/2018/07/16/llvm-exercise.html#experiment) to develop a rudimentary `mem2reg` pass that promotes stack slots to variables. Specifically, many compiler frontends emit LLVM IR emulating mutation by reading and writing to a stack
  slot standing in for a temporary. This is technically SSA because, although it mutates memory, it does not reassign any variable.
- An `alloca` instruction stands in for a temporary if its only uses are stores; this ensures that no other memory location aliases it.
- After collecting all of the `alloca` instructions, which stand in for temporaries, we then perform phi insertion to insert phi nodes for the
  `alloca` to be converted into a temporary. We treat the initial set of defining blocks for an `alloca` as the set of blocks that write to that location, and use LLVM's `DominatorTreeAnalysis` pass and `ForwardIDFCalculator` to compute the dominance frontier; we then insert phi nodes into these blocks.
- We then proceed with the renaming step of SSA conversion, which is simplified by the fact that the code is already in SSA; stacks of variable names are replaced by stacks of values stored to a stack slot. For example, when we encounter `store data, loc`, where `loc` is known to be an `alloca`, we push `data` onto the stack of definitions for `loc`. Then, when we encounter `x = load loc`, where `loc` is known to be an `alloca`, we replace all uses of `x` with the top of the definitions stack for `loc`.
- Generative AI Use: ChatGPT4 September 25 version with Bing browsing enabled was used for [initial code modifications](https://chat.openai.com/share/ec1d2e54-f146-4669-addb-13354ad35795) intended as a reference implementation with some initial success in error reduction, but it failed to close the gap transitioning to function analysis manager refreshing for each function in the module. We did end up making near-complete rewrites of [skeleton/Skeleton.cpp](https://github.com/rcplane/llvm-pass-skeleton/compare/c0e03aa...master#diff-fc54fadf44224331c02cb32b2dc6bd90741cc2c201a6d30a1323006cd046caef#skeleton/Skeleton.cpp) with no further ChatGPT input after initial adaptation attempts. ChatGPT4 was quite useful in quickly generating a [few](https://chat.openai.com/share/c8abdf2c-4206-4d2c-8aba-79547cedb81f) [sample](https://chat.openai.com/share/92b19a67-3089-4af7-8cfa-66fd10c155af) testing C programs with desired behaviors in short order.

## Testing

[turnt.toml](https://github.com/rcplane/llvm-pass-skeleton/blob/master/test/turnt.toml)

- We implemented three sample C programs performing array operations to test different cases of memory usage behavior inculding allocation in loops and printing verifiable results.
- We implemented a substantial set of turnt tests to verify clean build output, consistent execution output, and reduction in emitted llvm ir instruction count.
- Our memory allocation in a loop test exposed a segmentation fault in an early implementation, and required copying out the command with filename inlining to debug in detail.
- Due to time constraints, we did not pursue a broader benchmark suite of sample C programs including standard library program sets but our existing tools should scale reasonably to adding more test cases. Our usage of C++ has not been fully checked for memory safety, another sacrifice due to time constraints. We considered importing Bril programs from llvm conversion to run our optimization pass but did not fully pursue this idea. 


## Difficulties

- One source of problems was dealing with LLVM. The skeleton pass we started with was modeled as a module pass, but the mem2reg pass we wanted to implement was per-function. There was a lot of googling involved to figure out the right way to write our pass. This was made worse by the fact that LLVM had come out with a "new" Pass Manager system in the intervening time since LLVM 6, which rendered many resources we found (in the context of the legacy PM) unhelpful.

We also ran into issues while programming that were exacerbated by the fact that
we were using C++. Anything in the pass that broke during execution would simply
be a segfault, maybe accompanied by a sometimes-useful stacktrace. There were
several bugs where the fix was easy and most of the time was spent tracking down
the problem. We attempted lldb debugging in Visual Studio Code but resorted to
the old fallback of print statements to isolate some issues, notably a need to
check for a Def stack empty condition in successor iteration.

