/*
    knxd - KNX daemon
    KNX IP Secure - server-side TCP session handling

    Copyright (C) 2026

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef IPSECURE_H
#define IPSECURE_H

#include <cstdint>
#include <cstring>
#include <vector>
#include <map>
#include <string>

// Session status codes
#define STATUS_AUTH_SUCCESS     0x00
#define STATUS_AUTH_FAILED      0x01
#define STATUS_UNAUTHENTICATED  0x02
#define STATUS_TIMEOUT          0x03
#define STATUS_KEEPALIVE        0x04
#define STATUS_CLOSE            0x05

// Crypto constants
#define IPSEC_MAC_SIZE    16
#define IPSEC_KEY_SIZE    16
#define IPSEC_ECDH_SIZE   32

// Max concurrent unauthenticated + authenticated sessions
#define IPSEC_MAX_SESSIONS 16

/** Per-session state for KNX IP Secure unicast */
struct SecureSession {
  uint16_t session_id;
  uint8_t session_key[IPSEC_KEY_SIZE];

  // ECDH key exchange data (kept for authenticate step, cleared after)
  uint8_t xor_client_server[IPSEC_ECDH_SIZE]; // client_pub XOR server_pub

  // Sequence counters (recv_seq starts at max so first frame with seq=0 is accepted)
  uint64_t send_seq;
  uint64_t recv_seq;

  // Authenticated user
  uint8_t user_id;  // 0 = not yet authenticated

  enum State { IDLE, UNAUTHENTICATED, AUTHENTICATED } state;

  ~SecureSession() {
    memset(session_key, 0, IPSEC_KEY_SIZE);
    memset(xor_client_server, 0, IPSEC_ECDH_SIZE);
  }
};

/** KNX IP Secure crypto and session management */
class IPSecure {
public:
  IPSecure();
  ~IPSecure();

  // Configuration
  void setDeviceAuthPassword(const std::string& password);
  void setUserPassword(uint8_t userId, const std::string& password);
  void setSerialNumber(const uint8_t sno[6]);

  // Load passwords from .knxkeys keyring
  bool loadKeyring(const std::string& path, const std::string& password);

  // Session management
  SecureSession* findSession(uint16_t session_id);
  void removeSession(uint16_t session_id);

  // Handle SESSION_REQUEST: returns SESSION_RESPONSE bytes to send
  std::vector<uint8_t> handleSessionRequest(const uint8_t* data, size_t len);

  // Handle SESSION_AUTHENTICATE (already unwrapped from SecureWrapper)
  bool handleSessionAuthenticate(uint16_t session_id,
                                 const uint8_t* data, size_t len);

  // Unwrap a SECURE_WRAPPER frame
  std::vector<uint8_t> unwrapSecure(const uint8_t* data, size_t len,
                                     uint16_t& session_id_out);

  // Wrap a KNXnet/IP frame in SECURE_WRAPPER
  std::vector<uint8_t> wrapSecure(uint16_t session_id,
                                   const uint8_t* knxip_frame, size_t len);

  // Build a SESSION_STATUS frame wrapped in SECURE_WRAPPER
  std::vector<uint8_t> buildSessionStatus(uint16_t session_id, uint8_t status);

  bool isEnabled() const { return enabled; }

private:
  bool enabled;
  uint8_t serial_number[6];
  uint8_t device_auth_key[IPSEC_KEY_SIZE];

  // user_id -> password hash (16 bytes)
  std::map<uint8_t, std::vector<uint8_t>> user_pwd_hashes;

  // session_id -> session
  std::map<uint16_t, SecureSession> sessions;
  uint16_t next_session_id;

  uint16_t allocSessionId();

  static void deriveDeviceAuthKey(const std::string& password, uint8_t key[IPSEC_KEY_SIZE]);
  static void deriveUserPwdHash(const std::string& password, uint8_t key[IPSEC_KEY_SIZE]);

  static bool computeMAC16(const uint8_t key[IPSEC_KEY_SIZE],
                           const uint8_t b0[16],
                           const uint8_t* aad, size_t aad_len,
                           const uint8_t* payload, size_t payload_len,
                           uint8_t mac[IPSEC_MAC_SIZE]);

  static bool ctrEncrypt(const uint8_t key[IPSEC_KEY_SIZE],
                         const uint8_t ctr0[16],
                         uint8_t* data, size_t data_len);

  static void xorBytes(uint8_t* out, const uint8_t* a, const uint8_t* b, size_t len);
};

#endif
