#ifndef CORE_SOCKET_STREAM_TLS_DETAIL_TLSHANDOFFOPERATIONS_H
#define CORE_SOCKET_STREAM_TLS_DETAIL_TLSHANDOFFOPERATIONS_H

#include <cstddef>
#include <openssl/bio.h>
#include <openssl/ssl.h>

namespace core::socket::stream::tls::detail {

    int tlsHandoffSslPending(SSL* ssl);
    int tlsHandoffSslRead(SSL* ssl, char* buffer, std::size_t size);
    std::size_t tlsHandoffBioPending(BIO* bio);
    int tlsHandoffBioRead(BIO* bio, char* buffer, std::size_t size);
    int tlsHandoffSslHasPending(SSL* ssl);

} // namespace core::socket::stream::tls::detail

#endif
