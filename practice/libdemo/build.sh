#!/usr/bin/env bash
set -euo pipefail
# Static library (.a): code is copied at link time.
gcc -Wall -Wextra -c util.c -o util.o
ar rcs libutil.a util.o
gcc -Wall -Wextra main.c ./libutil.a -o app_static
./app_static
# Shared library (.so): requires a runtime loader to locate .so.
gcc -Wall -Wextra -fPIC -c util.c -o util_pic.o
gcc -shared -o libutil.so util_pic.o
gcc -Wall -Wextra main.c -L. -lutil -o app_shared
LD_LIBRARY_PATH=. ./app_shared
