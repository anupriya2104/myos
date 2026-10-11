#!/usr/bin/env bash
# Boots the kernel headless in QEMU, types commands over the serial port,
# and checks the output. Needs qemu-system-i386.
set -u
cd "$(dirname "$0")/.."

run() {   # run "<commands...>" -> serial output
  (sleep 2; for c in "$@"; do printf '%s\r' "$c"; sleep 0.7; done) |
    timeout 12 qemu-system-i386 -kernel build/myos.elf -serial stdio -display none -m 128 2>/dev/null |
    tr -d '\r'
}

pass=0; fail=0
check() { if grep -q -- "$2" <<<"$OUT"; then echo "PASS  $1"; pass=$((pass+1)); else echo "FAIL  $1"; fail=$((fail+1)); fi; }

OUT=$(run "mem" "heaptest" "translate 0xb8000" "translate 0xDEADB000" "sleep 500" "echo hello kernel")
check "boots and enables interrupts" "Interrupts enabled"
check "reads E820 memory map"        "usable"
check "paging translates mapped va"  "phys 0x000b8000"
check "unmapped va reported"         "is not mapped"
check "heap coalesces after free"    "coalesced OK"
check "timer sleep (500 ms = 50)"    "slept 5[01] ticks"
check "shell echo"                   "hello kernel"
OUT=$(run "date")
check "RTC date"                     "20[0-9][0-9]-[01][0-9]-[0-3][0-9] [0-2][0-9]:[0-5][0-9]:[0-5][0-9] UTC"

OUT=$(run "pagefault")
check "page fault caught with CR2"   "cr2=0xdeadb000"

OUT=$(run "divzero")
check "divide error caught"          "EXCEPTION 0: Divide by zero"

echo "$pass passed, $fail failed"
[ $fail -eq 0 ]
