// lib/network/secrets.h
#pragma once
#include <pgmspace.h>

// 1. Amazon Root CA 1 (Chứng chỉ gốc của AWS)
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIEkjCCA3qgAwIBAgITBn+US8... (Copy từ file Root CA)
-----END CERTIFICATE-----
)EOF";

// 2. Device Certificate (Chứng chỉ của riêng board ESP32 này)
static const char AWS_CERT_CRT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUU... (Copy từ file xxxxxxxxxx-certificate.pem.crt)
-----END CERTIFICATE-----
)EOF";

// 3. Device Private Key (Khóa bí mật - TUYỆT ĐỐI KHÔNG ĐỂ LỘ)
static const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEA... (Copy từ file xxxxxxxxxx-private.pem.key)
-----END RSA PRIVATE KEY-----
)EOF";