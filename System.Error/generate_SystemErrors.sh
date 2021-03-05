#! /bin/bash

echo -e '// This file was generated by generate_SystemErrors.sh. Do not edit.\n' > SystemErrors.h
echo | gcc -c - -include /usr/include/errno.h -E -dMM | grep '#define E' | sed -e 's/^#define[[:space:]]*\([^[:space:]]*\)[[:space:]].*/  SE_\1 = \1,/' | sort -u >> SystemErrors.h
