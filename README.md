# Cornell CS6120 Lesson 7 Mem2Reg reimplementation based on llvm-pass-skeleton

It simplifies small programs removing unneeded memory traffic using register allocation as an LLVM pass.
It's for LLVM 17.

Build:
```
    $ cd llvm-pass-skeleton
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ cd ..
```
Run:
```
    $ clang -fpass-plugin=`ls build/skeleton/SkeletonPass.*` something.c
```
Turnt test:
```
    $ cd test
    $ vim turnt.toml   # Check your clang prefix
    $ turnt *.c        # seg fault on build pass with array_alloc_print.c known issue
```
