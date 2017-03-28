#!/bin/sh

# This runs a couple of cheap tests on knxd.

export LD_LIBRARY_PATH=src/client/c/.libs${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH

set -ex
export PATH="$(pwd)/src/tools/.libs:$(pwd)/src/tools:$(pwd)/src/server/.libs:$(pwd)/src/server:$(pwd)/src/server:$PATH"

EF=$(tempfile)

# first test argument handling
if knxd -e 1.2.3 --stop-right-now >$EF 2>&1; then
	echo "Bad argument A" >&2
	cat $EF 2>&1
	exit 1
fi

if knxd -e 1.2.3 --stop-right-now -b dummy: >$EF 2>&1; then
	echo "Bad argument B" >&2
	cat $EF 2>&1
	exit 1
fi

if ! knxd -B log -e 1.2.3 --stop-right-now -b dummy: -b dummy: >$EF 2>&1; then
	echo "Bad argument C" >&2
	cat $EF 2>&1
	exit 1
fi

if knxd -e 1.2.3 --stop-right-now -b dummy: -b dummy: --tpuarts-disch-reset >$EF 2>&1; then
	echo "Bad argument D" >&2
	cat $EF 2>&1
	exit 1
fi

if knxd -e 1.2.3 --stop-right-now -b dummy: --tpuarts-disch-reset -b dummy: >$EF 2>&1; then
	echo "Bad argument E" >&2
	cat $EF 2>&1
	exit 1
fi

if knxd -e 1.2.3 --stop-right-now -T -b dummy: -b dummy: >$EF 2>&1; then
	echo "Bad argument F" >&2
	cat $EF 2>&1
	exit 1
fi

S1=$(tempfile); rm $S1
S2=$(tempfile); rm $S2
S3=$(tempfile); rm $S3
L1=$(tempfile)
L2=$(tempfile)
L3=$(tempfile)
L4=$(tempfile)
L5=$(tempfile)
E1=$(tempfile)
E2=$(tempfile)
E3=$(tempfile)
E4=$(tempfile)
E5=$(tempfile)

PORT=$((9999 + $$))
PORT2=$((9998 + $$))

knxd -n K1 -B log -t 0xfffc -f 9 -e 4.1.0 -E 4.1.1:5 -B log -c -B log -u$S1 --multi-port -D -B log -A delay=200 -B pace -R -B log -T --Server=224.99.98.97:$PORT -bdummy: &
KNX1=$!
trap 'echo T1; rm -f $L1 $L2 $E1 $E2 $EF; kill $KNX1; wait' 0 1 2

sleep 2
knxd -n K2 -B log -t 0xfffc -f 9 -e 4.2.0 -E 4.2.1:5 -D -B log -T -B log -R --Server=:$PORT2 -B log -u$S2 -B log -b ip:224.99.98.97:$PORT &
KNX2=$!
sleep 2
knxd -n K3 -B log -t 0xfffc -f 9 -e 4.3.0 -E 4.3.1:5 -B log -u$S3 -B log -b ipt:localhost:$PORT2:$((10001 + $$)) &
KNX3=$!
#read RETURN
trap 'echo T2; rm -f $L1 $L2 $E1 $E2 $EF; kill $KNX1 $KNX2 $KNX3; wait' 0 1 2
sleep 1

knxtool vbusmonitor1 local:$S1 >$L1 2>$E1 &
PL1=$!
knxtool vbusmonitor1 local:$S2 >$L2 2>$E2 &
PL2=$!
knxtool vbusmonitor1 local:$S3 >$L3 2>$E3 &
PL3=$!
knxtool grouplisten local:$S2 1/2/3 >$L5 2>$E5 &
PL5=$!
sleep 1
knxtool groupcachereadsync local:$S1 1/2/3 3 >$L4 2>$E4 &
PL4=$!
# will die by itself when the server terminates

# test that addresses get recycled
sleep 2
echo xmit 1
if ! knxtool groupswrite local:$S1 1/2/3 4 ; then echo X1; exit 1; fi
sleep 1
echo xmit 2
if ! knxtool groupswrite local:$S2 1/2/3 5 ; then echo X2; exit 1; fi
sleep 1
echo xmit 3
if ! knxtool groupswrite local:$S3 1/2/3 6 ; then echo X3; exit 1; fi
sleep 1
echo xmit 1
if ! knxtool groupswrite local:$S1 1/2/3 7 ; then echo X4; exit 1; fi
sleep 1
echo xmit 3
if ! knxtool groupswrite local:$S3 1/2/3 8 ; then echo X5; exit 1; fi
sleep 1
echo xmit 2
if ! knxtool groupwrite local:$S2 1/2/3 4 5 6 ; then echo X6; exit 1; fi
sleep 1
kill $PL4 || true
if ! knxtool groupcacheread local:$S1 1/2/3 >>$L4 2>>$E4 ; then echo X7; exit 1;
fi
if ! knxtool groupcachelastupdates local:$S1 3 1 >>$L4 2>>$E4 ; then echo X7; exit 1; fi

#read RETURN
kill $KNX1 $KNX2 $KNX3
sleep 1
kill $PL1 $PL2 $PL3 $PL5 || true
trap 'echo T3; rm -f $L1 $L2 $L3 $L4 $L5 $E1 $E2 $E3 $E4 $E5 $EF' 0 1 2
sleep 1
#ls -l $L1 $L2 $E1 $E2
#cat $L1 $L2 $E1 $E2
sed -e 's/^/E vbusmonitor 1: /' <$E1
sed -e 's/^/E vbusmonitor 2: /' <$E2
sed -e 's/^/E vbusmonitor 3: /' <$E3
sed -e 's/^/E groupcacheread: /' <$E4
sed -e 's/^/E grouplisten: /' <$E5

E=""
diff -u "$(dirname "$0")"/logs/monitor1 $L1 || E=1$E
diff -u "$(dirname "$0")"/logs/monitor2 $L2 || E=2$E
diff -u "$(dirname "$0")"/logs/monitor3 $L3 || E=3$E
diff -u "$(dirname "$0")"/logs/cache $L4 || E=4$E
diff -u "$(dirname "$0")"/logs/listen $L5 || E=5$E
test -z "$E"

set +ex

rm -f $L1 $L2 $L3 $L4 $L5 $E1 $E2 $E3 $E4 $E5 $EF
trap '' 0 1 2 
echo DONE OK
