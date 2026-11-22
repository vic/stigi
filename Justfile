safe := "-fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks"
warn := "-std=c11 -Wall -Wextra -Werror -pedantic"
tupu_inc := "/home/vic/hk/tupu/result/include"
tupu_lib := "/home/vic/hk/tupu/result/lib/libtupu.a"

default: build

build:
    mkdir -p build
    gcc {{warn}} -O2 -DNDEBUG {{safe}} -I{{tupu_inc}} -o build/stigi-seed src/*.c {{tupu_lib}} -pthread

clang:
    mkdir -p build
    clang {{warn}} -O2 -DNDEBUG {{safe}} -I{{tupu_inc}} -o build/stigi-seed-clang src/*.c {{tupu_lib}} -pthread

san:
    mkdir -p build
    gcc {{warn}} -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all {{safe}} -I{{tupu_inc}} -o build/stigi-seed-san src/*.c {{tupu_lib}} -pthread

test:
    #!/bin/sh
    set -eu
    mkdir -p build/obj build/obj_san
    lib_srcs=$(ls src/*.c | grep -v '/main.c$')
    rel_objs=""
    san_objs=""
    for s in $lib_srcs; do
        b=$(basename "$s" .c)
        gcc {{warn}} -O2 -DNDEBUG {{safe}} -Isrc -I{{tupu_inc}} -c "$s" -o "build/obj/${b}.o" &
        rel_objs="$rel_objs build/obj/${b}.o"
        gcc {{warn}} -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all {{safe}} -Isrc -I{{tupu_inc}} -c "$s" -o "build/obj_san/${b}.o" &
        san_objs="$san_objs build/obj_san/${b}.o"
    done
    wait
    for t in tests/*_test.c; do
        name=$(basename "$t" .c)
        gcc {{warn}} -O2 -DNDEBUG {{safe}} -Isrc -I{{tupu_inc}} -o "build/${name}" "$t" $rel_objs {{tupu_lib}} -pthread
        timeout 30 "build/${name}"
        gcc {{warn}} -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all {{safe}} -Isrc -I{{tupu_inc}} -o "build/${name}_san" "$t" $san_objs {{tupu_lib}} -pthread
        timeout 30 "build/${name}_san"
    done

clean:
    rm -rf build
