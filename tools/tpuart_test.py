#!/usr/bin/env python3
"""
Test knxd's tpuart serial driver against an emulated TPUART chip.

Creates a PTY pair, starts knxd with -b tpuarts:<pty> and emulates the
TPUART side of the protocol:

  1. answers the initial reset with 0x03 (RESET_ACK)
  2. answers GetState (0x02) with a state indication (0x07)
  3. sends L_Data standard frames; checks that knxd
     - ACKs addressed frames (0x11)
     - forwards the telegrams to a group socket client
  4. sends a group write from the client side and checks that knxd
     transmits it as U-data (0x80|n ... 0x40|n) and accepts the 0x8B
  5. checks byte-at-a-time and burst delivery behave identically

Exit status: 0 = OK, 1 = test failed, 2 = setup problem.
"""

import os
import pty
import select
import signal
import socket
import subprocess
import sys
import tempfile
import threading
import time

KNXD = os.environ.get("KNXD", "./src/server/knxd")

# ---------------------------------------------------------------- helpers

def checksum(frame):
    c = 0
    for b in frame:
        c ^= b
    return (~c) & 0xFF


def ldata_std(src, dst, payload, group=True, hop=6):
    """L_Data standard frame (9+ bytes), big endian"""
    ctrl = 0x90  # not repeated, priority low, standard frame
    npdu = ((0x80 if group else 0x00) | (hop << 4) | (len(payload) - 1)) & 0xFF
    f = [ctrl, src >> 8, src & 0xFF, dst >> 8, dst & 0xFF, npdu] + list(payload)
    f.append(checksum(f))
    return bytes(f)


# 1/2/3 = 0x0A03 group address
GA = 0x0A03
FRAMES = [
    ldata_std(0x1101, GA, [0x00, 0x01]),
    ldata_std(0x1102, GA, [0x00, 0x02]),
    ldata_std(0x1103, GA, [0x00, 0x03]),
    ldata_std(0x1104, GA, [0x00, 0x04]),
    ldata_std(0x1105, GA, [0x00, 0x05]),
]

# ------------------------------------------------------------------ setup

master, slave = pty.openpty()
os.set_blocking(master, False)

SOCK = tempfile.mktemp(prefix="knxtp-")
LOGF = tempfile.NamedTemporaryFile(prefix="knxtp-log", delete=False)
LOG = LOGF.name

proc = [None]
fails = []
udata_frames = []
lock = threading.Lock()
events = []               # (name, data) timeline


def P(*a):
    msg = " ".join(str(x) for x in a)
    print(msg, flush=True)
    LOGF.write((msg + "\n").encode())
    LOGF.flush()


# ------------------------------------------------------- emulator thread

udata_buf = bytearray()   # partial U-data frame from the emulator's perspective


def emulator():
    """TPUART chip emulation: reads knxd's output, answers protocol bytes"""
    global udata_buf
    while True:
        try:
            r, _, _ = select.select([master], [], [], 0.2)
        except OSError:
            return
        if proc[0] and proc[0].poll() is not None:
            return
        if not r:
            continue
        try:
            data = os.read(master, 4096)
        except OSError:
            continue
        if not data:
            continue
        with lock:
            events.append(("emu<-knxd", data))
        i = 0
        while i < len(data):
            b = data[i]
            if b == 0x01:            # Reset
                os.write(master, b"\x03")
                i += 1
            elif b == 0x02:          # GetState
                os.write(master, b"\x07")
                i += 1
            elif b >= 0x40:          # U-data prefix byte (0x40|n or 0x80|n)
                if i + 1 < len(data):
                    udata_buf.append(data[i + 1])
                    if b & 0x40:     # terminating byte: frame complete
                        with lock:
                            udata_frames.append(bytes(udata_buf))
                        # real chip: confirm after successful bus transmission
                        os.write(master, b"\x8B")
                        udata_buf.clear()
                    i += 2
                else:
                    i += 1           # prefix at end of chunk; wait for data byte
            else:
                # stray control byte (0x8B etc.) between frames: skip
                i += 1


emu = threading.Thread(target=emulator, daemon=True)
emu.start()

# ---------------------------------------------------------------- start

proc[0] = subprocess.Popen(
    [
        KNXD, "-e", "1.2.3", "-E", "1.2.200:5", "-n", "tpuart-test",
        "-B", "log", "-t", "0xfffc", "-f", "9",
        "--tpuarts-ack-all-group",
        "-b", "tpuarts:" + os.ttyname(slave),
        "-u", SOCK,
    ],
    stdout=LOGF, stderr=subprocess.STDOUT,
)
P("knxd pid", proc[0].pid, "socket", SOCK, "serial", os.ttyname(slave))

deadline = time.time() + 15
while not os.path.exists(SOCK):
    if proc[0].poll() is not None:
        P("FAIL: knxd died during startup")
        P(open(LOG).read())
        sys.exit(1)
    if time.time() > deadline:
        P("FAIL: knxd socket never appeared")
        sys.exit(2)
    time.sleep(0.1)

# wait for the link to come up (reset + getstate answered)
deadline = time.time() + 15
while time.time() < deadline:
    with lock:
        seen = [d for _, d in events if b"\x07" in d or b"\x03" in d]
    if seen:
        break
    if proc[0].poll() is not None:
        break
    time.sleep(0.1)
time.sleep(0.5)

# --------------------------------------------------- group socket client

gs = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
gs.connect(SOCK)
gs.settimeout(0.5)
# open group socket: [len=5][type=0x0026][00][00][write_only=0]
gs.sendall(b"\x00\x05\x00\x26\x00\x00\x00")
resp = gs.recv(64)
if resp != b"\x00\x02\x00\x26":
    P("FAIL: group socket open: got", resp.hex())
    fails.append(f"group socket open: {resp.hex()}")
else:
    P("group socket open OK")

# ------------------------------------------------------ bus -> knxd frames

# send frames: first three one byte at a time, last two as bursts
for idx, frame in enumerate(FRAMES):
    P(f"--- sending frame {idx}: {frame.hex()}")
    if idx < 3:
        for b in frame:
            os.write(master, bytes([b]))
            time.sleep(0.02)
    else:
        os.write(master, frame)
    time.sleep(0.5)

# collect group socket data
deadline = time.time() + 10
gs_data = bytearray()
while time.time() < deadline:
    try:
        d = gs.recv(4096)
        if not d:
            break
        gs_data += d
        deadline = time.time() + 2
    except socket.timeout:
        pass

P("group socket got", len(gs_data), "bytes:", gs_data.hex())

# parse: each message = [len(2)][type=0x0027][src(2)][dst(2)][data...]
telegrams = []
i = 0
while i + 2 <= len(gs_data):
    mlen = (gs_data[i] << 8) | gs_data[i + 1]
    body = gs_data[i + 2:i + 2 + mlen]
    if len(body) < mlen:
        P("FAIL: truncated message at", i, gs_data.hex())
        fails.append("truncated group socket message")
        break
    if len(body) >= 6 and body[0] == 0x00 and body[1] == 0x27:
        src = (body[2] << 8) | body[3]
        dst = (body[4] << 8) | body[5]
        data = body[6:]
        telegrams.append((src, dst, data))
    i += 2 + mlen

P("telegrams:", [(hex(s), hex(d), d2.hex()) for s, d, d2 in telegrams])

# check all 5 frames arrived with correct values
for idx, frame in enumerate(FRAMES):
    want_src = (frame[1] << 8) | frame[2]
    want_val = frame[7]
    found = [t for t in telegrams if t[0] == want_src and t[2] == bytes([0x00, want_val])]
    if not found:
        P(f"FAIL: frame {idx} (src {want_src:04x} value {want_val}) not received")
        fails.append(f"frame {idx} not received by group socket")
    else:
        P(f"frame {idx}: received OK (src {found[0][0]:04x})")

# ------------------------------------------------------ knxd -> bus frames

# send a group write from the client; knxd must transmit U-data on serial
gs.sendall(b"\x00\x06\x00\x27" + bytes([GA >> 8, GA & 0xFF]) + b"\x00\x42")
P("sent groupsocket write (value 0x42)")

# wait for the U-data frame to appear on the serial line
deadline = time.time() + 10
while time.time() < deadline:
    with lock:
        if udata_frames:
            break
    time.sleep(0.1)

with lock:
    frames_out = list(udata_frames)

if not frames_out:
    P("FAIL: knxd did not transmit our group write as U-data")
    fails.append("no U-data for client group write")
else:
    for f in frames_out:
        P("U-data frame:", f.hex())
    # the transmitted frame must be an L_Data standard frame to 1/2/3, value 0x42
    f = frames_out[0]
    if len(f) < 9 or f[0] & 0x53 != 0x10:
        P("FAIL: U-data is not an L_Data standard frame:", f.hex())
        fails.append(f"U-data frame bad: {f.hex()}")
    else:
        dst = (f[3] << 8) | f[4]
        if dst != GA:
            P(f"FAIL: U-data destination {dst:04x} != {GA:04x}")
            fails.append(f"U-data dst {dst:04x}")
        if f[6:] != b"\x00\x42" + bytes([checksum(f[:-1])]):
            P("FAIL: U-data payload wrong:", f.hex())
            fails.append(f"U-data payload: {f.hex()}")
        else:
            P("U-data frame content OK")

# ---------------------------------------------------------- check ACKs

# knxd must have ACKed our 5 bus frames with 0x11 each.
# The ACKs went to the master fd; we count 0x11 bytes seen by the emulator.
# (The emulator thread records raw bytes; re-scan them.)
ack_count = 0
with lock:
    for name, data in events:
        if name == "emu<-knxd":
            ack_count += data.count(b"\x11")
P("ACK (0x11) count:", ack_count, "expected 5")
if ack_count < 5:
    P("FAIL: expected at least 5 ACKs")
    fails.append(f"ACK count {ack_count} < 5")

# -------------------------------------------------------------- shutdown

gs.close()
try:
    proc[0].send_signal(signal.SIGTERM)
    proc[0].wait(timeout=60)
except subprocess.TimeoutExpired:
    P("FAIL: knxd did not exit")
    proc[0].kill()
    proc[0].wait()
    fails.append("knxd did not exit")

P("")
P("=" * 60)
if fails:
    P("TEST FAILED:")
    for f in fails:
        P(" -", f)
    P("full log in", LOG)
    sys.exit(1)
P("TEST PASSED")
P("log:", LOG)
LOGF.close()
