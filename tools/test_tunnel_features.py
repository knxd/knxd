#!/usr/bin/env python3
"""
Test script for KNXnet/IP Tunnelling v2 features (TUNNEL_FEATURE_GET/SET).

Tests all interface features defined in KNX Standard v3.0.4 section 03.08.04
(Tunnelling v01.07.01) against a knxd instance with TCP tunnel support.

Usage:
    python3 test_tunnel_features.py [host] [port]
    Default: host=127.0.0.1, port=3671
"""

import socket
import struct
import sys

# KNXnet/IP constants
HEADER_SIZE = 0x06
KNXNETIP_VERSION = 0x10

# Service types
DESCRIPTION_REQUEST = 0x0203
DESCRIPTION_RESPONSE = 0x0204
CONNECTION_REQUEST = 0x0205
CONNECTION_RESPONSE = 0x0206
DISCONNECT_REQUEST = 0x0209
DISCONNECT_RESPONSE = 0x020A
SEARCH_REQUEST_EXTENDED = 0x020B
SEARCH_RESPONSE_EXTENDED = 0x020C
TUNNEL_FEATURE_GET = 0x0422
TUNNEL_FEATURE_RESPONSE = 0x0423
TUNNEL_FEATURE_SET = 0x0424

# Connection types
TUNNEL_CONNECTION = 0x04
TUNNEL_LINKLAYER = 0x02

# Feature IDs
FEATURES = {
    0x01: "SupportedEMIType",
    0x02: "HostDeviceDescriptorType0",
    0x03: "BusConnectionStatus",
    0x04: "KNXManufacturerCode",
    0x05: "ActiveEMIType",
    0x06: "InterfaceIndividualAddress",
    0x07: "MaxAPDULength",
    0x08: "InterfaceFeatureInfoEnable",
}

# Feature return codes
RETURN_CODES = {
    0x00: "E_NO_ERROR",
    0x01: "E_ACCESS_READ_ONLY",
    0x02: "E_ADDRESS_VOID",
    0x03: "E_DATA_TYPE_CONFLICT",
    0x04: "E_DATA_VOID",
}

# Host Protocol Address Information (HPAI) — route back (all zeros for TCP)
HPAI_ROUTE_BACK = struct.pack("!BB HI", 0x08, 0x02, 0, 0)  # len=8, TCP, port=0, ip=0


def make_header(service_type, total_length):
    return struct.pack("!BBHH", HEADER_SIZE, KNXNETIP_VERSION, service_type, total_length)


def make_connection_request():
    """CONNECTION_REQUEST for a tunnel link-layer connection."""
    # CRI: connection type (TUNNEL_CONNECTION), tunnel layer (LINK), reserved
    cri = struct.pack("!BBB", TUNNEL_CONNECTION, TUNNEL_LINKLAYER, 0x00)
    # CRI length header
    cri_with_len = struct.pack("!B", len(cri) + 1) + cri
    body = HPAI_ROUTE_BACK + HPAI_ROUTE_BACK + cri_with_len
    header = make_header(CONNECTION_REQUEST, HEADER_SIZE + len(body))
    return header + body


def make_disconnect_request(channel_id):
    body = struct.pack("!BB", channel_id, 0x00) + HPAI_ROUTE_BACK
    header = make_header(DISCONNECT_REQUEST, HEADER_SIZE + len(body))
    return header + body


def make_feature_get(channel_id, seqno, feature_id):
    """TUNNEL_FEATURE_GET packet."""
    # Connection header: length(4), channelID, seqno, reserved
    # Feature: featureID, reserved
    conn_hdr = struct.pack("!BBBB", 4, channel_id, seqno, 0x00)
    feature = struct.pack("!BB", feature_id, 0x00)
    body = conn_hdr + feature
    header = make_header(TUNNEL_FEATURE_GET, HEADER_SIZE + len(body))
    return header + body


def make_feature_set(channel_id, seqno, feature_id, value_bytes):
    """TUNNEL_FEATURE_SET packet."""
    conn_hdr = struct.pack("!BBBB", 4, channel_id, seqno, 0x00)
    feature = struct.pack("!BB", feature_id, 0x00) + value_bytes
    body = conn_hdr + feature
    header = make_header(TUNNEL_FEATURE_SET, HEADER_SIZE + len(body))
    return header + body


def make_description_request():
    """DESCRIPTION_REQUEST packet."""
    body = HPAI_ROUTE_BACK
    header = make_header(DESCRIPTION_REQUEST, HEADER_SIZE + len(body))
    return header + body


def make_search_request_extended():
    """SEARCH_REQUEST_EXTENDED packet (no SRPs — request all DIBs)."""
    body = HPAI_ROUTE_BACK
    header = make_header(SEARCH_REQUEST_EXTENDED, HEADER_SIZE + len(body))
    return header + body


def recv_packet(sock, timeout=5.0):
    """Receive one KNXnet/IP packet from TCP socket."""
    sock.settimeout(timeout)
    # Read header
    hdr = b""
    while len(hdr) < HEADER_SIZE:
        chunk = sock.recv(HEADER_SIZE - len(hdr))
        if not chunk:
            raise ConnectionError("Connection closed")
        hdr += chunk

    hdr_len, version, service, total_len = struct.unpack("!BBHH", hdr)
    if hdr_len != HEADER_SIZE or version != KNXNETIP_VERSION:
        raise ValueError(f"Bad header: len={hdr_len} ver=0x{version:02x}")

    remaining = total_len - HEADER_SIZE
    body = b""
    while len(body) < remaining:
        chunk = sock.recv(remaining - len(body))
        if not chunk:
            raise ConnectionError("Connection closed while reading body")
        body += chunk

    return service, body


def format_hex(data):
    return " ".join(f"{b:02x}" for b in data)


def format_knx_addr(hi, lo):
    addr = (hi << 8) | lo
    return f"{addr >> 12}.{(addr >> 8) & 0xf}.{addr & 0xff}"


def run_tests(host, port):
    print(f"Connecting to {host}:{port} ...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(10)
    sock.connect((host, port))
    print("Connected.\n")

    passed = 0
    failed = 0

    # Service family IDs (03_08_02 Core v01.06.02, §7.5.4.3 Table 3)
    SF_CORE = 0x02
    SF_DEVICE_MANAGEMENT = 0x03
    SF_TUNNELLING = 0x04

    # Step 0a: DESCRIPTION_REQUEST
    print("=== DESCRIPTION_REQUEST ===")
    sock.sendall(make_description_request())
    service, body = recv_packet(sock)
    if service == DESCRIPTION_RESPONSE:
        # Device info DIB starts at offset 0, length 54, type 0x01
        # Service families DIB follows at offset 54
        if len(body) >= 56:
            svc_dib_len = body[54]
            svc_dib_type = body[55]
            families = {}
            if svc_dib_type == 0x02:  # SUPPORTED_SVC_FAMILIES
                for i in range(56, 54 + svc_dib_len, 2):
                    if i + 1 < len(body):
                        families[body[i]] = body[i + 1]
            v2_ok = (families.get(SF_CORE, 0) >= 2 and
                     families.get(SF_DEVICE_MANAGEMENT, 0) >= 2 and
                     families.get(SF_TUNNELLING, 0) >= 2)
            family_str = ", ".join(f"0x{k:02x}=v{v}" for k, v in sorted(families.items()))
            if v2_ok:
                print(f"  OK: families=[{family_str}] (all v2)")
                passed += 1
            else:
                print(f"  UNEXPECTED: families=[{family_str}] (expected v2)")
                failed += 1
        else:
            print(f"  UNEXPECTED: response too short ({len(body)} bytes)")
            failed += 1
    else:
        print(f"  ERROR: Expected DESCRIPTION_RESPONSE (0x0204), got 0x{service:04x}")
        failed += 1
    print()

    # Step 0b: SEARCH_REQUEST_EXTENDED
    print("=== SEARCH_REQUEST_EXTENDED ===")
    sock.sendall(make_search_request_extended())
    service, body = recv_packet(sock)
    if service == SEARCH_RESPONSE_EXTENDED:
        # HPAI (8 bytes) + Device info DIB (54) + Service families DIB
        if len(body) >= 64:
            svc_dib_offset = 8 + 54  # HPAI + device DIB
            svc_dib_len = body[svc_dib_offset]
            svc_dib_type = body[svc_dib_offset + 1]
            families = {}
            if svc_dib_type == 0x02:
                for i in range(svc_dib_offset + 2, svc_dib_offset + svc_dib_len, 2):
                    if i + 1 < len(body):
                        families[body[i]] = body[i + 1]
            v2_ok = (families.get(SF_CORE, 0) >= 2 and
                     families.get(SF_DEVICE_MANAGEMENT, 0) >= 2 and
                     families.get(SF_TUNNELLING, 0) >= 2)
            family_str = ", ".join(f"0x{k:02x}=v{v}" for k, v in sorted(families.items()))
            if v2_ok:
                print(f"  OK: families=[{family_str}] (all v2)")
                passed += 1
            else:
                print(f"  UNEXPECTED: families=[{family_str}] (expected v2)")
                failed += 1
        else:
            print(f"  UNEXPECTED: response too short ({len(body)} bytes)")
            failed += 1
    else:
        print(f"  ERROR: Expected SEARCH_RESPONSE_EXTENDED (0x020C), got 0x{service:04x}")
        failed += 1
    print()

    # Step 1: CONNECTION_REQUEST
    print("=== CONNECTION_REQUEST ===")
    sock.sendall(make_connection_request())
    service, body = recv_packet(sock)
    if service != CONNECTION_RESPONSE:
        print(f"  ERROR: Expected CONNECTION_RESPONSE (0x0206), got 0x{service:04x}")
        sock.close()
        return False

    channel_id = body[0]
    status = body[1]
    if status != 0:
        print(f"  ERROR: Connection rejected, status=0x{status:02x}")
        sock.close()
        return False

    # Parse CRD for individual address
    crd_offset = 2 + 8  # skip channel+status + HPAI (8 bytes)
    if len(body) > crd_offset + 3:
        crd_len = body[crd_offset]
        crd_type = body[crd_offset + 1]
        if crd_type == TUNNEL_CONNECTION and crd_len >= 4:
            addr_hi = body[crd_offset + 2]
            addr_lo = body[crd_offset + 3]
            print(f"  Channel ID: {channel_id}, Address: {format_knx_addr(addr_hi, addr_lo)}")
        else:
            print(f"  Channel ID: {channel_id}")
    else:
        print(f"  Channel ID: {channel_id}")
    print()

    seqno = 0

    # Step 2: TUNNEL_FEATURE_GET for all known features
    print("=== TUNNEL_FEATURE_GET ===")
    for fid in sorted(FEATURES.keys()):
        fname = FEATURES[fid]
        sock.sendall(make_feature_get(channel_id, seqno, fid))
        seqno = (seqno + 1) & 0xFF

        service, body = recv_packet(sock)
        if service != TUNNEL_FEATURE_RESPONSE:
            print(f"  [{fname}] ERROR: Expected FEATURE_RESPONSE, got 0x{service:04x}")
            failed += 1
            continue

        resp_fid = body[4]
        resp_rc = body[5]
        resp_value = body[6:]
        rc_name = RETURN_CODES.get(resp_rc, f"0x{resp_rc:02x}")

        if resp_fid != fid:
            print(f"  [{fname}] ERROR: Response featureID=0x{resp_fid:02x}, expected 0x{fid:02x}")
            failed += 1
            continue

        if resp_rc == 0x00:
            value_str = format_hex(resp_value)
            extra = ""
            if fid == 0x06 and len(resp_value) >= 2:
                extra = f" ({format_knx_addr(resp_value[0], resp_value[1])})"
            elif fid == 0x07 and len(resp_value) >= 2:
                extra = f" ({(resp_value[0] << 8) | resp_value[1]} bytes)"
            elif fid in (0x01, 0x05) and len(resp_value) >= 1:
                emi_names = {0x01: "EMI1", 0x02: "EMI2", 0x04: "cEMI"}
                emi_byte = resp_value[-1]  # last byte has the bitfield
                extra = f" ({emi_names.get(emi_byte, '?')})"
            elif fid == 0x04 and len(resp_value) >= 2:
                extra = f" (0x{(resp_value[0] << 8) | resp_value[1]:04x})"
            print(f"  [0x{fid:02x} {fname}] OK: value={value_str}{extra}")
            passed += 1
        else:
            print(f"  [0x{fid:02x} {fname}] rc={rc_name}, value={format_hex(resp_value)}")
            failed += 1

    print()

    # Step 3: TUNNEL_FEATURE_GET for unknown feature 0xFF
    print("=== TUNNEL_FEATURE_GET unknown feature 0xFF ===")
    sock.sendall(make_feature_get(channel_id, seqno, 0xFF))
    seqno = (seqno + 1) & 0xFF
    service, body = recv_packet(sock)
    if service == TUNNEL_FEATURE_RESPONSE:
        resp_rc = body[5]
        rc_name = RETURN_CODES.get(resp_rc, f"0x{resp_rc:02x}")
        resp_value = body[6:]
        if resp_rc == 0x02:  # FR_ADDRESS_VOID
            print(f"  OK: rc={rc_name}, no value (len={len(resp_value)})")
            passed += 1
        else:
            print(f"  UNEXPECTED: rc={rc_name}, value={format_hex(resp_value)}")
            failed += 1
    else:
        print(f"  ERROR: Expected FEATURE_RESPONSE, got 0x{service:04x}")
        failed += 1
    print()

    # Step 4: TUNNEL_FEATURE_SET for read-only feature 0x01
    print("=== TUNNEL_FEATURE_SET read-only feature 0x01 (SupportedEMIType) ===")
    sock.sendall(make_feature_set(channel_id, seqno, 0x01, b"\x04"))
    seqno = (seqno + 1) & 0xFF
    service, body = recv_packet(sock)
    if service == TUNNEL_FEATURE_RESPONSE:
        resp_rc = body[5]
        rc_name = RETURN_CODES.get(resp_rc, f"0x{resp_rc:02x}")
        resp_value = body[6:]
        if resp_rc == 0x01:  # FR_ACCESS_READ_ONLY
            print(f"  OK: rc={rc_name}, value={format_hex(resp_value)}")
            passed += 1
        else:
            print(f"  UNEXPECTED: rc={rc_name}, value={format_hex(resp_value)}")
            failed += 1
    else:
        print(f"  ERROR: Expected FEATURE_RESPONSE, got 0x{service:04x}")
        failed += 1
    print()

    # Step 5: TUNNEL_FEATURE_SET for writable feature 0x08
    print("=== TUNNEL_FEATURE_SET writable feature 0x08 (FeatureInfoEnable) ===")
    sock.sendall(make_feature_set(channel_id, seqno, 0x08, b"\x01"))
    seqno = (seqno + 1) & 0xFF
    service, body = recv_packet(sock)
    if service == TUNNEL_FEATURE_RESPONSE:
        resp_rc = body[5]
        rc_name = RETURN_CODES.get(resp_rc, f"0x{resp_rc:02x}")
        resp_value = body[6:]
        if resp_rc == 0x00 and len(resp_value) >= 1 and resp_value[0] == 0x01:
            print(f"  OK: rc={rc_name}, value={format_hex(resp_value)} (enabled)")
            passed += 1
        else:
            print(f"  UNEXPECTED: rc={rc_name}, value={format_hex(resp_value)}")
            failed += 1
    else:
        print(f"  ERROR: Expected FEATURE_RESPONSE, got 0x{service:04x}")
        failed += 1
    print()

    # Step 5b: Verify the value was stored by reading it back
    print("=== TUNNEL_FEATURE_GET 0x08 (verify write) ===")
    sock.sendall(make_feature_get(channel_id, seqno, 0x08))
    seqno = (seqno + 1) & 0xFF
    service, body = recv_packet(sock)
    if service == TUNNEL_FEATURE_RESPONSE:
        resp_rc = body[5]
        resp_value = body[6:]
        rc_name = RETURN_CODES.get(resp_rc, f"0x{resp_rc:02x}")
        if resp_rc == 0x00 and len(resp_value) >= 1 and resp_value[0] == 0x01:
            print(f"  OK: rc={rc_name}, value=0x{resp_value[0]:02x} (enabled, matches SET)")
            passed += 1
        else:
            print(f"  UNEXPECTED: rc={rc_name}, value={format_hex(resp_value)}")
            failed += 1
    else:
        print(f"  ERROR: Expected FEATURE_RESPONSE, got 0x{service:04x}")
        failed += 1
    print()

    # Step 6: TUNNEL_FEATURE_SET for unknown feature 0xFF
    print("=== TUNNEL_FEATURE_SET unknown feature 0xFF ===")
    sock.sendall(make_feature_set(channel_id, seqno, 0xFF, b"\x00"))
    seqno = (seqno + 1) & 0xFF
    service, body = recv_packet(sock)
    if service == TUNNEL_FEATURE_RESPONSE:
        resp_rc = body[5]
        rc_name = RETURN_CODES.get(resp_rc, f"0x{resp_rc:02x}")
        resp_value = body[6:]
        if resp_rc == 0x02:  # FR_ADDRESS_VOID
            print(f"  OK: rc={rc_name}, no value (len={len(resp_value)})")
            passed += 1
        else:
            print(f"  UNEXPECTED: rc={rc_name}, value={format_hex(resp_value)}")
            failed += 1
    else:
        print(f"  ERROR: Expected FEATURE_RESPONSE, got 0x{service:04x}")
        failed += 1
    print()

    # Step 7: DISCONNECT_REQUEST
    print("=== DISCONNECT_REQUEST ===")
    sock.sendall(make_disconnect_request(channel_id))
    service, body = recv_packet(sock)
    if service == DISCONNECT_RESPONSE:
        print(f"  OK: Disconnected (status={body[1]})")
    else:
        print(f"  Got 0x{service:04x}")
    print()

    sock.close()

    # Summary
    total = passed + failed
    print(f"Results: {passed}/{total} passed, {failed} failed")
    return failed == 0


if __name__ == "__main__":
    host = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 3671
    success = run_tests(host, port)
    sys.exit(0 if success else 1)
