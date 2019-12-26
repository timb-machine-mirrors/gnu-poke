************
Fuzzing poke
************

Poke can be stress-tested using the `american fuzzy lop
<http://lcamtuf.coredump.cx/afl/>`_ fuzzer. While it is possible to use the
"classical" afl-gcc compiler wrapper, the results performance is too poor to
achieve any useful results. It is therefore highly recommended to use the gcc
plugin provided by `afl++ <https://github.com/vanhauser-thc/AFLplusplus>`_ or
afl's llvm mode, once poke can be built by clang.

We achieve decent fuzzing performance and good code coverage by using afl's
deferred initialization and by providing a dictionary of poke's grammar.

How to
######

0. Obtain AFL or AFL++, either by installing it from your distributions package
   manager or by building it from source.

1. Build an instrumented version of poke (optionally with address sanitizer)::

     $ export CFLAGS="-fsanitize=address -O3 -march=native -mtune=native -g3 -gstrict-dwarf""
     $ # for afl:
     $ export CC=afl-gcc CXX=afl-g++
     $ # for afl++:
     $ export CC=afl-gcc-fast CXX=afl-g++-fast
     $ pushd fuzz; make afl_gcc_whitelist; popd;
     $ export AFL_GCC_WHITELIST="$(realpath fuzz/afl_gcc_whitelist)"

     # if you want to use ASAN
     $ export AFL_USE_ASAN=1

     $ ./configure && make -j $(nproc)

2. Setup the initial test cases and the poke dictionary. For this you'll need to
   install GNU parallel for the input file minimization (and bring some time)::

     $ export AFL_PATH=/path/to/the/afl/binaries
     $ export POKEDATADIR=$(realpath ../src) POKESTYLESDIR=$(realpath ../etc)
     $ # when using ASAN:
     $ export ASAN_OPTIONS=detect_leaks=0:abort_on_error=1:symbolize=0
     $ cd fuzz
     $ make all

3. Start the fuzzer::

     $ ${AFL_PATH}/afl-fuzz -i in -o out -x ./pk.dict -m none -- ../src/poke --no-init-file -L @@

   You can omit the `-m none` part when you've built poke without ASAN. If your
   distribution ships the `cgcreate`, `cgexec` and `cgdelete` tools, then you
   are advised to use the `experimental/asan_cgroups/limit_memory.sh` script
   from afl's source tree.

   If you have more cores to spare, start a few fuzzers in parallel::

     $ ${AFL_PATH}/afl-fuzz -i in -o sync_dir -M fuzzer01 -x ./pk.dict -m none -- ${POKEDATADIR}/poke --no-init-file -L @@
     $ ${AFL_PATH}/afl-fuzz -i in -o sync_dir -S fuzzer02 -x ./pk.dict -m none -- ${POKEDATADIR}/poke --no-init-file -L @@
     $ ${AFL_PATH}/afl-fuzz -i in -o sync_dir -S fuzzer03 -x ./pk.dict -m none -- ${POKEDATADIR}/poke --no-init-file -L @@

   (start as many fuzzers as you wish or as your system can take)

4. Reduce the number of results (if you got a lot)::

     $ mkdir crashes reduced_crashes hangs
     $ for file in $(find sync_dir/ -path '*/crashes/id*'); do cp $file crashes/; done
     $ ${AFL_PATH}/afl-cmin -C -i crashes/ -o reduced_crashes/ -m none -- ${POKEDATADIR}/poke --no-init-file -L @@
     $ for file in $(find sync_dir/ -path '*/hangs/id*'); do cp $file hangs/; done

5. Rebuild poke with your ordinary compiler and check whether the crashes are
   still occurring. Then report them to the mailinglist, or even better: submit
   patches ;-).


Behind the scenes
#################

Running poke
************

We test poke's ability to execute scripts, by feeding it mutated input files
passed to the `-L` flag. Furthermore we pass the `--no-init-file` option, so
that poke does not waste time in reading the user's history from `~/.pokerc`.

The current setup very likely not testing the whole poke command language, as we
are only using the pickles from pokes test suite and discarding any commands
that are included in the comments.


Deferred Initialization
***********************

AFL's llvm mode and afl++'s gcc plugin support to defer the initialization of
afl's forkserver: the forkserver can be explicitly started once the target
binary has been initialized. We use this to defer the launch of the fork server
until poke has completely initialized itself (including reading in the standard
library, which gives us a boost of the fuzzing performance of a factor of
10-15.


Initial input generation
************************

AFL requires a set of initial valid inputs that are mutated to make the program
under test crash or otherwise misbehave. For that we re-use the input files from
poke's test suite and process them as follows:

1. Strip all comments from the files, as afl will otherwise mutate them too
   wasting a lot of computing power.

2. Minimize the set of the initial test cases to a set of unique files using
   `afl-cmin`.

3. Minimize each remaining test file via `afl-tmin` (this is done using GNU
   parallel, as it can take quite some time).


We explicitly opt to not use the more elaborate pickles from the `pickles`
subdirectory, as these are fairly long and cannot be effectively minimized by
`afl-tmin`. Instead they just make the fuzzer's job harder.


Dictionary generation
*********************

Fuzzing an interpreter of a highly structured input via the mutations that afl
supports (like byte flips, bit flips, etc.) is very ineffective, as most inputs
will be rejected very quickly and the fuzzer will take a long time to reach more
interesting parts of the code. Therefore afl includes the feature to utilize a
dictionary (see
https://lcamtuf.blogspot.com/2015/04/finding-bugs-in-sqlite-easy-way.html for a
blog post from afl's author), which is used to construct an input file that will
likely be "more valid" than one generated via random mutations.

This dictionary can be a subset of valid tokens or expressions of the tested
language. We create it using the python script `generate_afl_dict.py`, which
just takes all tokens defined in the bison parser's input file and prints them
out in a way supported by afl. Beside adding a few tokens that are not in the
`pkl-tab.y` file, no further post processing is necessary and this approach
appears to be good enough.


Using with ASAN
***************

Note: If you want to use ASAN + afl, just follow the above steps.

Address sanitizer is a very popular and extremely fast tool to find all kinds of
memory errors in programs. It can be used in conjunction with afl to also detect
memory issues that pop up during fuzzing, at a bearable performance hit.

Unfortunately, poke currently has some small memory leaks that are detected by
ASAN's leak sanitizer and thus would pollute the fuzzing reports. We therefore
recommend to disable leak sanitizer for now by setting the environment variable
`ASAN_OPTIONS` to `detect_leaks=0` (afl also requires the settings
`abort_on_error=1` and `symbolize=0`, therefore they are appended in the steps
above).


GCC Plugin whitelist
********************

AFL++'s gcc plugin supports explicitly white-listing certain files: only files
in the whitelist are instrumented, all other files are compiled as they are. By
default, we only instrument poke's source files in the `src/` subdirectory and
thereby exclude jitter.


Future work
###########

- libfuzzer is another popular fuzzer, that could be used to explicitly test
  parts of poke's internal API and not the whole program. This requires that
  poke can be built with clang and furthermore will need a custom mutator as
  described here:
  https://github.com/google/fuzzing/blob/master/docs/structure-aware-fuzzing.md

  This approach could yield even better and more isolated results than using
  afl's grammar.

- The initial test cases are currently being generated from the pickles in
  poke's test suite. That unfortunately excludes a lot of the available poke
  commands. Including them could lead to even more interesting, but maybe harder
  to control results.
