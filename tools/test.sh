#!/bin/sh

# This runs a couple of cheap tests on knxd.

set -ex
export PATH="$(pwd)/src/examples:$(pwd)/src/server:$PATH"

EF=$(tempfile)

# first test argument handling
if knxd --stop-right-now >$EF 2>&1; then
	echo "Bad argument A" >&2
	cat $EF 2>&1
	exit 1
fi

if knxd --stop-right-now -b dummy: >$EF 2>&1; then
	echo "Bad argument B" >&2
	cat $EF 2>&1
	exit 1
fi

if ! knxd --stop-right-now -b dummy: -b dummy: >$EF 2>&1; then
	echo "Bad argument C" >&2
	cat $EF 2>&1
	exit 1
fi

if knxd --stop-right-now -b dummy: -b dummy: --tpuarts-disch-reset >$EF 2>&1; then
	echo "Bad argument D" >&2
	cat $EF 2>&1
	exit 1
fi

if knxd --stop-right-now -b dummy: --tpuarts-disch-reset -b dummy: >$EF 2>&1; then
	echo "Bad argument E" >&2
	cat $EF 2>&1
	exit 1
fi

if knxd --stop-right-now -T -b dummy: -b dummy: >$EF 2>&1; then
	echo "Bad argument F" >&2
	cat $EF 2>&1
	exit 1
fi

S1=$(tempfile); rm $S1
S2=$(tempfile); rm $S2
S3=$(tempfile); rm $S3
L1=$(tempfile)

knxd -t 0xfffc -f 9 -e 3.2.1 -u$S1 -u$S2 -u$S3 -bdummy: &
KNXD=$!
trap 'rm -f $L1 $EF; kill $KNXD' 0 1 2

grouplisten local:$S2 1/2/3 >$L1 &
# will die by itself when the server terminates
PL1=$!

sleep 1
groupswrite local:$S1 1/2/3 4
sleep 1
groupwrite local:$S2 1/2/3 4 5

sleep 1
kill $KNXD $PL1
trap 'rm -f $L1 $EF' 0 1 2

diff -u "$(dirname "$0")"/logs/listen $L1

