# -*- cmake -*-
include(Prebuilt)

set(OpenSSL_FIND_QUIETLY ON)
set(OpenSSL_FIND_REQUIRED ON)

if (STANDALONE OR USE_SYSTEM_OPENSSL)
  include(FindOpenSSL)
else (STANDALONE OR USE_SYSTEM_OPENSSL)
  use_prebuilt_binary(openssl)
  if (WINDOWS)
    # OpenSSL 1.1.x package names from Linden 3p-openssl.
    # Crypt32 is required for the Windows CAPI engine (Cert* APIs).
    set(OPENSSL_LIBRARIES libssl libcrypto crypt32)
  else (WINDOWS)
    set(OPENSSL_LIBRARIES ssl)
  endif (WINDOWS)
  set(OPENSSL_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE OR USE_SYSTEM_OPENSSL)

set(CRYPTO_LIBRARIES "")
