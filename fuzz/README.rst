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
persistent mode and by providing a dictionary of poke's grammar.

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

   (start as many fuzzers as you wish or as your system can take**


Behind the scenes
#################

**TODO**
