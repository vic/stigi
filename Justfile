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

spec-budget:
    #!/bin/sh
    set -eu
    if [ -d vic/spec ]; then
        bytes=$(cat vic/spec/language.md vic/spec/names.md vic/spec/types.md vic/spec/facts.md vic/spec/capabilities.md vic/spec/memory.md vic/spec/obligations.md vic/spec/tenet-gate.md | wc -c)
        printf "S57 agent-reference budget: %s / 128000 bytes\n" "$bytes"
        test "$bytes" -le 128000
    fi

test: spec-budget
    #!/bin/sh
    set -eu
    mkdir -p build/obj build/obj_san build/obj_clang
    lib_srcs=$(ls src/*.c | grep -v '/main.c$')
    rel_objs=""
    san_objs=""
    clang_objs=""
    pids=""
    for s in $lib_srcs; do
        b=$(basename "$s" .c)
        gcc {{warn}} -O2 -DNDEBUG {{safe}} -Isrc -I{{tupu_inc}} -c "$s" -o "build/obj/${b}.o" &
        pids="$pids $!"
        rel_objs="$rel_objs build/obj/${b}.o"
        gcc {{warn}} -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all {{safe}} -Isrc -I{{tupu_inc}} -c "$s" -o "build/obj_san/${b}.o" &
        pids="$pids $!"
        san_objs="$san_objs build/obj_san/${b}.o"
        clang {{warn}} -O2 -DNDEBUG {{safe}} -Isrc -I{{tupu_inc}} -c "$s" -o "build/obj_clang/${b}.o" &
        pids="$pids $!"
        clang_objs="$clang_objs build/obj_clang/${b}.o"
    done
    for pid in $pids; do
        wait "$pid"
    done
    for t in tests/*_test.c; do
        name=$(basename "$t" .c)
        gcc {{warn}} -O2 -DNDEBUG {{safe}} -Isrc -I{{tupu_inc}} -o "build/${name}" "$t" $rel_objs {{tupu_lib}} -pthread
        timeout 30 "build/${name}"
        clang {{warn}} -O2 -DNDEBUG {{safe}} -Isrc -I{{tupu_inc}} -o "build/${name}_clang" "$t" $clang_objs {{tupu_lib}} -pthread
        timeout 30 "build/${name}_clang"
        gcc {{warn}} -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all {{safe}} -Isrc -I{{tupu_inc}} -o "build/${name}_san" "$t" $san_objs {{tupu_lib}} -pthread
        timeout 30 "build/${name}_san"
    done

clean:
    rm -rf build

# S47 Fil-C backstop: compile tupu + seed + all tests + emitted C11 binaries with Fil-C
filc filcc="/nix/store/m6dqwwfjpxnmf1gmka33mkvacgrrpxql-filc-cc-wrapper-/bin/cc" tupu_src="/home/vic/hk/tupu/src":
    #!/bin/sh
    set -eu
    mkdir -p build/filc/tupu build/filc/obj
    tupu_objs=""
    pids=""
    for s in {{tupu_src}}/*.c; do
        b=$(basename "$s" .c)
        if [ "$b" = "main" ]; then continue; fi
        "{{filcc}}" -std=c11 -Wall -Wextra -Werror -pedantic -O2 -DNDEBUG {{safe}} -I"{{tupu_src}}" -I"{{tupu_inc}}" -c "$s" -o "build/filc/tupu/${b}.o" &
        pids="$pids $!"
        tupu_objs="$tupu_objs build/filc/tupu/${b}.o"
    done
    lib_objs=""
    for s in src/*.c; do
        b=$(basename "$s" .c)
        if [ "$b" = "main" ]; then continue; fi
        "{{filcc}}" {{warn}} -O2 -DNDEBUG {{safe}} -Isrc -I{{tupu_inc}} -c "$s" -o "build/filc/obj/${b}.o" &
        pids="$pids $!"
        lib_objs="$lib_objs build/filc/obj/${b}.o"
    done
    for pid in $pids; do
        wait "$pid"
    done
    ar rcs build/filc/libtupu-filc.a $tupu_objs
    "{{filcc}}" {{warn}} -O2 -DNDEBUG {{safe}} -Isrc -I{{tupu_inc}} -o build/stigi-seed-filc src/main.c $lib_objs build/filc/libtupu-filc.a -pthread
    export FILCC="{{filcc}}"
    for t in tests/*_test.c; do
        name=$(basename "$t" .c)
        "{{filcc}}" {{warn}} -O2 -DNDEBUG {{safe}} -Isrc -I{{tupu_inc}} -o "build/filc/${name}" "$t" $lib_objs build/filc/libtupu-filc.a -pthread
        timeout 90 "build/filc/${name}"
    done
