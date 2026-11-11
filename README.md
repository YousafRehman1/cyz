# CS311 Operating Systems — Assignment 01 Mini Shell evaluation preparation

**Start here:** read `CS311_Evaluation_Master_Guide.md` (or the matching PDF at top level in the download pack).

## Files
- `src/shell.h`, `main.c`, `parser.c`, `builtins.c`, `exec.c`: complete commented shell.
- `src/Makefile` and `src/CMakeLists.txt`: two build systems.
- `practice/q01_...c` to `q08_...c`: independent, tested practice examples.
- `practice/libdemo/`: a real static/shared library example.
- `tests/run_tests.sh`: quick regression tests.

## Fastest first run (Linux / WSL)
```bash
cd src
make
make test
./mysh
```
Example commands inside mysh:
```text
pwd
cd /tmp
pwd
echo hello > a.txt
echo world >> a.txt
cat < a.txt
seq 10 | wc -l
sleep 10 &
ps
kill 12345
exit
```
**The number after `kill` must be a real PID displayed by `ps`.** Do not type the example PID blindly.

## Using official linenoise (assignment requirement)
The assignment specifically says to use **linenoise** from https://github.com/antirez/linenoise.
It is third-party code and is NOT bundled in this pack because the required upstream source files were not available in the build environment.
Download `linenoise.c` and `linenoise.h` from that repository, place them inside `src/`, and run:
```bash
cd src
make clean
make USE_LINENOISE=1
./mysh
```
With CMake:
```bash
cmake -S src -B build -DUSE_LINENOISE=ON
cmake --build build
./build/mysh
```
The default build uses `getline()` strictly as a self-contained learning/testing fallback; it does not by itself satisfy the assignment's linenoise-specific instruction.

## Important scope
The lexer handles `<`, `>`, `>>`, `|`, `&` even without surrounding spaces. It deliberately does **not** implement optional Bash features such as quotes, escapes, environment expansion, wildcard globbing, `&&`, `||`, or `2>`; they are outside the uploaded assignment. The shell's own `ps` lists tracked background children, not the system-wide process list. A `kill PID` targets only one of those tracked children.

## Build checks
```bash
cd src
make clean && make && make test
```
Standalone exercise:
```bash
cd practice
gcc -Wall -Wextra -g q04_two_pipes.c -o q04
./q04  # prints 10
```
Debugging:
```bash
cd src
gdb ./mysh
# or:
valgrind --leak-check=full ./mysh
```

**Source basis:** Uploaded CS311 Fall 2026 Assignment 01 and uploaded OS Evaluation Reference Sheet. Practice tasks are informed guesses, not confirmed exam questions.
