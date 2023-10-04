# Cornell CS6120 Lesson 7 Mem2Reg reimplementation based on llvm-pass-skeleton

It simplifies small programs removing unneeded memory traffic using register allocation as an LLVM pass.
It's for LLVM 17. Assumes llvm@17 and cmake install on Mac, has also been adapted to llvm 14 on Windows.

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
    $ conda deactivate # if needed for clean linker environment on Mac or Linux
    $ turnt *.c        # may need a repeat to see clean build expected ouput
```
