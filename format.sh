#!/bin/sh

find . \( -iname "*.h" -o -iname "*.cpp" \) \! -ipath "*3rdparty*" | xargs clang-format -style=file -i
