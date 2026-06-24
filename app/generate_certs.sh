#!/usr/bin/env bash
# ============================================================================
# generate_certs.sh — Generowanie certyfikatów self-signed dla Redis TLS
# ============================================================================
set -euo pipefail

CERTS_DIR="./certs"
DAYS=365

echo ">>> Tworzenie katalogu ${CERTS_DIR}..."
mkdir -p "${CERTS_DIR}"

# ---------- 1. Klucz i certyfikat CA ----------
echo ">>> Generowanie klucza prywatnego CA..."
openssl genrsa -out "${CERTS_DIR}/ca.key" 4096

echo ">>> Generowanie certyfikatu CA (self-signed)..."
openssl req -x509 -new -nodes \
  -key "${CERTS_DIR}/ca.key" \
  -sha256 -days ${DAYS} \
  -subj "/O=Kluwak IoT/CN=Redis Dev CA" \
  -out "${CERTS_DIR}/ca.crt"

# ---------- 2. Klucz i certyfikat serwera Redis ----------
echo ">>> Generowanie klucza prywatnego serwera Redis..."
openssl genrsa -out "${CERTS_DIR}/redis.key" 2048

echo ">>> Generowanie CSR serwera..."
openssl req -new \
  -key "${CERTS_DIR}/redis.key" \
  -subj "/O=Kluwak IoT/CN=redis" \
  -out "${CERTS_DIR}/redis.csr"

echo ">>> Podpisywanie certyfikatu serwera przez CA..."
openssl x509 -req \
  -in "${CERTS_DIR}/redis.csr" \
  -CA "${CERTS_DIR}/ca.crt" \
  -CAkey "${CERTS_DIR}/ca.key" \
  -CAcreateserial \
  -days ${DAYS} -sha256 \
  -extfile <(printf "subjectAltName=DNS:redis,DNS:localhost,IP:127.0.0.1") \
  -out "${CERTS_DIR}/redis.crt"

# ---------- 3. Uprawnienia ----------
chmod 600 "${CERTS_DIR}/redis.key" "${CERTS_DIR}/ca.key"
chmod 644 "${CERTS_DIR}/redis.crt" "${CERTS_DIR}/ca.crt"

# ---------- 4. Porządki ----------
rm -f "${CERTS_DIR}/redis.csr" "${CERTS_DIR}/ca.srl"

echo ""
echo "=== Certyfikaty wygenerowane pomyślnie ==="
ls -la "${CERTS_DIR}"
echo ""
