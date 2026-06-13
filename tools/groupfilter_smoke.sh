#!/bin/sh

# Lightweight smoke test for the group filter (#585).
# It verifies:
# 1) filter registration in knxd -l filter
# 2) CLI path does not fail with "filter 'group' not found"
# 3) INI path does not fail with "filter 'group' not found"
# 4) behavior path blocks configured group address while allowing others

set -eu

log()
{
  echo "[groupfilter-smoke] $*"
}

ok()
{
  echo "[groupfilter-smoke][OK] $*"
}

fail()
{
  echo "[groupfilter-smoke][ERROR] $*" >&2
}

ROOT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
KNXD="$ROOT_DIR/src/server/knxd"
KNXTOOL="$ROOT_DIR/src/tools/knxtool"
BEHAVIOR_PID=""

if [ ! -x "$KNXD" ]; then
  echo "ERROR: $KNXD not found or not executable. Build first (make -j4)." >&2
  exit 1
fi

if [ ! -x "$KNXTOOL" ]; then
  echo "ERROR: $KNXTOOL not found or not executable. Build first (make -j4)." >&2
  exit 1
fi

TMP_DIR="$(mktemp -d)"
KEEP_TMP="${KEEP_TMP:-0}"
cleanup()
{
  if [ -n "$BEHAVIOR_PID" ]; then
    kill "$BEHAVIOR_PID" 2>/dev/null || true
    wait "$BEHAVIOR_PID" 2>/dev/null || true
  fi

  if [ "$KEEP_TMP" = "1" ]; then
    log "KEEP_TMP=1, keeping temporary files at: $TMP_DIR"
  else
    rm -rf "$TMP_DIR"
  fi
}
trap cleanup EXIT INT TERM

FILTERS_LOG="$TMP_DIR/filters.log"
CLI_LOG="$TMP_DIR/cli.log"
INI_LOG="$TMP_DIR/ini.log"
INI_FILE="$TMP_DIR/groupfilter.ini"
BEHAVIOR_LOG="$TMP_DIR/behavior.log"
BEHAVIOR_INI="$TMP_DIR/behavior.ini"
BEHAVIOR_SOCK="$TMP_DIR/knxd-gf.sock"

"$KNXD" -l filter >"$FILTERS_LOG" 2>&1
log "Step 1/4: checking filter registration via 'knxd -l filter'"
if ! grep -q '^group$' "$FILTERS_LOG"; then
  fail "group filter is not registered"
  cat "$FILTERS_LOG" >&2
  exit 1
fi
ok "group filter is registered"

# Run for a short, bounded period. timeout exit 124 is expected for smoke tests.
log "Step 2/4: checking CLI path (timeout 4s)"
set +e
timeout 4 "$KNXD" \
  -e 1.1.128 -E 1.1.129:8 \
  -D -S -i 6720 \
  -A mode=block -A addresses=0/0/1,1/2/3 -B group \
  -A multicast-ttl=13 -b ip: \
  >"$CLI_LOG" 2>&1
CLI_RC=$?
set -e

if [ "$CLI_RC" -ne 0 ] && [ "$CLI_RC" -ne 124 ]; then
  fail "CLI run failed with unexpected exit code: $CLI_RC"
  cat "$CLI_LOG" >&2
  exit 1
fi

if grep -q "filter 'group' not found" "$CLI_LOG"; then
  fail "CLI path failed to create group filter"
  cat "$CLI_LOG" >&2
  exit 1
fi
ok "CLI path passed (exit=$CLI_RC, no 'filter group not found')"

cat >"$INI_FILE" <<'EOF'
[main]
addr = 1.1.128
client-addrs = 1.1.129:8
connections = router,server

[server]
server = knxd_tcp
port = 6720

[router]
driver = ip
multicast-ttl = 13
filters = mygroupfilter

[mygroupfilter]
filter = group
mode = block
addresses = 0/0/1,1/2/3,2/1/0
EOF

log "Step 3/4: checking INI path (timeout 4s)"
set +e
timeout 4 "$KNXD" "$INI_FILE" >"$INI_LOG" 2>&1
INI_RC=$?
set -e

if [ "$INI_RC" -ne 0 ] && [ "$INI_RC" -ne 124 ]; then
  fail "INI run failed with unexpected exit code: $INI_RC"
  cat "$INI_LOG" >&2
  exit 1
fi

if grep -q "filter 'group' not found" "$INI_LOG"; then
  fail "INI path failed to create group filter"
  cat "$INI_LOG" >&2
  exit 1
fi
ok "INI path passed (exit=$INI_RC, no 'filter group not found')"

cat >"$BEHAVIOR_INI" <<EOF
[main]
addr = 1.1.128
client-addrs = 1.1.129:8
connections = unix,dummy

[unix]
server = knxd_unix
path = $BEHAVIOR_SOCK

[dummy]
driver = dummy
filters = mygroupfilter,mylog

[mygroupfilter]
filter = group
mode = block
addresses = 1/2/3
debug = debug-gf

[mylog]
filter = log
debug = debug-log

[debug-gf]
trace-mask = 0xffff
error-level = 9

[debug-log]
trace-mask = 0xffff
error-level = 9
EOF

log "Step 4/4: checking behavior path (blocked 1/2/3, passed 1/2/4)"
"$KNXD" "$BEHAVIOR_INI" >"$BEHAVIOR_LOG" 2>&1 &
BEHAVIOR_PID=$!

# Wait briefly for the unix socket to appear.
i=0
while [ ! -S "$BEHAVIOR_SOCK" ] && [ "$i" -lt 30 ]; do
  i=$((i + 1))
  sleep 0.1
done

if [ ! -S "$BEHAVIOR_SOCK" ]; then
  fail "behavior test socket was not created"
  cat "$BEHAVIOR_LOG" >&2
  exit 1
fi

"$KNXTOOL" groupswrite local:"$BEHAVIOR_SOCK" 1/2/3 01 >/dev/null 2>&1
"$KNXTOOL" groupswrite local:"$BEHAVIOR_SOCK" 1/2/4 01 >/dev/null 2>&1
sleep 0.5

if ! grep -q "groupfilter: blocked send to 1/2/3" "$BEHAVIOR_LOG"; then
  fail "blocked telegram evidence not found for 1/2/3"
  cat "$BEHAVIOR_LOG" >&2
  exit 1
fi

if ! grep -Eq "Send L_Data low .* to 1/2/4 " "$BEHAVIOR_LOG"; then
  fail "pass-through telegram evidence not found for 1/2/4"
  cat "$BEHAVIOR_LOG" >&2
  exit 1
fi

if grep -Eq "Send L_Data low .* to 1/2/3 " "$BEHAVIOR_LOG"; then
  fail "blocked address 1/2/3 unexpectedly passed beyond group filter"
  cat "$BEHAVIOR_LOG" >&2
  exit 1
fi

ok "behavior path passed (1/2/3 blocked, 1/2/4 passed)"

log "All checks passed"
echo "groupfilter smoke: OK"
