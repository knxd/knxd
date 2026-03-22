/*
    knxd - KNX daemon
    KNX IP Secure - server-side TCP session handling

    Copyright (C) 2026

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "ipsecure.h"

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <cstring>
#include <fstream>

// X25519 key exchange
#include <openssl/x509.h>

// KNXnet/IP header helpers
#define KNXIP_HEADER_LEN 6
#define KNXIP_VERSION 0x10

static void putU16BE(uint8_t* p, uint16_t v) {
  p[0] = (v >> 8) & 0xFF;
  p[1] = v & 0xFF;
}

static uint16_t getU16BE(const uint8_t* p) {
  return ((uint16_t)p[0] << 8) | p[1];
}

static uint64_t getU48BE(const uint8_t* p) {
  uint64_t v = 0;
  for (int i = 0; i < 6; i++)
    v = (v << 8) | p[i];
  return v;
}

static void putU48BE(uint8_t* p, uint64_t v) {
  for (int i = 5; i >= 0; i--) {
    p[i] = v & 0xFF;
    v >>= 8;
  }
}

static void buildKNXIPHeader(uint8_t* buf, uint16_t service, uint16_t total_len) {
  buf[0] = KNXIP_HEADER_LEN;
  buf[1] = KNXIP_VERSION;
  putU16BE(buf + 2, service);
  putU16BE(buf + 4, total_len);
}

// AES-128-CBC encrypt (no padding) — for CBC-MAC
static bool aes_cbc_encrypt(const uint8_t key[16], const uint8_t iv[16],
                            const uint8_t* pt, int pt_len,
                            uint8_t* ct, int* ct_len) {
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return false;
  EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);
  EVP_CIPHER_CTX_set_padding(ctx, 0);
  int len = 0;
  EVP_EncryptUpdate(ctx, ct, &len, pt, pt_len);
  *ct_len = len;
  int fl = 0;
  EVP_EncryptFinal_ex(ctx, ct + len, &fl);
  *ct_len += fl;
  EVP_CIPHER_CTX_free(ctx);
  return true;
}

// AES-128-CTR — continuous stream encrypt/decrypt
static bool aes_ctr_stream(const uint8_t key[16], const uint8_t ctr0[16],
                           uint8_t* data, size_t len) {
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return false;
  EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, ctr0);
  int outl = 0;
  EVP_EncryptUpdate(ctx, data, &outl, data, len);
  uint8_t dummy;
  EVP_EncryptFinal_ex(ctx, &dummy, &outl);
  EVP_CIPHER_CTX_free(ctx);
  return true;
}

// AES-128-CBC decrypt (no padding) — for keyring
static bool aes_cbc_decrypt(const uint8_t key[16], const uint8_t iv[16],
                            const uint8_t* ct, int ct_len,
                            uint8_t* pt, int* pt_len) {
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return false;
  EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv);
  EVP_CIPHER_CTX_set_padding(ctx, 0);
  int len = 0;
  EVP_DecryptUpdate(ctx, pt, &len, ct, ct_len);
  *pt_len = len;
  int fl = 0;
  EVP_DecryptFinal_ex(ctx, pt + len, &fl);
  *pt_len += fl;
  EVP_CIPHER_CTX_free(ctx);
  return true;
}

// base64 decode
static std::vector<uint8_t> b64decode(const std::string& encoded) {
  std::vector<uint8_t> out(encoded.size());
  EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
  EVP_DecodeInit(ctx);
  int outl = 0, outl2 = 0;
  EVP_DecodeUpdate(ctx, out.data(), &outl,
                   (const unsigned char*)encoded.c_str(), encoded.size());
  EVP_DecodeFinal(ctx, out.data() + outl, &outl2);
  EVP_ENCODE_CTX_free(ctx);
  out.resize(outl + outl2);
  return out;
}

// Simple XML attribute extractor
static std::string xmlAttr(const std::string& tag, const std::string& attr) {
  std::string search = attr + "=\"";
  auto pos = tag.find(search);
  if (pos == std::string::npos) return "";
  pos += search.size();
  auto end = tag.find('"', pos);
  if (end == std::string::npos) return "";
  return tag.substr(pos, end - pos);
}

// =====================================================
// IPSecure implementation
// =====================================================

IPSecure::IPSecure()
  : enabled(false), next_session_id(1)
{
  memset(serial_number, 0, 6);
  memset(device_auth_key, 0, IPSEC_KEY_SIZE);
}

IPSecure::~IPSecure() {
  memset(device_auth_key, 0, IPSEC_KEY_SIZE);
}

void IPSecure::setSerialNumber(const uint8_t sno[6]) {
  memcpy(serial_number, sno, 6);
}

void IPSecure::deriveDeviceAuthKey(const std::string& password, uint8_t key[IPSEC_KEY_SIZE]) {
  const char* salt = "device-authentication-code.1.secure.ip.knx.org";
  PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                     (const uint8_t*)salt, strlen(salt),
                     65536, EVP_sha256(), IPSEC_KEY_SIZE, key);
}

void IPSecure::deriveUserPwdHash(const std::string& password, uint8_t key[IPSEC_KEY_SIZE]) {
  const char* salt = "user-password.1.secure.ip.knx.org";
  PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                     (const uint8_t*)salt, strlen(salt),
                     65536, EVP_sha256(), IPSEC_KEY_SIZE, key);
}

void IPSecure::setDeviceAuthPassword(const std::string& password) {
  deriveDeviceAuthKey(password, device_auth_key);
  enabled = true;
}

void IPSecure::setUserPassword(uint8_t userId, const std::string& password) {
  std::vector<uint8_t> hash(IPSEC_KEY_SIZE);
  deriveUserPwdHash(password, hash.data());
  user_pwd_hashes[userId] = hash;
}

void IPSecure::xorBytes(uint8_t* out, const uint8_t* a, const uint8_t* b, size_t len) {
  for (size_t i = 0; i < len; i++)
    out[i] = a[i] ^ b[i];
}

uint16_t IPSecure::allocSessionId() {
  // Find a free session ID (1..0xFFFE, 0 is reserved for multicast)
  for (int i = 0; i < 0xFFFE; i++) {
    uint16_t id = next_session_id++;
    if (next_session_id > 0xFFFE) next_session_id = 1;
    if (sessions.find(id) == sessions.end())
      return id;
  }
  return 0; // all full
}

SecureSession* IPSecure::findSession(uint16_t session_id) {
  auto it = sessions.find(session_id);
  if (it == sessions.end()) return nullptr;
  return &it->second;
}

void IPSecure::removeSession(uint16_t session_id) {
  sessions.erase(session_id);
}

// =====================================================
// CBC-MAC (16-byte) for IP Secure
// =====================================================

bool IPSecure::computeMAC16(const uint8_t key[IPSEC_KEY_SIZE],
                            const uint8_t b0[16],
                            const uint8_t* aad, size_t aad_len,
                            const uint8_t* payload, size_t payload_len,
                            uint8_t mac[IPSEC_MAC_SIZE]) {
  // Input: B0(16) + len_aad(2) + aad + payload, padded to 16 bytes
  size_t input_len = 16 + 2 + aad_len + payload_len;
  size_t padded_len = ((input_len + 15) / 16) * 16;

  std::vector<uint8_t> input(padded_len, 0);
  memcpy(input.data(), b0, 16);
  input[16] = (aad_len >> 8) & 0xFF;
  input[17] = aad_len & 0xFF;
  if (aad_len > 0)
    memcpy(input.data() + 18, aad, aad_len);
  if (payload_len > 0)
    memcpy(input.data() + 18 + aad_len, payload, payload_len);

  uint8_t iv[16] = {};
  std::vector<uint8_t> ct(padded_len + 16);
  int ct_len = 0;

  if (!aes_cbc_encrypt(key, iv, input.data(), padded_len, ct.data(), &ct_len))
    return false;
  if (ct_len < 16) return false;

  // MAC = last 16 bytes of CBC output
  memcpy(mac, ct.data() + ct_len - 16, IPSEC_MAC_SIZE);
  return true;
}

// =====================================================
// CTR encrypt in-place
// =====================================================

bool IPSecure::ctrEncrypt(const uint8_t key[IPSEC_KEY_SIZE],
                          const uint8_t ctr0[16],
                          uint8_t* data, size_t data_len) {
  // For IP Secure SECURE_WRAPPER, the CTR keystream is:
  //   Block 0 (bytes 0-15) → encrypts MAC (16 bytes)
  //   Block 1+ (bytes 16+) → encrypts payload
  // But on the wire the order is: [payload][MAC]
  // So we need to generate the full keystream and XOR manually.
  //
  // For handshake MACs (SESSION_RESPONSE, SESSION_AUTHENTICATE),
  // only 16 bytes of MAC are encrypted (no payload), so this function
  // works as a simple XOR with CTR block 0.

  // Generate keystream blocks using AES-ECB
  size_t total_ks_needed = IPSEC_MAC_SIZE + (data_len > IPSEC_MAC_SIZE ? data_len - IPSEC_MAC_SIZE : 0);
  size_t num_blocks = (total_ks_needed + 15) / 16;

  std::vector<uint8_t> keystream(num_blocks * 16);
  uint8_t counter[16];
  memcpy(counter, ctr0, 16);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return false;
  EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key, NULL);
  EVP_CIPHER_CTX_set_padding(ctx, 0);

  for (size_t b = 0; b < num_blocks; b++) {
    int outl = 0;
    EVP_EncryptUpdate(ctx, keystream.data() + b * 16, &outl, counter, 16);
    // Increment counter (last byte)
    for (int j = 15; j >= 0; j--) {
      if (++counter[j] != 0) break;
    }
  }
  EVP_CIPHER_CTX_free(ctx);

  if (data_len <= IPSEC_MAC_SIZE) {
    // Handshake MAC only (16 bytes)
    for (size_t i = 0; i < data_len; i++)
      data[i] ^= keystream[i];
  } else {
    // SecureWrapper: payload(data_len - 16) then MAC(16)
    // Payload XOR with keystream[16..]
    size_t payload_len = data_len - IPSEC_MAC_SIZE;
    for (size_t i = 0; i < payload_len; i++)
      data[i] ^= keystream[IPSEC_MAC_SIZE + i];
    // MAC XOR with keystream[0..15]
    for (size_t i = 0; i < IPSEC_MAC_SIZE; i++)
      data[payload_len + i] ^= keystream[i];
  }

  return true;
}

// =====================================================
// Handle IPSEC_SESSION_REQUEST → generate IPSEC_SESSION_RESPONSE
// =====================================================

std::vector<uint8_t> IPSecure::handleSessionRequest(const uint8_t* data, size_t len) {
  // IPSEC_SESSION_REQUEST: header(6) + HPAI(8) + client_pub_key(32) = 46 bytes
  if (len != 46) return {};

  // Verify header
  if (data[0] != KNXIP_HEADER_LEN || data[1] != KNXIP_VERSION) return {};
  uint16_t svc = getU16BE(data + 2);
  if (svc != IPSEC_SESSION_REQUEST) return {};

  // HPAI: must be TCP route-back (08 02 00000000 0000)
  // We don't strictly enforce this for compatibility

  // Extract client public key (bytes 14..45)
  const uint8_t* client_pub = data + 14;

  // Generate server ECDH key pair (X25519)
  EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
  if (!pctx) return {};
  EVP_PKEY_keygen_init(pctx);
  EVP_PKEY* server_key = NULL;
  EVP_PKEY_keygen(pctx, &server_key);
  EVP_PKEY_CTX_free(pctx);
  if (!server_key) return {};

  // Extract server public key (raw 32 bytes)
  uint8_t server_pub[IPSEC_ECDH_SIZE];
  size_t pub_len = IPSEC_ECDH_SIZE;
  EVP_PKEY_get_raw_public_key(server_key, server_pub, &pub_len);

  // Load client public key as EVP_PKEY
  EVP_PKEY* client_key = EVP_PKEY_new_raw_public_key(EVP_PKEY_X25519, NULL,
                                                       client_pub, IPSEC_ECDH_SIZE);
  if (!client_key) {
    EVP_PKEY_free(server_key);
    return {};
  }

  // ECDH key agreement → shared secret
  EVP_PKEY_CTX* dctx = EVP_PKEY_CTX_new(server_key, NULL);
  EVP_PKEY_derive_init(dctx);
  EVP_PKEY_derive_set_peer(dctx, client_key);
  size_t secret_len = 0;
  EVP_PKEY_derive(dctx, NULL, &secret_len);
  std::vector<uint8_t> shared_secret(secret_len);
  EVP_PKEY_derive(dctx, shared_secret.data(), &secret_len);
  EVP_PKEY_CTX_free(dctx);
  EVP_PKEY_free(client_key);
  EVP_PKEY_free(server_key);

  // Derive session key: SHA-256(shared_secret)[0:16]
  uint8_t hash[32];
  SHA256(shared_secret.data(), secret_len, hash);
  // Clear shared secret
  memset(shared_secret.data(), 0, secret_len);

  // Allocate session
  uint16_t sid = allocSessionId();
  if (sid == 0) return {};

  SecureSession& session = sessions[sid];
  session.session_id = sid;
  memcpy(session.session_key, hash, IPSEC_KEY_SIZE);
  memset(hash, 0, 32);
  session.send_seq = 0;
  session.recv_seq = 0;
  session.user_id = 0;
  session.state = SecureSession::UNAUTHENTICATED;

  // Store XOR of client and server public keys (for authenticate step)
  xorBytes(session.xor_client_server, client_pub, server_pub, IPSEC_ECDH_SIZE);

  // Build IPSEC_SESSION_RESPONSE: header(6) + session_id(2) + server_pub(32) + mac(16) = 56 bytes
  std::vector<uint8_t> resp(56);
  buildKNXIPHeader(resp.data(), IPSEC_SESSION_RESPONSE, 56);
  putU16BE(resp.data() + 6, sid);
  memcpy(resp.data() + 8, server_pub, IPSEC_ECDH_SIZE);

  // Compute MAC for device authentication
  // B0: all zeros, Q=0
  uint8_t b0[16] = {};

  // AAD = header(6) + session_id(2) + XOR(client_pub, server_pub)
  // Total: 6 + 2 + 32 = 40 bytes
  uint8_t aad[40];
  memcpy(aad, resp.data(), 6);       // header
  memcpy(aad + 6, resp.data() + 6, 2); // session_id
  memcpy(aad + 8, session.xor_client_server, IPSEC_ECDH_SIZE);

  uint8_t mac[IPSEC_MAC_SIZE];
  computeMAC16(device_auth_key, b0, aad, 40, nullptr, 0, mac);

  // Encrypt MAC with CTR: Ctr0 = 00..00 FF 00
  uint8_t ctr0[16] = {};
  ctr0[14] = 0xFF;
  ctr0[15] = 0x00;
  ctrEncrypt(device_auth_key, ctr0, mac, IPSEC_MAC_SIZE);

  memcpy(resp.data() + 40, mac, IPSEC_MAC_SIZE);

  return resp;
}

// =====================================================
// Handle IPSEC_SESSION_AUTHENTICATE
// =====================================================

bool IPSecure::handleSessionAuthenticate(uint16_t session_id,
                                         const uint8_t* data, size_t len) {
  // IPSEC_SESSION_AUTHENTICATE inner frame: header(6) + reserved(1) + user_id(1) + mac(16) = 24 bytes
  if (len != 24) return false;

  // Verify it's a IPSEC_SESSION_AUTHENTICATE
  uint16_t svc = getU16BE(data + 2);
  if (svc != IPSEC_SESSION_AUTHENTICATE) return false;

  // Reserved byte must be 0
  if (data[6] != 0x00) return false;

  uint8_t userId = data[7];
  if (userId < 1 || userId > 0x7F) return false;

  auto* session = findSession(session_id);
  if (!session) return false;
  if (session->state != SecureSession::UNAUTHENTICATED) return false;

  // Check if we have this user's password hash
  auto it = user_pwd_hashes.find(userId);
  if (it == user_pwd_hashes.end()) return false;
  const uint8_t* user_key = it->second.data();

  // Extract received MAC
  uint8_t recv_mac[IPSEC_MAC_SIZE];
  memcpy(recv_mac, data + 8, IPSEC_MAC_SIZE);

  // Compute expected MAC
  // B0: all zeros, Q=0
  uint8_t b0[16] = {};

  // AAD = header(6) + reserved+userId(2) + XOR(client_pub, server_pub)
  // Total: 6 + 2 + 32 = 40 bytes
  uint8_t aad[40];
  memcpy(aad, data, 6);        // header (06 10 09 53 00 18)
  aad[6] = data[6];            // reserved = 0x00
  aad[7] = userId;
  memcpy(aad + 8, session->xor_client_server, IPSEC_ECDH_SIZE);

  uint8_t expected_mac[IPSEC_MAC_SIZE];
  computeMAC16(user_key, b0, aad, 40, nullptr, 0, expected_mac);

  // Encrypt expected MAC with CTR (same as client did)
  uint8_t ctr0[16] = {};
  ctr0[14] = 0xFF;
  ctr0[15] = 0x00;
  ctrEncrypt(user_key, ctr0, expected_mac, IPSEC_MAC_SIZE);

  if (memcmp(recv_mac, expected_mac, IPSEC_MAC_SIZE) != 0)
    return false;

  session->user_id = userId;
  session->state = SecureSession::AUTHENTICATED;
  return true;
}

// =====================================================
// Unwrap IPSEC_SECURE_WRAPPER
// =====================================================

std::vector<uint8_t> IPSecure::unwrapSecure(const uint8_t* data, size_t len,
                                             uint16_t& session_id_out) {
  // Minimum: header(6) + session_id(2) + seq(6) + serial(6) + tag(2) + inner_header(6) + mac(16) = 44
  if (len < 44) return {};

  // Parse header
  uint16_t svc = getU16BE(data + 2);
  if (svc != IPSEC_SECURE_WRAPPER) return {};
  uint16_t total_len = getU16BE(data + 4);
  if (total_len != len) return {};

  // Extract security info
  uint16_t sid = getU16BE(data + 6);
  session_id_out = sid;

  auto* session = findSession(sid);
  if (!session) return {};

  // Sequence info (6 bytes at offset 8)
  uint64_t seq = getU48BE(data + 8);

  // Validate sequence number (must be strictly increasing)
  if (seq <= session->recv_seq && session->recv_seq > 0)
    return {};

  // Encrypted portion: from offset 22 to end (payload + MAC)
  size_t encrypted_offset = KNXIP_HEADER_LEN + 2 + 6 + 6 + 2; // = 22
  size_t encrypted_len = len - encrypted_offset;

  if (encrypted_len < IPSEC_MAC_SIZE) return {};
  size_t inner_len = encrypted_len - IPSEC_MAC_SIZE;

  // Build CTR0 for decryption: seq(6) + serial(6) + tag(2) + FF 00
  uint8_t ctr0[16];
  memcpy(ctr0, data + 8, 6);   // seq
  memcpy(ctr0 + 6, data + 14, 6); // serial
  memcpy(ctr0 + 12, data + 20, 2); // tag
  ctr0[14] = 0xFF;
  ctr0[15] = 0x00;

  // Decrypt in place (copy first)
  std::vector<uint8_t> decrypted(encrypted_len);
  memcpy(decrypted.data(), data + encrypted_offset, encrypted_len);
  ctrEncrypt(session->session_key, ctr0, decrypted.data(), encrypted_len);

  // After CTR decryption:
  // decrypted[0..inner_len-1] = plain KNXnet/IP frame
  // decrypted[inner_len..inner_len+15] = decrypted MAC

  // Verify CBC-MAC
  // B0: seq(6) + serial(6) + tag(2) + payload_len(2)
  uint8_t b0[16];
  memcpy(b0, data + 8, 6);     // seq
  memcpy(b0 + 6, data + 14, 6); // serial
  memcpy(b0 + 12, data + 20, 2); // tag
  putU16BE(b0 + 14, inner_len);

  // AAD: wrapper_header(6) + session_id(2)
  uint8_t aad[8];
  memcpy(aad, data, 6);        // wrapper header
  putU16BE(aad + 6, sid);

  uint8_t expected_mac[IPSEC_MAC_SIZE];
  computeMAC16(session->session_key, b0, aad, 8,
               decrypted.data(), inner_len, expected_mac);

  if (memcmp(decrypted.data() + inner_len, expected_mac, IPSEC_MAC_SIZE) != 0)
    return {};

  // MAC verified, update sequence
  session->recv_seq = seq;

  // Return inner frame
  return std::vector<uint8_t>(decrypted.begin(), decrypted.begin() + inner_len);
}

// =====================================================
// Wrap frame in IPSEC_SECURE_WRAPPER
// =====================================================

std::vector<uint8_t> IPSecure::wrapSecure(uint16_t session_id,
                                           const uint8_t* knxip_frame, size_t frame_len) {
  auto* session = findSession(session_id);
  if (!session) return {};

  uint64_t seq = session->send_seq++;

  // Total = header(6) + sid(2) + seq(6) + serial(6) + tag(2) + frame + mac(16)
  size_t total = KNXIP_HEADER_LEN + 2 + 6 + 6 + 2 + frame_len + IPSEC_MAC_SIZE;

  std::vector<uint8_t> packet(total);
  buildKNXIPHeader(packet.data(), IPSEC_SECURE_WRAPPER, total);
  putU16BE(packet.data() + 6, session_id);
  putU48BE(packet.data() + 8, seq);
  memcpy(packet.data() + 14, serial_number, 6);
  putU16BE(packet.data() + 20, 0x0000); // message tag = 0 for unicast

  // Place plaintext frame and MAC placeholder
  size_t payload_offset = 22;
  memcpy(packet.data() + payload_offset, knxip_frame, frame_len);

  // Compute CBC-MAC
  uint8_t b0[16];
  putU48BE(b0, seq);
  memcpy(b0 + 6, serial_number, 6);
  putU16BE(b0 + 12, 0x0000); // tag
  putU16BE(b0 + 14, frame_len);

  uint8_t aad[8];
  memcpy(aad, packet.data(), 6); // wrapper header
  putU16BE(aad + 6, session_id);

  uint8_t mac[IPSEC_MAC_SIZE];
  computeMAC16(session->session_key, b0, aad, 8,
               knxip_frame, frame_len, mac);

  memcpy(packet.data() + payload_offset + frame_len, mac, IPSEC_MAC_SIZE);

  // CTR encrypt payload + MAC
  uint8_t ctr0[16];
  putU48BE(ctr0, seq);
  memcpy(ctr0 + 6, serial_number, 6);
  putU16BE(ctr0 + 12, 0x0000); // tag
  ctr0[14] = 0xFF;
  ctr0[15] = 0x00;

  ctrEncrypt(session->session_key, ctr0,
             packet.data() + payload_offset, frame_len + IPSEC_MAC_SIZE);

  return packet;
}

// =====================================================
// Build SESSION_STATUS wrapped in IPSEC_SECURE_WRAPPER
// =====================================================

std::vector<uint8_t> IPSecure::buildSessionStatus(uint16_t session_id, uint8_t status) {
  // Inner frame: header(6) + status(1) + reserved(1) = 8 bytes
  uint8_t inner[8];
  buildKNXIPHeader(inner, IPSEC_SESSION_STATUS, 8);
  inner[6] = status;
  inner[7] = 0x00;

  return wrapSecure(session_id, inner, 8);
}

// =====================================================
// Keyring loading
// =====================================================

bool IPSecure::loadKeyring(const std::string& path, const std::string& password) {
  std::ifstream f(path);
  if (!f.is_open()) return false;
  std::string xml((std::istreambuf_iterator<char>(f)),
                   std::istreambuf_iterator<char>());
  f.close();

  // Hash keyring password
  const char* salt = "1.keyring.ets.knx.org";
  uint8_t pwd_hash[16];
  PKCS5_PBKDF2_HMAC(password.c_str(), password.size(),
                     (const uint8_t*)salt, strlen(salt),
                     65536, EVP_sha256(), 16, pwd_hash);

  // Derive IV from Created timestamp
  std::string created;
  {
    auto pos = xml.find("Created=\"");
    if (pos != std::string::npos) {
      pos += 9;
      auto end = xml.find('"', pos);
      if (end != std::string::npos)
        created = xml.substr(pos, end - pos);
    }
  }
  if (created.empty()) return false;

  uint8_t created_hash[32];
  SHA256((const uint8_t*)created.c_str(), created.size(), created_hash);
  uint8_t iv[16];
  memcpy(iv, created_hash, 16);

  // Parse Interfaces for device authentication and user passwords
  // Look for the Interface that matches our setup (Tunneling type)
  {
    size_t pos = 0;
    while ((pos = xml.find("<Interface ", pos)) != std::string::npos) {
      size_t tag_end = xml.find(">", pos);
      if (tag_end == std::string::npos) break;
      std::string tag = xml.substr(pos, tag_end - pos + 1);
      pos = tag_end + 1;

      std::string type = xmlAttr(tag, "Type");
      std::string auth_b64 = xmlAttr(tag, "Authentication");
      std::string pwd_b64 = xmlAttr(tag, "Password");

      // Device authentication code
      if (!auth_b64.empty()) {
        auto enc = b64decode(auth_b64);
        if (enc.size() >= 16) {
          while (enc.size() % 16 != 0) enc.push_back(0);
          uint8_t dec[32];
          int dl = 0;
          aes_cbc_decrypt(pwd_hash, iv, enc.data(), enc.size(), dec, &dl);
          memcpy(device_auth_key, dec, IPSEC_KEY_SIZE);
          enabled = true;
        }
      }

      // User password (from keyring this is the pre-hashed value)
      if (!pwd_b64.empty()) {
        auto enc = b64decode(pwd_b64);
        if (enc.size() >= 16) {
          while (enc.size() % 16 != 0) enc.push_back(0);
          uint8_t dec[32];
          int dl = 0;
          aes_cbc_decrypt(pwd_hash, iv, enc.data(), enc.size(), dec, &dl);
          // User ID from the Interface's individual address (UserID attribute)
          std::string uid_str = xmlAttr(tag, "UserID");
          if (!uid_str.empty()) {
            uint8_t uid = (uint8_t)std::stoi(uid_str);
            user_pwd_hashes[uid] = std::vector<uint8_t>(dec, dec + IPSEC_KEY_SIZE);
          }
        }
      }
    }
  }

  return enabled;
}
