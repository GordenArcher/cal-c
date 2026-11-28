#!/bin/sh
# End-to-end check of the real binary. The unit tests cannot reach main.c, so
# this covers what it wires together: command dispatch, exit codes, and what
# lands on stdout versus stderr. Plain POSIX sh so it runs on Linux and macOS.
#
# Usage: sh tests/smoke/smoke.sh path/to/mycal

if [ $# -ne 1 ] || [ ! -x "$1" ]; then
    echo "usage: sh tests/smoke/smoke.sh path/to/mycal" >&2
    exit 2
fi

BIN=$1
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# A private home with no events, and no inherited settings.
HOME=$WORK
export HOME
unset CAL_EVENTS CAL_HOLIDAYS NO_COLOR

checks=0
failures=0

pass() { checks=$((checks + 1)); }

fail() {
    checks=$((checks + 1))
    failures=$((failures + 1))
    echo "  FAIL $1" >&2
    shift
    for line in "$@"; do echo "    $line" >&2; done
}

# check_status "what" expected_code actual_code
check_status() {
    if [ "$2" = "$3" ]; then pass; else fail "$1" "exit code: got $3, want $2"; fi
}

# check_has "what" text needle
check_has() {
    case "$2" in *"$3"*) pass ;; *) fail "$1" "missing: $3" "in: $2" ;; esac
}

check_lacks() {
    case "$2" in *"$3"*) fail "$1" "unexpected: $3" "in: $2" ;; *) pass ;; esac
}

check_eq() {
    if [ "$2" = "$3" ]; then pass; else fail "$1" "got:  $3" "want: $2"; fi
}

# Runs the binary, keeping stdout, stderr and the exit code apart.
run() {
    OUT=$("$BIN" "$@" 2>"$WORK/err")
    CODE=$?
    ERR=$(cat "$WORK/err")
}

# ---- adding, listing, viewing ----
run add 12-25 Christmas
check_status "add a yearly event" 0 "$CODE"
check_has "add confirms what it wrote" "$OUT" "12-25|Christmas"
[ -f "$HOME/.cal_events" ] && pass || fail "add creates ~/.cal_events"

run add 2026-10-15 Team offsite
check_status "add a one-off event" 0 "$CODE"

run list
check_status "list" 0 "$CODE"
check_eq "list shows both, numbered by file line" \
    "   1  every 12-25  Christmas
   2  2026-10-15   Team offsite" "$OUT"

run -n 10 2026
check_status "month view" 0 "$CODE"
check_has "month view lists the event" "$OUT" "15  Team offsite"
check_lacks "other months' events stay out" "$OUT" "Christmas"

run -n 12 2026
check_has "yearly event shows in its month" "$OUT" "25  Christmas"

run --no-events -n 12 2026
check_lacks "--no-events hides them" "$OUT" "Christmas"

# ---- holidays: shared dates come from their own file ----
cat > "$HOME/.cal_holidays" <<'EOF'
# shared calendar
04-05|Easter
easter+1|Easter Monday
4th-thu-11|Thanksgiving
last-mon-5|Memorial Day
EOF

run -n 4 2026
check_status "month view with holidays" 0 "$CODE"
check_has "easter rule resolves" "$OUT" "* 5  Easter"
check_has "easter offset follows" "$OUT" "* 6  Easter Monday"
check_lacks "a holiday stays in its month" "$OUT" "Thanksgiving"

run -n 11 2026
check_has "nth weekday rule resolves" "$OUT" "*26  Thanksgiving"

run --no-holidays -n 4 2026
check_lacks "--no-holidays hides them" "$OUT" "Easter"

cat > "$WORK/holidays.txt" <<'EOF'
05-01|Labour Day
EOF
run -H "$WORK/holidays.txt" -n 5 2026
check_has "-H reads the named file" "$OUT" "* 1  Labour Day"

run -H "$WORK/missing-holidays.txt" -n 4 2026
check_status "named holidays file that does not exist" 1 "$CODE"

run -H
check_status "-H without a value" 1 "$CODE"

# ---- removing ----
run remove 2
check_status "remove" 0 "$CODE"
check_has "remove says what it removed" "$OUT" "2026-10-15|Team offsite"

run list
check_eq "list after remove" "   1  every 12-25  Christmas" "$OUT"

run remove 5
check_status "remove a line that does not exist" 1 "$CODE"
check_has "and explains why" "$ERR" "is not an event"

run remove abc
check_status "remove with a bad number" 1 "$CODE"

# ---- a typo in the file is a warning, not a failure ----
echo "garbage" >> "$HOME/.cal_events"
run list
check_status "list survives a bad line" 0 "$CODE"
check_has "and warns with the line number" "$ERR" ":2:"
check_has "and still lists the good events" "$OUT" "Christmas"

# ---- explicit files and errors ----
run -e "$WORK/missing.txt" 10 2026
check_status "named events file that does not exist" 1 "$CODE"
run list -e "$WORK/missing.txt"
check_status "list on a missing named file" 1 "$CODE"

run add 2026-02-30 Nope
check_status "add an impossible date" 1 "$CODE"

run -x
check_status "unknown option" 1 "$CODE"
check_has "unknown option message" "$ERR" "Unknown option"

run foo
check_status "something that is neither month, year nor command" 1 "$CODE"

run --help
check_status "--help" 0 "$CODE"
check_has "help documents add" "$OUT" "add [-e FILE]"
check_has "help documents list" "$OUT" "list [-e FILE]"
check_has "help documents remove" "$OUT" "remove [-e FILE] LINE"
check_has "help documents holidays" "$OUT" "-H, --holidays FILE"

if [ "$failures" -eq 0 ]; then
    echo "ok    tests/smoke/smoke.sh ($checks checks)"
    exit 0
fi
echo "FAIL  tests/smoke/smoke.sh ($failures of $checks checks failed)"
exit 1
