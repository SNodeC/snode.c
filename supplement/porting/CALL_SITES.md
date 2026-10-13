# Source occurrences

Commit `d4b2b247888fac35411e7e38b5ae8e5a4dee22c8`. This ledger retains declarations and definitions as evidence, separately from calls. Line numbers refer to the audited source revision. Preprocessor guards are recorded in `occurrences.json`; they are textual branch context, not an evaluated build configuration.

## ASN1_INTEGER_set

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:215](../../tests/component/net/AddressResolutionPolicyTest.cpp#L215) | inline call | `ASN1_INTEGER_set(X509_get_serialNumber(certificate), 1);` |
| [tests/support/NetworkTest.h:68](../../tests/support/NetworkTest.h#L68) | inline call | `ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);` |

## ASN1_STRING_get0_data

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:144](../../src/apps/echo/model/clients.h#L144) | inline call | `std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),` |
| [src/apps/echo/model/clients.h:149](../../src/apps/echo/model/clients.h#L149) | inline call | `std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),` |
| [src/apps/echo/model/servers.h:144](../../src/apps/echo/model/servers.h#L144) | inline call | `std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),` |
| [src/apps/echo/model/servers.h:149](../../src/apps/echo/model/servers.h#L149) | inline call | `std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),` |
| [src/apps/http/httplowlevelclient.cpp:200](../../src/apps/http/httplowlevelclient.cpp#L200) | inline call | `reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),` |
| [src/apps/http/httplowlevelclient.cpp:205](../../src/apps/http/httplowlevelclient.cpp#L205) | inline call | `std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),` |
| [src/apps/http/model/clients.h:730](../../src/apps/http/model/clients.h#L730) | inline call | `std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),` |
| [src/apps/http/model/clients.h:735](../../src/apps/http/model/clients.h#L735) | inline call | `std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),` |
| [src/apps/http/model/servers.h:154](../../src/apps/http/model/servers.h#L154) | inline call | `std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),` |
| [src/apps/http/model/servers.h:159](../../src/apps/http/model/servers.h#L159) | inline call | `std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),` |
| [src/apps/testregex.cpp:397](../../src/apps/testregex.cpp#L397) | inline call | `std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),` |
| [src/apps/testregex.cpp:402](../../src/apps/testregex.cpp#L402) | inline call | `std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),` |
| [src/core/socket/stream/tls/ssl_utils.cpp:271](../../src/core/socket/stream/tls/ssl_utils.cpp#L271) | inline call | `std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),` |

## ASN1_STRING_length

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:145](../../src/apps/echo/model/clients.h#L145) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));` |
| [src/apps/echo/model/clients.h:150](../../src/apps/echo/model/clients.h#L150) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));` |
| [src/apps/echo/model/servers.h:145](../../src/apps/echo/model/servers.h#L145) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));` |
| [src/apps/echo/model/servers.h:150](../../src/apps/echo/model/servers.h#L150) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));` |
| [src/apps/http/httplowlevelclient.cpp:201](../../src/apps/http/httplowlevelclient.cpp#L201) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));` |
| [src/apps/http/httplowlevelclient.cpp:206](../../src/apps/http/httplowlevelclient.cpp#L206) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));` |
| [src/apps/http/model/clients.h:731](../../src/apps/http/model/clients.h#L731) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));` |
| [src/apps/http/model/clients.h:736](../../src/apps/http/model/clients.h#L736) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));` |
| [src/apps/http/model/servers.h:155](../../src/apps/http/model/servers.h#L155) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));` |
| [src/apps/http/model/servers.h:160](../../src/apps/http/model/servers.h#L160) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));` |
| [src/apps/testregex.cpp:398](../../src/apps/testregex.cpp#L398) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));` |
| [src/apps/testregex.cpp:403](../../src/apps/testregex.cpp#L403) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));` |
| [src/core/socket/stream/tls/ssl_utils.cpp:272](../../src/core/socket/stream/tls/ssl_utils.cpp#L272) | inline call | `static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));` |

## CMSG_DATA

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/dgram/Socket.cpp:91](../../src/net/un/dgram/Socket.cpp#L91) | macro use | `*reinterpret_cast<int*>(CMSG_DATA(cmptr)) = sendfd;` |
| [src/net/un/dgram/Socket.cpp:129](../../src/net/un/dgram/Socket.cpp#L129) | macro use | `*recvfd = *reinterpret_cast<int*>(CMSG_DATA(cmptr));` |

## CMSG_FIRSTHDR

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/dgram/Socket.cpp:88](../../src/net/un/dgram/Socket.cpp#L88) | macro use | `cmsghdr* cmptr = CMSG_FIRSTHDR(&msg);` |
| [src/net/un/dgram/Socket.cpp:123](../../src/net/un/dgram/Socket.cpp#L123) | macro use | `if ((cmptr = CMSG_FIRSTHDR(&msg)) != nullptr && cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {` |

## CMSG_LEN

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/dgram/Socket.cpp:92](../../src/net/un/dgram/Socket.cpp#L92) | macro use | `cmptr->cmsg_len = CMSG_LEN(sizeof(int));` |
| [src/net/un/dgram/Socket.cpp:123](../../src/net/un/dgram/Socket.cpp#L123) | macro use | `if ((cmptr = CMSG_FIRSTHDR(&msg)) != nullptr && cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {` |

## CMSG_SPACE

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/dgram/Socket.cpp:70](../../src/net/un/dgram/Socket.cpp#L70) | macro use | `char control[CMSG_SPACE(sizeof(int))] = {};` |
| [src/net/un/dgram/Socket.cpp:100](../../src/net/un/dgram/Socket.cpp#L100) | macro use | `char control[CMSG_SPACE(sizeof(int))] = {};` |

## CommandLineToArgvW

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:476](../../src/utils/CLI11.hpp#L476) | inline call | `auto wargv = std::unique_ptr<wchar_t *[], decltype(deleter)>(CommandLineToArgvW(GetCommandLineW(), &argc), deleter);` |

## ERR_GET_REASON

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/detail/TLSResult.h:53](../../src/core/socket/stream/tls/detail/TLSResult.h#L53) | macro use | `return openSslError != 0 && ERR_GET_REASON(openSslError) == SSL_R_UNEXPECTED_EOF_WHILE_READING;` |

## ERR_clear_error

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketReader.cpp:92](../../src/core/socket/stream/tls/SocketReader.cpp#L92) | inline call | `ERR_clear_error();` |
| [src/core/socket/stream/tls/SocketWriter.cpp:89](../../src/core/socket/stream/tls/SocketWriter.cpp#L89) | inline call | `ERR_clear_error();` |
| [src/core/socket/stream/tls/TLSHandshake.cpp:141](../../src/core/socket/stream/tls/TLSHandshake.cpp#L141) | inline call | `ERR_clear_error();` |
| [src/core/socket/stream/tls/TLSShutdown.cpp:190](../../src/core/socket/stream/tls/TLSShutdown.cpp#L190) | inline call | `ERR_clear_error();` |
| [src/core/socket/stream/tls/TLSShutdown.cpp:215](../../src/core/socket/stream/tls/TLSShutdown.cpp#L215) | inline call | `ERR_clear_error();` |
| [tests/unit/core/TLSResultClassificationTest.cpp:15](../../tests/unit/core/TLSResultClassificationTest.cpp#L15) | inline call | `ERR_clear_error();` |
| [tests/unit/core/TLSResultClassificationTest.cpp:18](../../tests/unit/core/TLSResultClassificationTest.cpp#L18) | inline call | `ERR_clear_error();` |
| [tests/unit/core/TlsOpenSslLoggingTest.cpp:28](../../tests/unit/core/TlsOpenSslLoggingTest.cpp#L28) | inline call | `ERR_clear_error();` |
| [tests/unit/core/TlsOpenSslLoggingTest.cpp:46](../../tests/unit/core/TlsOpenSslLoggingTest.cpp#L46) | inline call | `ERR_clear_error();` |

## ERR_error_string

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:89](../../src/core/socket/stream/tls/ssl_utils.cpp#L89) | inline call | `log.emit(level, std::string("  ") + ERR_error_string(firstErrorCode, nullptr));` |
| [src/core/socket/stream/tls/ssl_utils.cpp:93](../../src/core/socket/stream/tls/ssl_utils.cpp#L93) | inline call | `log.emit(level, std::string("  ") + ERR_error_string(errorCode, nullptr));` |

## ERR_error_string_n

| Source | Role | Source line |
| --- | --- | --- |
| [tests/unit/core/TlsOpenSslLoggingTest.cpp:62](../../tests/unit/core/TlsOpenSslLoggingTest.cpp#L62) | inline call | `ERR_error_string_n(ERR_peek_error(), expectedError, sizeof(expectedError));` |

## ERR_get_error

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:88](../../src/core/socket/stream/tls/ssl_utils.cpp#L88) | inline call | `const unsigned long firstErrorCode = ERR_get_error();` |
| [src/core/socket/stream/tls/ssl_utils.cpp:92](../../src/core/socket/stream/tls/ssl_utils.cpp#L92) | inline call | `while ((errorCode = ERR_get_error()) != 0) {` |

## ERR_new

| Source | Role | Source line |
| --- | --- | --- |
| [tests/unit/core/TlsOpenSslLoggingTest.cpp:47](../../tests/unit/core/TlsOpenSslLoggingTest.cpp#L47) | inline call | `ERR_new();` |

## ERR_peek_error

| Source | Role | Source line |
| --- | --- | --- |
| [tests/unit/core/TlsOpenSslLoggingTest.cpp:62](../../tests/unit/core/TlsOpenSslLoggingTest.cpp#L62) | inline call | `ERR_error_string_n(ERR_peek_error(), expectedError, sizeof(expectedError));` |
| [tests/unit/core/TlsOpenSslLoggingTest.cpp:66](../../tests/unit/core/TlsOpenSslLoggingTest.cpp#L66) | inline call | `result.expectEqual(static_cast<unsigned long>(0), ERR_peek_error(),` |
| [tests/unit/core/TlsOpenSslLoggingTest.cpp:83](../../tests/unit/core/TlsOpenSslLoggingTest.cpp#L83) | inline call | `result.expectTrue(ERR_peek_error() != 0, "suppressed ssl_log_" + label + " does not drain OpenSSL error queue");` |

## ERR_peek_last_error

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketReader.cpp:101](../../src/core/socket/stream/tls/SocketReader.cpp#L101) | inline call | `const unsigned long openSslError = ERR_peek_last_error();` |
| [src/core/socket/stream/tls/SocketWriter.cpp:98](../../src/core/socket/stream/tls/SocketWriter.cpp#L98) | inline call | `const unsigned long openSslError = ERR_peek_last_error();` |
| [src/core/socket/stream/tls/TLSHandshake.cpp:150](../../src/core/socket/stream/tls/TLSHandshake.cpp#L150) | inline call | `const unsigned long openSslError = ERR_peek_last_error();` |
| [src/core/socket/stream/tls/TLSShutdown.cpp:207](../../src/core/socket/stream/tls/TLSShutdown.cpp#L207) | inline call | `const unsigned long openSslError = ERR_peek_last_error();` |
| [src/core/socket/stream/tls/TLSShutdown.cpp:234](../../src/core/socket/stream/tls/TLSShutdown.cpp#L234) | inline call | `const unsigned long openSslError = ERR_peek_last_error();` |
| [tests/unit/core/TLSResultClassificationTest.cpp:17](../../tests/unit/core/TLSResultClassificationTest.cpp#L17) | inline call | `const unsigned long err = ERR_peek_last_error();` |

## ERR_put_error

| Source | Role | Source line |
| --- | --- | --- |
| [tests/unit/core/TLSResultClassificationTest.cpp:16](../../tests/unit/core/TLSResultClassificationTest.cpp#L16) | inline call | `ERR_put_error(ERR_LIB_SSL, 0, reason, __FILE__, __LINE__);` |

## ERR_set_error

| Source | Role | Source line |
| --- | --- | --- |
| [tests/unit/core/TlsOpenSslLoggingTest.cpp:48](../../tests/unit/core/TlsOpenSslLoggingTest.cpp#L48) | inline call | `ERR_set_error(ERR_LIB_SSL, SSL_R_SSLV3_ALERT_HANDSHAKE_FAILURE, nullptr);` |

## EVP_PKEY_CTX_free

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:210](../../tests/component/net/AddressResolutionPolicyTest.cpp#L210) | inline call | `EVP_PKEY_CTX_free(keyContext);` |
| [tests/support/NetworkTest.h:64](../../tests/support/NetworkTest.h#L64) | inline call | `EVP_PKEY_CTX_free(generator);` |

## EVP_PKEY_CTX_new_id

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:205](../../tests/component/net/AddressResolutionPolicyTest.cpp#L205) | inline call | `EVP_PKEY_CTX* keyContext = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);` |
| [tests/support/NetworkTest.h:60](../../tests/support/NetworkTest.h#L60) | inline call | `EVP_PKEY_CTX* generator = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);` |

## EVP_PKEY_CTX_set_ec_paramgen_curve_nid

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:208](../../tests/component/net/AddressResolutionPolicyTest.cpp#L208) | inline call | `EVP_PKEY_CTX_set_ec_paramgen_curve_nid(keyContext, NID_X9_62_prime256v1) == 1 &&` |
| [tests/support/NetworkTest.h:63](../../tests/support/NetworkTest.h#L63) | inline call | `EVP_PKEY_CTX_set_ec_paramgen_curve_nid(generator, NID_X9_62_prime256v1) == 1 && EVP_PKEY_keygen(generator, &key) == 1;` |

## EVP_PKEY_free

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:226](../../tests/component/net/AddressResolutionPolicyTest.cpp#L226) | inline call | `EVP_PKEY_free(key);` |
| [tests/support/NetworkTest.h:79](../../tests/support/NetworkTest.h#L79) | inline call | `EVP_PKEY_free(key);` |

## EVP_PKEY_keygen

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:209](../../tests/component/net/AddressResolutionPolicyTest.cpp#L209) | inline call | `EVP_PKEY_keygen(keyContext, &key) == 1;` |
| [tests/support/NetworkTest.h:63](../../tests/support/NetworkTest.h#L63) | inline call | `EVP_PKEY_CTX_set_ec_paramgen_curve_nid(generator, NID_X9_62_prime256v1) == 1 && EVP_PKEY_keygen(generator, &key) == 1;` |

## EVP_PKEY_keygen_init

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:207](../../tests/component/net/AddressResolutionPolicyTest.cpp#L207) | inline call | `const bool keyReady = keyContext && EVP_PKEY_keygen_init(keyContext) == 1 &&` |
| [tests/support/NetworkTest.h:62](../../tests/support/NetworkTest.h#L62) | inline call | `bool ok = generator && EVP_PKEY_keygen_init(generator) == 1 &&` |

## EVP_sha256

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:222](../../tests/component/net/AddressResolutionPolicyTest.cpp#L222) | inline call | `ready = X509_sign(certificate, key, EVP_sha256()) > 0 && SSL_CTX_use_certificate(context, certificate) == 1 &&` |
| [tests/support/NetworkTest.h:75](../../tests/support/NetworkTest.h#L75) | inline call | `ok = X509_sign(cert, key, EVP_sha256()) > 0 && SSL_CTX_use_certificate(ctx, cert) == 1 && SSL_CTX_use_PrivateKey(ctx, key) == 1;` |

## FD_CLR

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/multiplexer/select/DescriptorEventPublisher.cpp:66](../../src/core/multiplexer/select/DescriptorEventPublisher.cpp#L66) | macro use | `FD_CLR(fd, &registered);` |
| [src/core/multiplexer/select/DescriptorEventPublisher.cpp:67](../../src/core/multiplexer/select/DescriptorEventPublisher.cpp#L67) | macro use | `FD_CLR(fd, &active);` |

## FD_ISSET

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/multiplexer/select/DescriptorEventPublisher.cpp:71](../../src/core/multiplexer/select/DescriptorEventPublisher.cpp#L71) | macro use | `return FD_ISSET(fd, &active);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:111](../../src/tools/snodec-control/src/ProcessRunner.cpp#L111) | macro use | `if (outOpen && FD_ISSET(outPipe.readEnd, &readSet)) {` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:122](../../src/tools/snodec-control/src/ProcessRunner.cpp#L122) | macro use | `if (errOpen && FD_ISSET(errPipe.readEnd, &readSet)) {` |

## FD_SET

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/multiplexer/select/DescriptorEventPublisher.cpp:62](../../src/core/multiplexer/select/DescriptorEventPublisher.cpp#L62) | macro use | `FD_SET(fd, &registered);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:95](../../src/tools/snodec-control/src/ProcessRunner.cpp#L95) | macro use | `FD_SET(outPipe.readEnd, &readSet);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:99](../../src/tools/snodec-control/src/ProcessRunner.cpp#L99) | macro use | `FD_SET(errPipe.readEnd, &readSet);` |

## FD_ZERO

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/multiplexer/select/DescriptorEventPublisher.cpp:75](../../src/core/multiplexer/select/DescriptorEventPublisher.cpp#L75) | macro use | `FD_ZERO(&registered);` |
| [src/core/multiplexer/select/DescriptorEventPublisher.cpp:76](../../src/core/multiplexer/select/DescriptorEventPublisher.cpp#L76) | macro use | `FD_ZERO(&active);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:91](../../src/tools/snodec-control/src/ProcessRunner.cpp#L91) | macro use | `FD_ZERO(&readSet);` |

## GENERAL_NAME_free

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:157](../../src/apps/echo/model/clients.h#L157) | callback reference | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |
| [src/apps/echo/model/servers.h:157](../../src/apps/echo/model/servers.h#L157) | callback reference | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |
| [src/apps/http/httplowlevelclient.cpp:213](../../src/apps/http/httplowlevelclient.cpp#L213) | callback reference | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |
| [src/apps/http/model/clients.h:743](../../src/apps/http/model/clients.h#L743) | callback reference | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |
| [src/apps/http/model/servers.h:167](../../src/apps/http/model/servers.h#L167) | callback reference | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |
| [src/apps/testregex.cpp:410](../../src/apps/testregex.cpp#L410) | callback reference | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |
| [src/core/socket/stream/tls/ssl_utils.cpp:277](../../src/core/socket/stream/tls/ssl_utils.cpp#L277) | callback reference | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |

## GetCommandLineW

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:476](../../src/utils/CLI11.hpp#L476) | inline call | `auto wargv = std::unique_ptr<wchar_t *[], decltype(deleter)>(CommandLineToArgvW(GetCommandLineW(), &argc), deleter);` |

## GetLastError

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:480](../../src/utils/CLI11.hpp#L480) | inline call | `throw std::runtime_error("CommandLineToArgvW failed with code " + std::to_string(GetLastError()));` |

## KEY_F

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:733](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L733) | macro use | `case KEY_F(1):` |

## LocalFree

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:474](../../src/utils/CLI11.hpp#L474) | inline call | `auto deleter = [](wchar_t **ptr) { LocalFree(ptr); };` |

## OPENSSL_free

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:123](../../src/apps/echo/model/clients.h#L123) | inline call | `OPENSSL_free(str);` |
| [src/apps/echo/model/clients.h:129](../../src/apps/echo/model/clients.h#L129) | inline call | `OPENSSL_free(str);` |
| [src/apps/echo/model/servers.h:123](../../src/apps/echo/model/servers.h#L123) | inline call | `OPENSSL_free(str);` |
| [src/apps/echo/model/servers.h:129](../../src/apps/echo/model/servers.h#L129) | inline call | `OPENSSL_free(str);` |
| [src/apps/http/httplowlevelclient.cpp:179](../../src/apps/http/httplowlevelclient.cpp#L179) | inline call | `OPENSSL_free(str);` |
| [src/apps/http/httplowlevelclient.cpp:185](../../src/apps/http/httplowlevelclient.cpp#L185) | inline call | `OPENSSL_free(str);` |
| [src/apps/http/model/clients.h:709](../../src/apps/http/model/clients.h#L709) | inline call | `OPENSSL_free(str);` |
| [src/apps/http/model/clients.h:715](../../src/apps/http/model/clients.h#L715) | inline call | `OPENSSL_free(str);` |
| [src/apps/http/model/servers.h:132](../../src/apps/http/model/servers.h#L132) | inline call | `OPENSSL_free(str);` |
| [src/apps/http/model/servers.h:138](../../src/apps/http/model/servers.h#L138) | inline call | `OPENSSL_free(str);` |
| [src/apps/testregex.cpp:376](../../src/apps/testregex.cpp#L376) | inline call | `OPENSSL_free(str);` |
| [src/apps/testregex.cpp:382](../../src/apps/testregex.cpp#L382) | inline call | `OPENSSL_free(str);` |

## OPENSSL_sk_num

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/http/model/servers.h:146](../../src/apps/http/model/servers.h#L146) | inline call | `int32_t altNameCount = OPENSSL_sk_num(reinterpret_cast<const OPENSSL_STACK*>(subjectAltNames));` |

## SSL_CTX_check_private_key

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:225](../../src/core/socket/stream/tls/ssl_utils.cpp#L225) | inline call | `} else if (SSL_CTX_check_private_key(ctx) != 1) {` |

## SSL_CTX_free

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:248](../../src/core/socket/stream/tls/ssl_utils.cpp#L248) | inline call | `SSL_CTX_free(ctx);` |
| [src/core/socket/stream/tls/ssl_utils.cpp:303](../../src/core/socket/stream/tls/ssl_utils.cpp#L303) | inline call | `SSL_CTX_free(ctx);` |
| [tests/support/NetworkTest.h:89](../../tests/support/NetworkTest.h#L89) | inline call | `SSL_CTX_free(ctx);` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:79](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L79) | inline call | `SSL_CTX_free(serverCtx);` |

## SSL_CTX_get0_certificate

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:260](../../src/core/socket/stream/tls/ssl_utils.cpp#L260) | inline call | `const X509* x509 = SSL_CTX_get0_certificate(sslCtx);` |

## SSL_CTX_get_mode

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:296](../../src/core/socket/stream/tls/ssl_utils.cpp#L296) | inline call | `SSL_set_mode(ssl, SSL_CTX_get_mode(sslCtx));` |

## SSL_CTX_get_options

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:293](../../src/core/socket/stream/tls/ssl_utils.cpp#L293) | inline call | `SSL_set_options(ssl, SSL_CTX_get_options(sslCtx));` |

## SSL_CTX_get_verify_callback

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:294](../../src/core/socket/stream/tls/ssl_utils.cpp#L294) | inline call | `SSL_set_verify(ssl, SSL_CTX_get_verify_mode(sslCtx), SSL_CTX_get_verify_callback(sslCtx));` |

## SSL_CTX_get_verify_depth

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:295](../../src/core/socket/stream/tls/ssl_utils.cpp#L295) | inline call | `SSL_set_verify_depth(ssl, SSL_CTX_get_verify_depth(sslCtx));` |

## SSL_CTX_get_verify_mode

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:209](../../src/core/socket/stream/tls/ssl_utils.cpp#L209) | inline call | `if ((SSL_CTX_get_verify_mode(ctx) & SSL_VERIFY_PEER) != 0) {` |
| [src/core/socket/stream/tls/ssl_utils.cpp:294](../../src/core/socket/stream/tls/ssl_utils.cpp#L294) | inline call | `SSL_set_verify(ssl, SSL_CTX_get_verify_mode(sslCtx), SSL_CTX_get_verify_callback(sslCtx));` |

## SSL_CTX_load_verify_locations

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:167](../../src/core/socket/stream/tls/ssl_utils.cpp#L167) | inline call | `if (SSL_CTX_load_verify_locations(ctx,` |

## SSL_CTX_new

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:150](../../src/core/socket/stream/tls/ssl_utils.cpp#L150) | inline call | `SSL_CTX* ctx = SSL_CTX_new(sslConfig.server ? TLS_server_method() : TLS_client_method());` |
| [tests/support/NetworkTest.h:103](../../tests/support/NetworkTest.h#L103) | inline call | `ctx = SSL_CTX_new(TLS_client_method());` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:14](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L14) | inline call | `SSL_CTX* serverCtx = SSL_CTX_new(TLS_server_method());` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:22](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L22) | inline call | `peer.ctx = SSL_CTX_new(TLS_client_method());` |

## SSL_CTX_set_cipher_list

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:245](../../src/core/socket/stream/tls/ssl_utils.cpp#L245) | inline call | `SSL_CTX_set_cipher_list(ctx, sslConfig.cipherList.c_str());` |

## SSL_CTX_set_client_hello_cb

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketAcceptor.hpp:148](../../src/core/socket/stream/tls/SocketAcceptor.hpp#L148) | inline call | `SSL_CTX_set_client_hello_cb(sslCtx, clientHelloCallback, nullptr);` |

## SSL_CTX_set_default_passwd_cb

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:218](../../src/core/socket/stream/tls/ssl_utils.cpp#L218) | inline call | `SSL_CTX_set_default_passwd_cb(ctx, password_callback);` |

## SSL_CTX_set_default_passwd_cb_userdata

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:219](../../src/core/socket/stream/tls/ssl_utils.cpp#L219) | inline call | `SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(sslConfig.password.c_str()));` |

## SSL_CTX_set_default_verify_paths

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:192](../../src/core/socket/stream/tls/ssl_utils.cpp#L192) | inline call | `if (SSL_CTX_set_default_verify_paths(ctx) == 0) {` |

## SSL_CTX_set_mode

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:157](../../src/core/socket/stream/tls/ssl_utils.cpp#L157) | inline call | `SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);` |
| [src/core/socket/stream/tls/ssl_utils.cpp:158](../../src/core/socket/stream/tls/ssl_utils.cpp#L158) | inline call | `SSL_CTX_set_mode(ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);` |

## SSL_CTX_set_options

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:242](../../src/core/socket/stream/tls/ssl_utils.cpp#L242) | inline call | `SSL_CTX_set_options(ctx, sslConfig.sslOptions);` |

## SSL_CTX_set_read_ahead

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:155](../../src/core/socket/stream/tls/ssl_utils.cpp#L155) | inline call | `SSL_CTX_set_read_ahead(ctx, 0);` |

## SSL_CTX_set_session_id_context

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:163](../../src/core/socket/stream/tls/ssl_utils.cpp#L163) | inline call | `SSL_CTX_set_session_id_context(ctx, reinterpret_cast<const unsigned char*>(&sslSessionCtxId), sizeof(sslSessionCtxId));` |

## SSL_CTX_set_verify

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:203](../../src/core/socket/stream/tls/ssl_utils.cpp#L203) | inline call | `SSL_CTX_set_verify(ctx,` |
| [tests/support/NetworkTest.h:105](../../tests/support/NetworkTest.h#L105) | inline call | `SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);` |

## SSL_CTX_set_verify_depth

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:202](../../src/core/socket/stream/tls/ssl_utils.cpp#L202) | inline call | `SSL_CTX_set_verify_depth(ctx, 5);` |

## SSL_CTX_use_PrivateKey

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:223](../../tests/component/net/AddressResolutionPolicyTest.cpp#L223) | inline call | `SSL_CTX_use_PrivateKey(context, key) == 1;` |
| [tests/support/NetworkTest.h:75](../../tests/support/NetworkTest.h#L75) | inline call | `ok = X509_sign(cert, key, EVP_sha256()) > 0 && SSL_CTX_use_certificate(ctx, cert) == 1 && SSL_CTX_use_PrivateKey(ctx, key) == 1;` |

## SSL_CTX_use_PrivateKey_file

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:221](../../src/core/socket/stream/tls/ssl_utils.cpp#L221) | inline call | `if (SSL_CTX_use_PrivateKey_file(ctx, sslConfig.certKey.c_str(), SSL_FILETYPE_PEM) == 0) {` |

## SSL_CTX_use_certificate

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:222](../../tests/component/net/AddressResolutionPolicyTest.cpp#L222) | inline call | `ready = X509_sign(certificate, key, EVP_sha256()) > 0 && SSL_CTX_use_certificate(context, certificate) == 1 &&` |
| [tests/support/NetworkTest.h:75](../../tests/support/NetworkTest.h#L75) | inline call | `ok = X509_sign(cert, key, EVP_sha256()) > 0 && SSL_CTX_use_certificate(ctx, cert) == 1 && SSL_CTX_use_PrivateKey(ctx, key) == 1;` |

## SSL_CTX_use_certificate_chain_file

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:213](../../src/core/socket/stream/tls/ssl_utils.cpp#L213) | inline call | `if (SSL_CTX_use_certificate_chain_file(ctx, sslConfig.cert.c_str()) == 0) {` |

## SSL_clear_options

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:292](../../src/core/socket/stream/tls/ssl_utils.cpp#L292) | inline call | `SSL_clear_options(ssl, 0xFFFFFFFFL);` |

## SSL_client_hello_get0_ext

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:315](../../src/core/socket/stream/tls/ssl_utils.cpp#L315) | inline call | `if (SSL_client_hello_get0_ext(ssl, TLSEXT_TYPE_server_name, &ext, &ext_len) == 0) {` |

## SSL_connect

| Source | Role | Source line |
| --- | --- | --- |
| [tests/support/NetworkTest.h:115](../../tests/support/NetworkTest.h#L115) | inline call | `ready = SSL_connect(ssl) == 1;` |

## SSL_do_handshake

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/TLSHandshake.cpp:143](../../src/core/socket/stream/tls/TLSHandshake.cpp#L143) | inline call | `const int ret = SSL_do_handshake(ssl);` |

## SSL_free

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketConnection.hpp:97](../../src/core/socket/stream/tls/SocketConnection.hpp#L97) | inline call | `SSL_free(ssl);` |
| [src/core/socket/stream/tls/SocketConnection.hpp:191](../../src/core/socket/stream/tls/SocketConnection.hpp#L191) | inline call | `SSL_free(ssl);` |
| [src/core/socket/stream/tls/SocketConnection.hpp:213](../../src/core/socket/stream/tls/SocketConnection.hpp#L213) | inline call | `SSL_free(tlsLifecycle->ssl);` |
| [tests/support/NetworkTest.h:88](../../tests/support/NetworkTest.h#L88) | inline call | `SSL_free(ssl);` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:78](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L78) | inline call | `SSL_free(server);` |
| [tests/unit/core/TLSTransportStateMachineTest.cpp:30](../../tests/unit/core/TLSTransportStateMachineTest.cpp#L30) | inline call | `SSL_free(peer.ssl);` |

## SSL_get_SSL_CTX

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:232](../../tests/component/net/AddressResolutionPolicyTest.cpp#L232) | inline call | `result.expectTrue(SSL_get_SSL_CTX(connection->getSSL()) == context, "fallback listener uses its shared TLS context");` |

## SSL_get_error

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketReader.cpp:100](../../src/core/socket/stream/tls/SocketReader.cpp#L100) | inline call | `const int sslErr = SSL_get_error(ssl, static_cast<int>(ret));` |
| [src/core/socket/stream/tls/SocketWriter.cpp:97](../../src/core/socket/stream/tls/SocketWriter.cpp#L97) | inline call | `const int sslErr = SSL_get_error(ssl, static_cast<int>(ret));` |
| [src/core/socket/stream/tls/TLSHandshake.cpp:149](../../src/core/socket/stream/tls/TLSHandshake.cpp#L149) | inline call | `const int sslErr = SSL_get_error(ssl, ret);` |
| [src/core/socket/stream/tls/TLSShutdown.cpp:206](../../src/core/socket/stream/tls/TLSShutdown.cpp#L206) | inline call | `const int sslErr = SSL_get_error(ssl, ret);` |
| [src/core/socket/stream/tls/TLSShutdown.cpp:229](../../src/core/socket/stream/tls/TLSShutdown.cpp#L229) | inline call | `const int sslErr = SSL_get_error(ssl, ret);` |

## SSL_get_ex_data

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketAcceptor.hpp:167](../../src/core/socket/stream/tls/SocketAcceptor.hpp#L167) | inline call | `const std::string connectionName = *static_cast<std::string*>(SSL_get_ex_data(ssl, 0));` |
| [src/core/socket/stream/tls/SocketAcceptor.hpp:168](../../src/core/socket/stream/tls/SocketAcceptor.hpp#L168) | inline call | `Config* config = static_cast<Config*>(SSL_get_ex_data(ssl, 1));` |
| [src/core/socket/stream/tls/ssl_utils.cpp:111](../../src/core/socket/stream/tls/ssl_utils.cpp#L111) | inline call | `const std::string connectionName = *static_cast<std::string*>(SSL_get_ex_data(ssl, 0));` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:233](../../tests/component/net/AddressResolutionPolicyTest.cpp#L233) | inline call | `result.expectTrue(SSL_get_ex_data(connection->getSSL(), 1) == server.getConfig(), "SNI uses the retained configuration");` |

## SSL_get_ex_data_X509_STORE_CTX_idx

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:109](../../src/core/socket/stream/tls/ssl_utils.cpp#L109) | inline call | `const SSL* ssl = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));` |

## SSL_get_fd

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/TLSHandshake.cpp:67](../../src/core/socket/stream/tls/TLSHandshake.cpp#L67) | inline call | `auto* helper = new TLSHandshake(instanceName, logScope.scope(), ssl, onSuccess, onTimeout, onStatus, timeout, {}, SSL_get_fd(ssl));` |
| [src/core/socket/stream/tls/TLSHandshake.cpp:79](../../src/core/socket/stream/tls/TLSHandshake.cpp#L79) | inline call | `auto* helper = new TLSHandshake(instanceName, logScope, ssl, onSuccess, onTimeout, onStatus, timeout, onReleased, SSL_get_fd(ssl));` |
| [src/core/socket/stream/tls/TLSShutdown.cpp:79](../../src/core/socket/stream/tls/TLSShutdown.cpp#L79) | inline call | `SSL_get_fd(ssl));` |
| [src/core/socket/stream/tls/TLSShutdown.cpp:94](../../src/core/socket/stream/tls/TLSShutdown.cpp#L94) | inline call | `instanceName, logScope, ssl, onSuccess, onTimeout, onStatus, timeout, onReleased, SSL_get_fd(ssl), onApplicationData);` |

## SSL_get_peer_certificate

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:113](../../src/apps/echo/model/clients.h#L113) | inline call | `X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());` |
| [src/apps/echo/model/servers.h:113](../../src/apps/echo/model/servers.h#L113) | inline call | `X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());` |
| [src/apps/http/httplowlevelclient.cpp:169](../../src/apps/http/httplowlevelclient.cpp#L169) | inline call | `X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());` |
| [src/apps/http/model/clients.h:699](../../src/apps/http/model/clients.h#L699) | inline call | `X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());` |
| [src/apps/http/model/servers.h:120](../../src/apps/http/model/servers.h#L120) | inline call | `X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());` |
| [src/apps/testregex.cpp:366](../../src/apps/testregex.cpp#L366) | inline call | `X509* client_cert = SSL_get_peer_certificate(socketConnection->getSSL());` |

## SSL_get_read_ahead

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketConnection.hpp:184](../../src/core/socket/stream/tls/SocketConnection.hpp#L184) | inline call | `if (SSL_set_fd(ssl, fd) == 1 && SSL_get_read_ahead(ssl) == 0) {` |

## SSL_get_shutdown

| Source | Role | Source line |
| --- | --- | --- |
| [tests/unit/core/TLSFrameworkShutdownTest.cpp:38](../../tests/unit/core/TLSFrameworkShutdownTest.cpp#L38) | inline call | `if (SSL_get_shutdown(peer.ssl) & SSL_RECEIVED_SHUTDOWN) {` |

## SSL_get_verify_result

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:115](../../src/apps/echo/model/clients.h#L115) | inline call | `const long verifyErr = SSL_get_verify_result(socketConnection->getSSL());` |
| [src/apps/echo/model/servers.h:115](../../src/apps/echo/model/servers.h#L115) | inline call | `long verifyErr = SSL_get_verify_result(socketConnection->getSSL());` |
| [src/apps/http/httplowlevelclient.cpp:171](../../src/apps/http/httplowlevelclient.cpp#L171) | inline call | `const long verifyErr = SSL_get_verify_result(socketConnection->getSSL());` |
| [src/apps/http/model/clients.h:701](../../src/apps/http/model/clients.h#L701) | inline call | `long verifyErr = SSL_get_verify_result(socketConnection->getSSL());` |
| [src/apps/http/model/servers.h:125](../../src/apps/http/model/servers.h#L125) | inline call | `long verifyErr = SSL_get_verify_result(socketConnection->getSSL());` |
| [src/apps/testregex.cpp:369](../../src/apps/testregex.cpp#L369) | inline call | `const long verifyErr = SSL_get_verify_result(socketConnection->getSSL());` |

## SSL_new

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketConnection.hpp:175](../../src/core/socket/stream/tls/SocketConnection.hpp#L175) | inline call | `ssl = SSL_new(ctx);` |
| [tests/support/NetworkTest.h:106](../../tests/support/NetworkTest.h#L106) | inline call | `ssl = SSL_new(ctx);` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:16](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L16) | inline call | `SSL* server = SSL_new(serverCtx);` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:24](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L24) | inline call | `peer.ssl = SSL_new(peer.ctx);` |

## SSL_pending

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketConnection.hpp:359](../../src/core/socket/stream/tls/SocketConnection.hpp#L359) | inline call | `if (SSL_pending(ssl) != 0) {` |
| [src/core/socket/stream/tls/TLSShutdown.cpp:224](../../src/core/socket/stream/tls/TLSShutdown.cpp#L224) | inline call | `if (SSL_pending(ssl) > 0) {` |

## SSL_read

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketReader.cpp:94](../../src/core/socket/stream/tls/SocketReader.cpp#L94) | inline call | `ret = SSL_read(ssl, chunk, static_cast<int>(chunkLen));` |
| [src/core/socket/stream/tls/TLSShutdown.cpp:217](../../src/core/socket/stream/tls/TLSShutdown.cpp#L217) | inline call | `const int ret = SSL_read(ssl, buffer.data(), static_cast<int>(buffer.size()));` |
| [src/core/socket/stream/tls/system/ssl.cpp:53](../../src/core/socket/stream/tls/system/ssl.cpp#L53) | wrapper definition | `int SSL_read(SSL* ssl, void* buf, int num) {` |
| [src/core/socket/stream/tls/system/ssl.cpp:55](../../src/core/socket/stream/tls/system/ssl.cpp#L55) | wrapper implementation call | `return ::SSL_read(ssl, buf, num);` |
| [src/core/socket/stream/tls/system/ssl.h:54](../../src/core/socket/stream/tls/system/ssl.h#L54) | wrapper declaration | `int SSL_read(SSL* ssl, void* buf, int num);` |
| [tests/support/NetworkTest.h:132](../../tests/support/NetworkTest.h#L132) | inline call | `return ssl ? SSL_read(ssl, bytes, size) : static_cast<int>(::recv(fd, bytes, static_cast<std::size_t>(size), 0));` |

## SSL_set_SSL_CTX

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:291](../../src/core/socket/stream/tls/ssl_utils.cpp#L291) | inline call | `SSL_CTX* newSslCtx = SSL_set_SSL_CTX(ssl, sslCtx);` |

## SSL_set_accept_state

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketAcceptor.hpp:74](../../src/core/socket/stream/tls/SocketAcceptor.hpp#L74) | inline call | `SSL_set_accept_state(ssl);` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:19](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L19) | inline call | `SSL_set_accept_state(server);` |

## SSL_set_connect_state

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketConnector.hpp:75](../../src/core/socket/stream/tls/SocketConnector.hpp#L75) | inline call | `SSL_set_connect_state(ssl);` |
| [tests/support/NetworkTest.h:109](../../tests/support/NetworkTest.h#L109) | inline call | `SSL_set_connect_state(ssl);` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:27](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L27) | inline call | `SSL_set_connect_state(peer.ssl);` |

## SSL_set_ex_data

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketAcceptor.hpp:75](../../src/core/socket/stream/tls/SocketAcceptor.hpp#L75) | inline call | `SSL_set_ex_data(ssl, 1, config.get());` |
| [src/core/socket/stream/tls/SocketConnection.hpp:180](../../src/core/socket/stream/tls/SocketConnection.hpp#L180) | inline call | `SSL_set_ex_data(ssl, 0, const_cast<std::string*>(&Super::getConnectionName()));` |
| [src/core/socket/stream/tls/SocketConnector.hpp:76](../../src/core/socket/stream/tls/SocketConnector.hpp#L76) | inline call | `SSL_set_ex_data(ssl, 1, config.get());` |

## SSL_set_fd

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketConnection.hpp:184](../../src/core/socket/stream/tls/SocketConnection.hpp#L184) | inline call | `if (SSL_set_fd(ssl, fd) == 1 && SSL_get_read_ahead(ssl) == 0) {` |
| [tests/support/NetworkTest.h:108](../../tests/support/NetworkTest.h#L108) | inline call | `require(SSL_set_fd(ssl, fd) == 1, "peer SSL fd");` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:18](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L18) | inline call | `SSL_set_fd(server, sockets[0]);` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:26](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L26) | inline call | `SSL_set_fd(peer.ssl, peer.fd);` |

## SSL_set_mode

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:296](../../src/core/socket/stream/tls/ssl_utils.cpp#L296) | inline call | `SSL_set_mode(ssl, SSL_CTX_get_mode(sslCtx));` |

## SSL_set_options

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:293](../../src/core/socket/stream/tls/ssl_utils.cpp#L293) | inline call | `SSL_set_options(ssl, SSL_CTX_get_options(sslCtx));` |

## SSL_set_read_ahead

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketConnection.hpp:183](../../src/core/socket/stream/tls/SocketConnection.hpp#L183) | inline call | `SSL_set_read_ahead(ssl, 0);` |

## SSL_set_tlsext_host_name

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:286](../../src/core/socket/stream/tls/ssl_utils.cpp#L286) | inline call | `SSL_set_tlsext_host_name(ssl, sni.data());` |

## SSL_set_verify

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:294](../../src/core/socket/stream/tls/ssl_utils.cpp#L294) | inline call | `SSL_set_verify(ssl, SSL_CTX_get_verify_mode(sslCtx), SSL_CTX_get_verify_callback(sslCtx));` |

## SSL_set_verify_depth

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:295](../../src/core/socket/stream/tls/ssl_utils.cpp#L295) | inline call | `SSL_set_verify_depth(ssl, SSL_CTX_get_verify_depth(sslCtx));` |

## SSL_shutdown

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/TLSShutdown.cpp:192](../../src/core/socket/stream/tls/TLSShutdown.cpp#L192) | inline call | `const int ret = SSL_shutdown(ssl);` |
| [tests/unit/core/TLSFrameworkShutdownTest.cpp:40](../../tests/unit/core/TLSFrameworkShutdownTest.cpp#L40) | inline call | `SSL_shutdown(peer.ssl);` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:69](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L69) | inline call | `SSL_shutdown(peer.ssl);` |
| [tests/unit/core/TLSTransportStateMachineTest.cpp:28](../../tests/unit/core/TLSTransportStateMachineTest.cpp#L28) | inline call | `shutdown = SSL_shutdown(peer.ssl) == 1;` |

## SSL_write

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/SocketWriter.cpp:91](../../src/core/socket/stream/tls/SocketWriter.cpp#L91) | inline call | `ret = SSL_write(ssl, chunk, static_cast<int>(chunkLen));` |
| [src/core/socket/stream/tls/system/ssl.cpp:58](../../src/core/socket/stream/tls/system/ssl.cpp#L58) | wrapper definition | `int SSL_write(SSL* ssl, const void* buf, int num) {` |
| [src/core/socket/stream/tls/system/ssl.cpp:60](../../src/core/socket/stream/tls/system/ssl.cpp#L60) | wrapper implementation call | `return ::SSL_write(ssl, buf, num);` |
| [src/core/socket/stream/tls/system/ssl.h:55](../../src/core/socket/stream/tls/system/ssl.h#L55) | wrapper declaration | `int SSL_write(SSL* ssl, const void* buf, int num);` |
| [tests/support/NetworkTest.h:128](../../tests/support/NetworkTest.h#L128) | inline call | `return ssl ? SSL_write(ssl, bytes.data(), static_cast<int>(bytes.size()))` |

## S_ISREG

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/phy/PhysicalSocket.hpp:222](../../src/net/un/phy/PhysicalSocket.hpp#L222) | macro use | `if (S_ISREG(lockStatus.st_mode) && static_cast<std::uintmax_t>(lockStatus.st_dev) == lockDevice &&` |
| [src/net/un/phy/PhysicalSocket.hpp:281](../../src/net/un/phy/PhysicalSocket.hpp#L281) | macro use | `return S_ISREG(status.st_mode) && static_cast<std::uintmax_t>(status.st_dev) == lockDevice &&` |
| [src/net/un/phy/PhysicalSocket.hpp:319](../../src/net/un/phy/PhysicalSocket.hpp#L319) | macro use | `if (lockStatResult != 0 &#124;&#124; !S_ISREG(lockStatus.st_mode)) {` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:138](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L138) | macro use | `testResult.expectTrue(inspectPath(regularPath, regularBefore) && S_ISREG(regularBefore.st_mode),` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:153](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L153) | macro use | `testResult.expectTrue(inspectPath(regularPath, regularAfter) && S_ISREG(regularAfter.st_mode),` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:261](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L261) | macro use | `testResult.expectTrue(inspectPath(unrecognizedLockPath + ".lock", unrecognizedLockBefore) && S_ISREG(unrecognizedLockBefore.st_mode),` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:275](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L275) | macro use | `testResult.expectTrue(inspectPath(unrecognizedLockPath + ".lock", unrecognizedLockAfter) && S_ISREG(unrecognizedLockAfter.st_mode) &&` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:293](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L293) | macro use | `testResult.expectTrue(inspectPath(replacementPath + ".lock", replacementLockStatus) && S_ISREG(replacementLockStatus.st_mode),` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:297](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L297) | macro use | `testResult.expectTrue(replacementCreated && inspectPath(replacementPath, replacementStatus) && S_ISREG(replacementStatus.st_mode),` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:302](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L302) | macro use | `S_ISREG(replacementLockStatus.st_mode),` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:307](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L307) | macro use | `testResult.expectTrue(replacementCreated && inspectPath(replacementPath, replacementAfter) && S_ISREG(replacementAfter.st_mode),` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:314](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L314) | macro use | `S_ISREG(replacementLockAfter.st_mode),` |

## S_ISSOCK

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/phy/PhysicalSocket.hpp:195](../../src/net/un/phy/PhysicalSocket.hpp#L195) | macro use | `if (S_ISSOCK(socketStatus.st_mode) && static_cast<std::uintmax_t>(socketStatus.st_dev) == socketDevice &&` |
| [src/net/un/phy/PhysicalSocket.hpp:390](../../src/net/un/phy/PhysicalSocket.hpp#L390) | macro use | `if (!S_ISSOCK(existingStatus.st_mode)) {` |
| [src/net/un/phy/PhysicalSocket.hpp:422](../../src/net/un/phy/PhysicalSocket.hpp#L422) | macro use | `} else if (!S_ISSOCK(unlinkStatus.st_mode) &#124;&#124; unlinkStatus.st_dev != existingStatus.st_dev &#124;&#124;` |
| [src/net/un/phy/PhysicalSocket.hpp:470](../../src/net/un/phy/PhysicalSocket.hpp#L470) | macro use | `if (boundStatResult != 0 &#124;&#124; !S_ISSOCK(boundStatus.st_mode)) {` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:163](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L163) | macro use | `testResult.expectTrue(inspectPath(activePath, activeBefore) && S_ISSOCK(activeBefore.st_mode),` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:176](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L176) | macro use | `testResult.expectTrue(inspectPath(activePath, activeAfter) && S_ISSOCK(activeAfter.st_mode) &&` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:187](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L187) | macro use | `testResult.expectTrue(inspectPath(unmarkedStalePath, unmarkedStaleBefore) && S_ISSOCK(unmarkedStaleBefore.st_mode),` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:202](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L202) | macro use | `testResult.expectTrue(inspectPath(unmarkedStalePath, unmarkedStaleAfter) && S_ISSOCK(unmarkedStaleAfter.st_mode) &&` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:213](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L213) | macro use | `testResult.expectTrue(inspectPath(ownedStalePath, ownedStaleBefore) && S_ISSOCK(ownedStaleBefore.st_mode),` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:223](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L223) | macro use | `S_ISSOCK(recoveredStatus.st_mode),` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:234](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L234) | macro use | `testResult.expectTrue(inspectPath(boundBeforeListenPath, boundBeforeListenBefore) && S_ISSOCK(boundBeforeListenBefore.st_mode),` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:248](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L248) | macro use | `testResult.expectTrue(inspectPath(boundBeforeListenPath, boundBeforeListenAfter) && S_ISSOCK(boundBeforeListenAfter.st_mode) &&` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:291](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L291) | macro use | `testResult.expectTrue(inspectPath(replacementPath, ownedSocketStatus) && S_ISSOCK(ownedSocketStatus.st_mode),` |

## TLS_client_method

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:150](../../src/core/socket/stream/tls/ssl_utils.cpp#L150) | inline call | `SSL_CTX* ctx = SSL_CTX_new(sslConfig.server ? TLS_server_method() : TLS_client_method());` |
| [tests/support/NetworkTest.h:103](../../tests/support/NetworkTest.h#L103) | inline call | `ctx = SSL_CTX_new(TLS_client_method());` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:22](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L22) | inline call | `peer.ctx = SSL_CTX_new(TLS_client_method());` |

## TLS_server_method

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:150](../../src/core/socket/stream/tls/ssl_utils.cpp#L150) | inline call | `SSL_CTX* ctx = SSL_CTX_new(sslConfig.server ? TLS_server_method() : TLS_client_method());` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:14](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L14) | inline call | `SSL_CTX* serverCtx = SSL_CTX_new(TLS_server_method());` |

## WEXITSTATUS

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:219](../../src/tools/snodec-control/src/ProcessRunner.cpp#L219) | macro use | `result.exitCode = WEXITSTATUS(status);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:247](../../src/tools/snodec-control/src/ProcessRunner.cpp#L247) | macro use | `result.exitCode = WEXITSTATUS(status);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:329](../../src/tools/snodec-control/src/ProcessRunner.cpp#L329) | macro use | `result.exitCode = WEXITSTATUS(status);` |
| [tests/component/core/ShutdownReceiverNotificationTest.cpp:120](../../tests/component/core/ShutdownReceiverNotificationTest.cpp#L120) | macro use | `testResult.expectTrue(WIFEXITED(signalerStatus) && WEXITSTATUS(signalerStatus) == 0, "SIGINT sender process succeeds");` |

## WIFEXITED

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:218](../../src/tools/snodec-control/src/ProcessRunner.cpp#L218) | macro use | `if (WIFEXITED(status)) {` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:246](../../src/tools/snodec-control/src/ProcessRunner.cpp#L246) | macro use | `if (WIFEXITED(status)) {` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:328](../../src/tools/snodec-control/src/ProcessRunner.cpp#L328) | macro use | `if (WIFEXITED(status)) {` |
| [tests/component/core/ShutdownReceiverNotificationTest.cpp:120](../../tests/component/core/ShutdownReceiverNotificationTest.cpp#L120) | macro use | `testResult.expectTrue(WIFEXITED(signalerStatus) && WEXITSTATUS(signalerStatus) == 0, "SIGINT sender process succeeds");` |

## WIFSIGNALED

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:220](../../src/tools/snodec-control/src/ProcessRunner.cpp#L220) | macro use | `} else if (WIFSIGNALED(status)) {` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:248](../../src/tools/snodec-control/src/ProcessRunner.cpp#L248) | macro use | `} else if (WIFSIGNALED(status)) {` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:330](../../src/tools/snodec-control/src/ProcessRunner.cpp#L330) | macro use | `} else if (WIFSIGNALED(status)) {` |

## WTERMSIG

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:222](../../src/tools/snodec-control/src/ProcessRunner.cpp#L222) | macro use | `result.signalNumber = WTERMSIG(status);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:250](../../src/tools/snodec-control/src/ProcessRunner.cpp#L250) | macro use | `result.signalNumber = WTERMSIG(status);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:332](../../src/tools/snodec-control/src/ProcessRunner.cpp#L332) | macro use | `result.signalNumber = WTERMSIG(status);` |

## X509_NAME_add_entry_by_txt

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:220](../../tests/component/net/AddressResolutionPolicyTest.cpp#L220) | inline call | `X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("localhost"), -1, -1, 0);` |
| [tests/support/NetworkTest.h:73](../../tests/support/NetworkTest.h#L73) | inline call | `X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("localhost"), -1, -1, 0);` |

## X509_NAME_oneline

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:120](../../src/apps/echo/model/clients.h#L120) | inline call | `char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);` |
| [src/apps/echo/model/clients.h:126](../../src/apps/echo/model/clients.h#L126) | inline call | `str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);` |
| [src/apps/echo/model/servers.h:120](../../src/apps/echo/model/servers.h#L120) | inline call | `char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);` |
| [src/apps/echo/model/servers.h:126](../../src/apps/echo/model/servers.h#L126) | inline call | `str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);` |
| [src/apps/http/httplowlevelclient.cpp:176](../../src/apps/http/httplowlevelclient.cpp#L176) | inline call | `char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);` |
| [src/apps/http/httplowlevelclient.cpp:182](../../src/apps/http/httplowlevelclient.cpp#L182) | inline call | `str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);` |
| [src/apps/http/model/clients.h:706](../../src/apps/http/model/clients.h#L706) | inline call | `char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);` |
| [src/apps/http/model/clients.h:712](../../src/apps/http/model/clients.h#L712) | inline call | `str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);` |
| [src/apps/http/model/servers.h:129](../../src/apps/http/model/servers.h#L129) | inline call | `char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);` |
| [src/apps/http/model/servers.h:135](../../src/apps/http/model/servers.h#L135) | inline call | `str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);` |
| [src/apps/testregex.cpp:373](../../src/apps/testregex.cpp#L373) | inline call | `char* str = X509_NAME_oneline(X509_get_subject_name(client_cert), nullptr, 0);` |
| [src/apps/testregex.cpp:379](../../src/apps/testregex.cpp#L379) | inline call | `str = X509_NAME_oneline(X509_get_issuer_name(client_cert), nullptr, 0);` |
| [src/core/socket/stream/tls/ssl_utils.cpp:117](../../src/core/socket/stream/tls/ssl_utils.cpp#L117) | inline call | `X509_NAME_oneline(X509_get_subject_name(curr_cert), subjectName, 256);` |
| [src/core/socket/stream/tls/ssl_utils.cpp:120](../../src/core/socket/stream/tls/ssl_utils.cpp#L120) | inline call | `X509_NAME_oneline(X509_get_issuer_name(curr_cert), issuerName, 256);` |

## X509_STORE_CTX_get_current_cert

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:113](../../src/core/socket/stream/tls/ssl_utils.cpp#L113) | inline call | `const X509* curr_cert = X509_STORE_CTX_get_current_cert(ctx);` |

## X509_STORE_CTX_get_error

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:127](../../src/core/socket/stream/tls/ssl_utils.cpp#L127) | inline call | `const int err = X509_STORE_CTX_get_error(ctx);` |

## X509_STORE_CTX_get_error_depth

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:114](../../src/core/socket/stream/tls/ssl_utils.cpp#L114) | inline call | `const int depth = X509_STORE_CTX_get_error_depth(ctx);` |

## X509_STORE_CTX_get_ex_data

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:109](../../src/core/socket/stream/tls/ssl_utils.cpp#L109) | inline call | `const SSL* ssl = static_cast<SSL*>(X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));` |

## X509_free

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:159](../../src/apps/echo/model/clients.h#L159) | inline call | `X509_free(server_cert);` |
| [src/apps/echo/model/servers.h:159](../../src/apps/echo/model/servers.h#L159) | inline call | `X509_free(server_cert);` |
| [src/apps/http/httplowlevelclient.cpp:215](../../src/apps/http/httplowlevelclient.cpp#L215) | inline call | `X509_free(server_cert);` |
| [src/apps/http/model/clients.h:745](../../src/apps/http/model/clients.h#L745) | inline call | `X509_free(server_cert);` |
| [src/apps/http/model/servers.h:170](../../src/apps/http/model/servers.h#L170) | inline call | `X509_free(server_cert);` |
| [src/apps/testregex.cpp:412](../../src/apps/testregex.cpp#L412) | inline call | `X509_free(client_cert);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:225](../../tests/component/net/AddressResolutionPolicyTest.cpp#L225) | inline call | `X509_free(certificate);` |
| [tests/support/NetworkTest.h:78](../../tests/support/NetworkTest.h#L78) | inline call | `X509_free(cert);` |

## X509_get_ext_d2i

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:135](../../src/apps/echo/model/clients.h#L135) | inline call | `static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));` |
| [src/apps/echo/model/servers.h:135](../../src/apps/echo/model/servers.h#L135) | inline call | `static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));` |
| [src/apps/http/httplowlevelclient.cpp:191](../../src/apps/http/httplowlevelclient.cpp#L191) | inline call | `static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));` |
| [src/apps/http/model/clients.h:721](../../src/apps/http/model/clients.h#L721) | inline call | `static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));` |
| [src/apps/http/model/servers.h:144](../../src/apps/http/model/servers.h#L144) | inline call | `static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));` |
| [src/apps/testregex.cpp:388](../../src/apps/testregex.cpp#L388) | inline call | `static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(client_cert, NID_subject_alt_name, nullptr, nullptr));` |
| [src/core/socket/stream/tls/ssl_utils.cpp:263](../../src/core/socket/stream/tls/ssl_utils.cpp#L263) | inline call | `static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(x509, NID_subject_alt_name, nullptr, nullptr));` |

## X509_get_issuer_name

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:126](../../src/apps/echo/model/clients.h#L126) | inline call | `str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);` |
| [src/apps/echo/model/servers.h:126](../../src/apps/echo/model/servers.h#L126) | inline call | `str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);` |
| [src/apps/http/httplowlevelclient.cpp:182](../../src/apps/http/httplowlevelclient.cpp#L182) | inline call | `str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);` |
| [src/apps/http/model/clients.h:712](../../src/apps/http/model/clients.h#L712) | inline call | `str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);` |
| [src/apps/http/model/servers.h:135](../../src/apps/http/model/servers.h#L135) | inline call | `str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);` |
| [src/apps/testregex.cpp:379](../../src/apps/testregex.cpp#L379) | inline call | `str = X509_NAME_oneline(X509_get_issuer_name(client_cert), nullptr, 0);` |
| [src/core/socket/stream/tls/ssl_utils.cpp:120](../../src/core/socket/stream/tls/ssl_utils.cpp#L120) | inline call | `X509_NAME_oneline(X509_get_issuer_name(curr_cert), issuerName, 256);` |

## X509_get_notAfter

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:217](../../tests/component/net/AddressResolutionPolicyTest.cpp#L217) | inline call | `X509_gmtime_adj(X509_get_notAfter(certificate), 3600);` |
| [tests/support/NetworkTest.h:70](../../tests/support/NetworkTest.h#L70) | inline call | `X509_gmtime_adj(X509_get_notAfter(cert), 3600);` |

## X509_get_notBefore

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:216](../../tests/component/net/AddressResolutionPolicyTest.cpp#L216) | inline call | `X509_gmtime_adj(X509_get_notBefore(certificate), -60);` |
| [tests/support/NetworkTest.h:69](../../tests/support/NetworkTest.h#L69) | inline call | `X509_gmtime_adj(X509_get_notBefore(cert), -60);` |

## X509_get_serialNumber

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:215](../../tests/component/net/AddressResolutionPolicyTest.cpp#L215) | inline call | `ASN1_INTEGER_set(X509_get_serialNumber(certificate), 1);` |
| [tests/support/NetworkTest.h:68](../../tests/support/NetworkTest.h#L68) | inline call | `ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);` |

## X509_get_subject_name

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:120](../../src/apps/echo/model/clients.h#L120) | inline call | `char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);` |
| [src/apps/echo/model/servers.h:120](../../src/apps/echo/model/servers.h#L120) | inline call | `char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);` |
| [src/apps/http/httplowlevelclient.cpp:176](../../src/apps/http/httplowlevelclient.cpp#L176) | inline call | `char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);` |
| [src/apps/http/model/clients.h:706](../../src/apps/http/model/clients.h#L706) | inline call | `char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);` |
| [src/apps/http/model/servers.h:129](../../src/apps/http/model/servers.h#L129) | inline call | `char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);` |
| [src/apps/testregex.cpp:373](../../src/apps/testregex.cpp#L373) | inline call | `char* str = X509_NAME_oneline(X509_get_subject_name(client_cert), nullptr, 0);` |
| [src/core/socket/stream/tls/ssl_utils.cpp:117](../../src/core/socket/stream/tls/ssl_utils.cpp#L117) | inline call | `X509_NAME_oneline(X509_get_subject_name(curr_cert), subjectName, 256);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:219](../../tests/component/net/AddressResolutionPolicyTest.cpp#L219) | inline call | `X509_NAME* name = X509_get_subject_name(certificate);` |
| [tests/support/NetworkTest.h:72](../../tests/support/NetworkTest.h#L72) | inline call | `X509_NAME* name = X509_get_subject_name(cert);` |

## X509_gmtime_adj

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:216](../../tests/component/net/AddressResolutionPolicyTest.cpp#L216) | inline call | `X509_gmtime_adj(X509_get_notBefore(certificate), -60);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:217](../../tests/component/net/AddressResolutionPolicyTest.cpp#L217) | inline call | `X509_gmtime_adj(X509_get_notAfter(certificate), 3600);` |
| [tests/support/NetworkTest.h:69](../../tests/support/NetworkTest.h#L69) | inline call | `X509_gmtime_adj(X509_get_notBefore(cert), -60);` |
| [tests/support/NetworkTest.h:70](../../tests/support/NetworkTest.h#L70) | inline call | `X509_gmtime_adj(X509_get_notAfter(cert), 3600);` |

## X509_new

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:211](../../tests/component/net/AddressResolutionPolicyTest.cpp#L211) | inline call | `X509* certificate = X509_new();` |
| [tests/support/NetworkTest.h:65](../../tests/support/NetworkTest.h#L65) | inline call | `X509* cert = X509_new();` |

## X509_set_issuer_name

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:221](../../tests/component/net/AddressResolutionPolicyTest.cpp#L221) | inline call | `X509_set_issuer_name(certificate, name);` |
| [tests/support/NetworkTest.h:74](../../tests/support/NetworkTest.h#L74) | inline call | `X509_set_issuer_name(cert, name);` |

## X509_set_pubkey

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:218](../../tests/component/net/AddressResolutionPolicyTest.cpp#L218) | inline call | `X509_set_pubkey(certificate, key);` |
| [tests/support/NetworkTest.h:71](../../tests/support/NetworkTest.h#L71) | inline call | `X509_set_pubkey(cert, key);` |

## X509_set_version

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:214](../../tests/component/net/AddressResolutionPolicyTest.cpp#L214) | inline call | `X509_set_version(certificate, 2);` |
| [tests/support/NetworkTest.h:67](../../tests/support/NetworkTest.h#L67) | inline call | `X509_set_version(cert, 2);` |

## X509_sign

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:222](../../tests/component/net/AddressResolutionPolicyTest.cpp#L222) | inline call | `ready = X509_sign(certificate, key, EVP_sha256()) > 0 && SSL_CTX_use_certificate(context, certificate) == 1 &&` |
| [tests/support/NetworkTest.h:75](../../tests/support/NetworkTest.h#L75) | inline call | `ok = X509_sign(cert, key, EVP_sha256()) > 0 && SSL_CTX_use_certificate(ctx, cert) == 1 && SSL_CTX_use_PrivateKey(ctx, key) == 1;` |

## X509_verify_cert_error_string

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:118](../../src/apps/echo/model/clients.h#L118) | inline call | `std::string(X509_verify_cert_error_string(verifyErr));` |
| [src/apps/echo/model/servers.h:118](../../src/apps/echo/model/servers.h#L118) | inline call | `std::string(X509_verify_cert_error_string(verifyErr));` |
| [src/apps/http/httplowlevelclient.cpp:174](../../src/apps/http/httplowlevelclient.cpp#L174) | inline call | `<< "     Server certificate: " + std::string(X509_verify_cert_error_string(verifyErr));` |
| [src/apps/http/model/clients.h:704](../../src/apps/http/model/clients.h#L704) | inline call | `std::string(X509_verify_cert_error_string(verifyErr));` |
| [src/apps/http/model/servers.h:127](../../src/apps/http/model/servers.h#L127) | inline call | `log.debug() << "\tPeer certificate verifyErr = " << verifyErr << ": " << X509_verify_cert_error_string(verifyErr);` |
| [src/apps/testregex.cpp:371](../../src/apps/testregex.cpp#L371) | inline call | `snode::log::application().debug() << "\tClient certificate: " + std::string(X509_verify_cert_error_string(verifyErr));` |
| [src/core/socket/stream/tls/ssl_utils.cpp:129](../../src/core/socket/stream/tls/ssl_utils.cpp#L129) | inline call | `tlsLog().debug("{}: SSL/TLS verify error at depth={}: {}", connectionName, depth, X509_verify_cert_error_string(err));` |

## _dupenv_s

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:1310](../../src/utils/CLI11.hpp#L1310) | inline call | `if(_dupenv_s(&buffer, &sz, env_name.c_str()) == 0 && buffer != nullptr) {` |

## _exit

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/core/ShutdownReceiverNotificationTest.cpp:99](../../tests/component/core/ShutdownReceiverNotificationTest.cpp#L99) | inline call | `::_exit(killResult == 0 ? 0 : 1);` |

## _stat64

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:4319](../../src/utils/CLI11.hpp#L4319) | inline call | `if(_stat64(file, &buffer) == 0) {` |

## abort

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:93](../../tests/component/net/AddressResolutionPolicyTest.cpp#L93) | inline call | `std::abort();` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:98](../../tests/component/net/AddressResolutionPolicyTest.cpp#L98) | inline call | `std::abort();` |
| [tests/unit/log/SemanticLoggerBackendTest.cpp:145](../../tests/unit/log/SemanticLoggerBackendTest.cpp#L145) | inline call | `std::abort();` |
| [tests/unit/log/SemanticLoggerDisabledPathTest.cpp:142](../../tests/unit/log/SemanticLoggerDisabledPathTest.cpp#L142) | inline call | `std::abort();` |
| [tests/unit/log/SemanticLoggerScopeTest.cpp:93](../../tests/unit/log/SemanticLoggerScopeTest.cpp#L93) | inline call | `std::abort();` |
| [tests/unit/utils/ConfigFormatterCommentMetadataTest.cpp:13](../../tests/unit/utils/ConfigFormatterCommentMetadataTest.cpp#L13) | inline call | `std::abort();` |
| [tests/unit/utils/ConfigFormatterCommentMetadataTest.cpp:20](../../tests/unit/utils/ConfigFormatterCommentMetadataTest.cpp#L20) | inline call | `std::abort();` |
| [tests/unit/utils/ConfigFormatterCommentMetadataTest.cpp:29](../../tests/unit/utils/ConfigFormatterCommentMetadataTest.cpp#L29) | inline call | `std::abort();` |
| [tests/unit/utils/ConfigRequirementStateTest.cpp:16](../../tests/unit/utils/ConfigRequirementStateTest.cpp#L16) | inline call | `std::abort();` |

## accept

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/socket.cpp:67](../../src/core/system/socket.cpp#L67) | wrapper definition | `int accept(int sockfd, sockaddr* addr, socklen_t* addrlen) {` |
| [src/core/system/socket.cpp:69](../../src/core/system/socket.cpp#L69) | wrapper implementation call | `return ::accept(sockfd, addr, addrlen);` |
| [src/core/system/socket.h:69](../../src/core/system/socket.h#L69) | wrapper declaration | `int accept(int sockfd, sockaddr* addr, socklen_t* addrlen);` |
| [src/net/phy/stream/PhysicalSocketServer.hpp:59](../../src/net/phy/stream/PhysicalSocketServer.hpp#L59) | extracted caller | `return core::system::accept(Super::getFd(), nullptr, nullptr);` |

## accept4

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/socket.cpp:72](../../src/core/system/socket.cpp#L72) | wrapper definition | `int accept4(int sockfd, sockaddr* addr, socklen_t* addrlen, int flags) {` |
| [src/core/system/socket.cpp:74](../../src/core/system/socket.cpp#L74) | wrapper implementation call | `return ::accept4(sockfd, addr, addrlen, flags);` |
| [src/core/system/socket.h:70](../../src/core/system/socket.h#L70) | wrapper declaration | `int accept4(int sockfd, sockaddr* addr, socklen_t* addrlen, int flags);` |
| [src/net/phy/stream/PhysicalSocketServer.hpp:64](../../src/net/phy/stream/PhysicalSocketServer.hpp#L64) | extracted caller | `return core::system::accept4(Super::getFd(), nullptr, nullptr, flags);` |

## access

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/Cli.cpp:315](../../src/tools/snodec-control/src/Cli.cpp#L315) | inline call | `if (::access(targetPath.c_str(), X_OK) != 0) {` |
| [tests/component/net/UnixLegacyClientConnectFailureTest.cpp:76](../../tests/component/net/UnixLegacyClientConnectFailureTest.cpp#L76) | inline call | `return access(socketPath.c_str(), F_OK) == 0;` |
| [tests/component/net/UnixLegacyServerClientCompositionTest.cpp:79](../../tests/component/net/UnixLegacyServerClientCompositionTest.cpp#L79) | inline call | `return access(socketPath.c_str(), F_OK) == 0;` |
| [tests/component/net/UnixLegacyServerClientDisconnectLifecycleTest.cpp:89](../../tests/component/net/UnixLegacyServerClientDisconnectLifecycleTest.cpp#L89) | inline call | `return access(socketPath.c_str(), F_OK) == 0;` |
| [tests/component/net/UnixLegacyServerClientFramedPayloadExchangeTest.cpp:295](../../tests/component/net/UnixLegacyServerClientFramedPayloadExchangeTest.cpp#L295) | inline call | `return ::access(socketPath.c_str(), F_OK) == 0;` |
| [tests/component/net/UnixLegacyServerClientLargePayloadExchangeTest.cpp:87](../../tests/component/net/UnixLegacyServerClientLargePayloadExchangeTest.cpp#L87) | inline call | `return access(socketPath.c_str(), F_OK) == 0;` |
| [tests/component/net/UnixLegacyServerClientMultiMessagePayloadExchangeTest.cpp:91](../../tests/component/net/UnixLegacyServerClientMultiMessagePayloadExchangeTest.cpp#L91) | inline call | `return access(socketPath.c_str(), F_OK) == 0;` |
| [tests/component/net/UnixLegacyServerClientPayloadExchangeTest.cpp:86](../../tests/component/net/UnixLegacyServerClientPayloadExchangeTest.cpp#L86) | inline call | `return access(socketPath.c_str(), F_OK) == 0;` |
| [tests/component/net/UnixLegacyServerControlledCloseLifecycleTest.cpp:72](../../tests/component/net/UnixLegacyServerControlledCloseLifecycleTest.cpp#L72) | inline call | `return access(socketPath.c_str(), F_OK) == 0;` |
| [tests/component/net/UnixLegacyServerMultipleClientsCompositionTest.cpp:79](../../tests/component/net/UnixLegacyServerMultipleClientsCompositionTest.cpp#L79) | inline call | `return access(socketPath.c_str(), F_OK) == 0;` |
| [tests/component/net/UnixLegacyServerMultipleClientsDisconnectLifecycleTest.cpp:73](../../tests/component/net/UnixLegacyServerMultipleClientsDisconnectLifecycleTest.cpp#L73) | inline call | `return access(socketPath.c_str(), F_OK) == 0;` |
| [tests/component/net/UnixLegacyServerMultipleClientsPayloadExchangeTest.cpp:92](../../tests/component/net/UnixLegacyServerMultipleClientsPayloadExchangeTest.cpp#L92) | inline call | `return access(socketPath.c_str(), F_OK) == 0;` |

## atexit

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/MariaDBLibrary.cpp:68](../../src/database/mariadb/MariaDBLibrary.cpp#L68) | inline call | `std::atexit([]() {` |

## atoi

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/database/testmariadb.cpp:367](../../src/apps/database/testmariadb.cpp#L367) | inline call | `if (std::atoi(row[0]) != j + 1) { // NOLINT` |
| [src/apps/database/testmariadb.cpp:369](../../src/apps/database/testmariadb.cpp#L369) | inline call | `<< "Wrong number of rows 16: " << std::atoi(row[0]) << " != " << j + 1; // NOLINT` |

## attroff

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:186](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L186) | inline call | `attroff(A_REVERSE);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:201](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L201) | inline call | `attroff(A_REVERSE);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:559](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L559) | inline call | `attroff(A_REVERSE);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:595](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L595) | inline call | `attroff(A_BOLD);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:598](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L598) | inline call | `attroff(A_REVERSE);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:618](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L618) | inline call | `attroff(A_REVERSE);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:753](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L753) | inline call | `attroff(A_REVERSE);` |

## attron

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:184](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L184) | inline call | `attron(A_REVERSE);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:199](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L199) | inline call | `attron(A_REVERSE);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:550](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L550) | inline call | `attron(A_REVERSE);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:588](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L588) | inline call | `attron(A_REVERSE);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:591](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L591) | inline call | `attron(A_BOLD);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:612](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L612) | inline call | `attron(A_REVERSE);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:748](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L748) | inline call | `attron(A_REVERSE);` |

## ba2str

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/l2/SocketAddress.cpp:77](../../src/net/l2/SocketAddress.cpp#L77) | inline call | `ba2str(&sockAddr.l2_bdaddr, btAddressC);` |
| [src/net/rc/SocketAddress.cpp:77](../../src/net/rc/SocketAddress.cpp#L77) | inline call | `ba2str(&sockAddr.rc_bdaddr, btAddressC);` |

## be16toh

| Source | Role | Source line |
| --- | --- | --- |
| [src/iot/mqtt/types/UInt16.cpp:65](../../src/iot/mqtt/types/UInt16.cpp#L65) | inline call | `return be16toh(*reinterpret_cast<uint16_t*>(const_cast<char*>(value.data())));` |
| [src/web/websocket/Receiver.cpp:264](../../src/web/websocket/Receiver.cpp#L264) | inline call | `payloadNumBytes = payloadNumBytesLeft = be16toh(eLength2.asValue);` |

## be32toh

| Source | Role | Source line |
| --- | --- | --- |
| [src/iot/mqtt/types/UInt32.cpp:65](../../src/iot/mqtt/types/UInt32.cpp#L65) | inline call | `return be32toh(*reinterpret_cast<uint32_t*>(const_cast<char*>(value.data())));` |

## be64toh

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/websocket/Receiver.cpp:282](../../src/web/websocket/Receiver.cpp#L282) | inline call | `payloadNumBytes = payloadNumBytesLeft = be64toh(eLength8.asValue);` |

## bind

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/socket.cpp:57](../../src/core/system/socket.cpp#L57) | wrapper definition | `int bind(int sockfd, const sockaddr* addr, socklen_t addrlen) {` |
| [src/core/system/socket.cpp:59](../../src/core/system/socket.cpp#L59) | wrapper implementation call | `return ::bind(sockfd, addr, addrlen);` |
| [src/core/system/socket.h:67](../../src/core/system/socket.h#L67) | wrapper declaration | `int bind(int sockfd, const sockaddr* addr, socklen_t addrlen);` |
| [src/net/phy/PhysicalSocket.hpp:107](../../src/net/phy/PhysicalSocket.hpp#L107) | extracted caller | `int ret = core::system::bind(core::Descriptor::getFd(), &configuredAddress.getSockAddr(), configuredAddress.getSockAddrLen());` |
| [tests/component/http/InetHttpClientConnectFailureTest.cpp:24](../../tests/component/http/InetHttpClientConnectFailureTest.cpp#L24) | inline call | `if (fd < 0 &#124;&#124; ::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:193](../../tests/component/net/AddressResolutionPolicyTest.cpp#L193) | inline call | `result.expectEqual(0, bind(blocker, reinterpret_cast<sockaddr*>(&occupied), sizeof(occupied)), "first address is occupied");` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:265](../../tests/component/net/AddressResolutionPolicyTest.cpp#L265) | inline call | `result.expectEqual(0, bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)), "raw fixture binds");` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:77](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L77) | inline call | `const bool bound = ::bind(fd, &address.getSockAddr(), address.getSockAddrLen()) == 0;` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:91](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L91) | inline call | `if (::bind(fd, &address.getSockAddr(), address.getSockAddrLen()) != 0 &#124;&#124; (listen && ::listen(fd, 4) != 0)) {` |
| [tests/unit/core/ConnectionAttemptLifecycleTest.cpp:15](../../tests/unit/core/ConnectionAttemptLifecycleTest.cpp#L15) | inline call | `require(bind(reservation, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0, "bind reserved port");` |

## btohs

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/l2/SocketAddress.cpp:74](../../src/net/l2/SocketAddress.cpp#L74) | inline call | `psm = btohs(sockAddr.l2_psm);` |

## cbreak

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:87](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L87) | inline call | `cbreak();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:478](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L478) | inline call | `cbreak();` |

## chdir

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Daemon.cpp:144](../../src/utils/Daemon.cpp#L144) | inline call | `if (chdir("/") != 0) {` |
| [tests/component/core/FileReaderTest.cpp:129](../../tests/component/core/FileReaderTest.cpp#L129) | inline call | `result.expectTrue(savedWorkingDirectory >= 0 && ::chdir(temporaryDirectory.c_str()) == 0,` |

## chown

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Config.cpp:1003](../../src/utils/Config.cpp#L1003) | inline call | `if (chown(configDirectory.c_str(), euid, gr->gr_gid) < 0) {` |
| [src/utils/Config.cpp:1025](../../src/utils/Config.cpp#L1025) | inline call | `if (chown(logDirectory.c_str(), euid, gr->gr_gid) < 0) {` |
| [src/utils/Config.cpp:1047](../../src/utils/Config.cpp#L1047) | inline call | `if (chown(pidDirectory.c_str(), euid, gr->gr_gid) < 0) {` |

## close

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/Descriptor.cpp:76](../../src/core/Descriptor.cpp#L76) | extracted caller | `core::system::close(fd);` |
| [src/core/file/FileReader.cpp:75](../../src/core/file/FileReader.cpp#L75) | extracted caller | `core::system::close(fd);` |
| [src/core/pipe/Pipe.cpp:73](../../src/core/pipe/Pipe.cpp#L73) | extracted caller | `core::system::close(descriptor);` |
| [src/core/pipe/PipeSink.cpp:167](../../src/core/pipe/PipeSink.cpp#L167) | extracted caller | `core::system::close(getRegisteredFd());` |
| [src/core/pipe/PipeSource.cpp:201](../../src/core/pipe/PipeSource.cpp#L201) | extracted caller | `core::system::close(getRegisteredFd());` |
| [src/core/system/unistd.cpp:72](../../src/core/system/unistd.cpp#L72) | wrapper definition | `int close(int fd) {` |
| [src/core/system/unistd.cpp:74](../../src/core/system/unistd.cpp#L74) | wrapper implementation call | `return ::close(fd);` |
| [src/core/system/unistd.h:69](../../src/core/system/unistd.h#L69) | wrapper declaration | `int close(int fd);` |
| [src/net/un/phy/PhysicalSocket.hpp:119](../../src/net/un/phy/PhysicalSocket.hpp#L119) | extracted caller | `core::system::close(probeFd);` |
| [src/net/un/phy/PhysicalSocket.hpp:248](../../src/net/un/phy/PhysicalSocket.hpp#L248) | extracted caller | `core::system::close(lockFd);` |
| [src/net/un/phy/PhysicalSocket.hpp:322](../../src/net/un/phy/PhysicalSocket.hpp#L322) | extracted caller | `core::system::close(lockFd);` |
| [src/net/un/phy/PhysicalSocket.hpp:338](../../src/net/un/phy/PhysicalSocket.hpp#L338) | extracted caller | `core::system::close(lockFd);` |
| [src/tools/snodec-control/src/ConfigActions.cpp:286](../../src/tools/snodec-control/src/ConfigActions.cpp#L286) | inline call | `::close(fd);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:116](../../src/tools/snodec-control/src/ProcessRunner.cpp#L116) | inline call | `::close(outPipe.readEnd);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:127](../../src/tools/snodec-control/src/ProcessRunner.cpp#L127) | inline call | `::close(errPipe.readEnd);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:202](../../src/tools/snodec-control/src/ProcessRunner.cpp#L202) | inline call | `::close(outPipe.writeEnd);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:203](../../src/tools/snodec-control/src/ProcessRunner.cpp#L203) | inline call | `::close(errPipe.writeEnd);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:206](../../src/tools/snodec-control/src/ProcessRunner.cpp#L206) | inline call | `::close(outPipe.readEnd);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:207](../../src/tools/snodec-control/src/ProcessRunner.cpp#L207) | inline call | `::close(errPipe.readEnd);` |
| [src/utils/Daemon.cpp:148](../../src/utils/Daemon.cpp#L148) | inline call | `close(STDIN_FILENO);` |
| [src/utils/Daemon.cpp:149](../../src/utils/Daemon.cpp#L149) | inline call | `close(STDOUT_FILENO);` |
| [src/utils/Daemon.cpp:150](../../src/utils/Daemon.cpp#L150) | inline call | `close(STDERR_FILENO);` |
| [src/utils/Daemon.cpp:188](../../src/utils/Daemon.cpp#L188) | inline call | `close(pidfd);` |
| [src/web/websocket/client/SocketContextUpgradeFactory.cpp:84](../../src/web/websocket/client/SocketContextUpgradeFactory.cpp#L84) | inline call | `close(fd);` |
| [tests/component/core/DescriptorRegistrationFailureTest.cpp:145](../../tests/component/core/DescriptorRegistrationFailureTest.cpp#L145) | inline call | `::close(closedFd);` |
| [tests/component/core/FileReaderTest.cpp:113](../../tests/component/core/FileReaderTest.cpp#L113) | inline call | `::close(writeFd);` |
| [tests/component/core/FileReaderTest.cpp:119](../../tests/component/core/FileReaderTest.cpp#L119) | inline call | `::close(writeFd);` |
| [tests/component/core/FileReaderTest.cpp:134](../../tests/component/core/FileReaderTest.cpp#L134) | inline call | `::close(savedWorkingDirectory);` |
| [tests/component/core/FileReaderTest.cpp:263](../../tests/component/core/FileReaderTest.cpp#L263) | inline call | `::close(directoryFd);` |
| [tests/component/core/PipeOwnershipTest.cpp:83](../../tests/component/core/PipeOwnershipTest.cpp#L83) | inline call | `::close(releasedReadFd);` |
| [tests/component/core/PipeOwnershipTest.cpp:100](../../tests/component/core/PipeOwnershipTest.cpp#L100) | inline call | `::close(releasedWriteFd);` |
| [tests/component/core/PipeOwnershipTest.cpp:104](../../tests/component/core/PipeOwnershipTest.cpp#L104) | inline call | `::close(invalidatedReadFd);` |
| [tests/component/core/PipeSinkFairnessTest.cpp:50](../../tests/component/core/PipeSinkFairnessTest.cpp#L50) | inline call | `::close(drainWriteFd);` |
| [tests/component/core/PipeSinkFairnessTest.cpp:51](../../tests/component/core/PipeSinkFairnessTest.cpp#L51) | inline call | `::close(limitedWriteFd);` |
| [tests/component/core/PipeSinkFairnessTest.cpp:70](../../tests/component/core/PipeSinkFairnessTest.cpp#L70) | inline call | `::close(drainWriteFd);` |
| [tests/component/core/PipeSinkFairnessTest.cpp:99](../../tests/component/core/PipeSinkFairnessTest.cpp#L99) | inline call | `::close(limitedWriteFd);` |
| [tests/component/core/ShutdownReceiverNotificationTest.cpp:45](../../tests/component/core/ShutdownReceiverNotificationTest.cpp#L45) | extracted caller | `core::system::close(getRegisteredFd());` |
| [tests/component/core/ShutdownReceiverNotificationTest.cpp:46](../../tests/component/core/ShutdownReceiverNotificationTest.cpp#L46) | extracted caller | `core::system::close(writeFd);` |
| [tests/component/http/InetHttpClientConnectFailureTest.cpp:26](../../tests/component/http/InetHttpClientConnectFailureTest.cpp#L26) | inline call | `::close(fd);` |
| [tests/component/http/InetHttpClientConnectFailureTest.cpp:31](../../tests/component/http/InetHttpClientConnectFailureTest.cpp#L31) | inline call | `::close(fd);` |
| [tests/component/http/InetHttpClientConnectFailureTest.cpp:35](../../tests/component/http/InetHttpClientConnectFailureTest.cpp#L35) | inline call | `::close(fd);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:99](../../tests/component/net/AddressResolutionPolicyTest.cpp#L99) | inline call | `close(pair[0]);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:128](../../tests/component/net/AddressResolutionPolicyTest.cpp#L128) | test interposer definition | `extern "C" int close(int fd) {` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:129](../../tests/component/net/AddressResolutionPolicyTest.cpp#L129) | native symbol resolved for indirect call | `static const auto real = reinterpret_cast<decltype(&close)>(dlsym(RTLD_NEXT, "close"));` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:256](../../tests/component/net/AddressResolutionPolicyTest.cpp#L256) | inline call | `close(blocker);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:297](../../tests/component/net/AddressResolutionPolicyTest.cpp#L297) | inline call | `close(listener);` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:78](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L78) | inline call | `::close(fd);` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:92](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L92) | inline call | `::close(fd);` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:181](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L181) | inline call | `::close(activeFd);` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:255](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L255) | inline call | `::close(boundBeforeListenFd);` |
| [tests/support/NetworkTest.h:91](../../tests/support/NetworkTest.h#L91) | inline call | `::close(fd);` |
| [tests/support/NetworkTest.h:124](../../tests/support/NetworkTest.h#L124) | inline call | `::close(fd);` |
| [tests/unit/core/ConnectionAttemptLifecycleTest.cpp:42](../../tests/unit/core/ConnectionAttemptLifecycleTest.cpp#L42) | inline call | `::close(reservation);` |
| [tests/unit/core/ConnectionAttemptLifecycleTest.cpp:62](../../tests/unit/core/ConnectionAttemptLifecycleTest.cpp#L62) | inline call | `::close(reservation);` |
| [tests/unit/core/SocketWriterResourcePolicyTest.cpp:205](../../tests/unit/core/SocketWriterResourcePolicyTest.cpp#L205) | inline call | `::close(fds[0]);` |
| [tests/unit/core/SocketWriterResourcePolicyTest.cpp:208](../../tests/unit/core/SocketWriterResourcePolicyTest.cpp#L208) | inline call | `::close(fds[1]);` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:99](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L99) | inline call | `::close(fd);` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:187](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L187) | inline call | `::close(fd);` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:420](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L420) | inline call | `::close(fd);` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:80](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L80) | inline call | `::close(sockets[0]);` |
| [tests/unit/log/SemanticTerminalColorRoutingTest.cpp:85](../../tests/unit/log/SemanticTerminalColorRoutingTest.cpp#L85) | inline call | `::close(saved);` |
| [tests/unit/net/UnixPeerCredentialsTest.cpp:83](../../tests/unit/net/UnixPeerCredentialsTest.cpp#L83) | inline call | `::close(sockets[0]);` |
| [tests/unit/net/UnixPeerCredentialsTest.cpp:84](../../tests/unit/net/UnixPeerCredentialsTest.cpp#L84) | inline call | `::close(sockets[1]);` |

## connect

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/socket.cpp:76](../../src/core/system/socket.cpp#L76) | wrapper definition | `int connect(int sockfd, const sockaddr* addr, socklen_t addrlen) {` |
| [src/core/system/socket.cpp:78](../../src/core/system/socket.cpp#L78) | wrapper implementation call | `return ::connect(sockfd, addr, addrlen);` |
| [src/core/system/socket.h:71](../../src/core/system/socket.h#L71) | wrapper declaration | `int connect(int sockfd, const sockaddr* addr, socklen_t addrlen);` |
| [src/net/phy/stream/PhysicalSocketClient.hpp:56](../../src/net/phy/stream/PhysicalSocketClient.hpp#L56) | extracted caller | `return core::system::connect(Super::getFd(), &remoteAddress.getSockAddr(), remoteAddress.getSockAddrLen());` |
| [src/net/un/phy/PhysicalSocket.hpp:117](../../src/net/un/phy/PhysicalSocket.hpp#L117) | extracted caller | `const int connectResult = core::system::connect(probeFd, &address.getSockAddr(), address.getSockAddrLen());` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:77](../../tests/component/net/AddressResolutionPolicyTest.cpp#L77) | test interposer definition | `extern "C" int connect(int fd, const sockaddr* address, socklen_t length) {` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:78](../../tests/component/net/AddressResolutionPolicyTest.cpp#L78) | native symbol resolved for indirect call | `static const auto real = reinterpret_cast<decltype(&connect)>(dlsym(RTLD_NEXT, "connect"));` |
| [tests/support/NetworkTest.h:100](../../tests/support/NetworkTest.h#L100) | inline call | `require(::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0, "peer connect");` |

## curs_set

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:90](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L90) | inline call | `curs_set(0);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:246](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L246) | inline call | `curs_set(1);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:258](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L258) | inline call | `curs_set(0);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:262](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L262) | inline call | `curs_set(0);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:480](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L480) | inline call | `curs_set(0);` |

## def_prog_mode

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:452](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L452) | inline call | `def_prog_mode();` |

## dlclose

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/DynamicLoader.cpp:196](../../src/core/DynamicLoader.cpp#L196) | extracted caller | `const int ret = core::system::dlclose(library.handle);` |
| [src/core/system/dlfcn.cpp:57](../../src/core/system/dlfcn.cpp#L57) | wrapper definition | `int dlclose(void* handle) {` |
| [src/core/system/dlfcn.cpp:59](../../src/core/system/dlfcn.cpp#L59) | wrapper implementation call | `return ::dlclose(handle);` |
| [src/core/system/dlfcn.h:58](../../src/core/system/dlfcn.h#L58) | wrapper declaration | `int dlclose(void* handle);` |

## dlerror

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/DynamicLoader.cpp:191](../../src/core/DynamicLoader.cpp#L191) | extracted caller | `const char* err = core::system::dlerror();` |
| [src/core/system/dlfcn.cpp:53](../../src/core/system/dlfcn.cpp#L53) | wrapper implementation call | `dlerror();` |
| [src/core/system/dlfcn.cpp:58](../../src/core/system/dlfcn.cpp#L58) | wrapper implementation call | `dlerror();` |
| [src/core/system/dlfcn.cpp:63](../../src/core/system/dlfcn.cpp#L63) | wrapper implementation call | `dlerror();` |
| [src/core/system/dlfcn.cpp:67](../../src/core/system/dlfcn.cpp#L67) | wrapper definition | `char* dlerror() {` |
| [src/core/system/dlfcn.cpp:69](../../src/core/system/dlfcn.cpp#L69) | wrapper implementation call | `return ::dlerror();` |
| [src/core/system/dlfcn.h:60](../../src/core/system/dlfcn.h#L60) | wrapper declaration | `char* dlerror();` |

## dlopen

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/DynamicLoader.cpp:94](../../src/core/DynamicLoader.cpp#L94) | extracted caller | `handle = core::system::dlopen(libFile.c_str(), flags);` |
| [src/core/system/dlfcn.cpp:52](../../src/core/system/dlfcn.cpp#L52) | wrapper definition | `void* dlopen(const char* filename, int flags) {` |
| [src/core/system/dlfcn.cpp:54](../../src/core/system/dlfcn.cpp#L54) | wrapper implementation call | `return ::dlopen(filename, flags);` |
| [src/core/system/dlfcn.h:57](../../src/core/system/dlfcn.h#L57) | wrapper declaration | `void* dlopen(const char* filename, int flags);` |

## dlsym

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/DynamicLoader.cpp:186](../../src/core/DynamicLoader.cpp#L186) | extracted caller | `void* sym = core::system::dlsym(handle, symbol.c_str());` |
| [src/core/system/dlfcn.cpp:62](../../src/core/system/dlfcn.cpp#L62) | wrapper definition | `void* dlsym(void* handle, const char* symbol) {` |
| [src/core/system/dlfcn.cpp:64](../../src/core/system/dlfcn.cpp#L64) | wrapper implementation call | `return ::dlsym(handle, symbol);` |
| [src/core/system/dlfcn.h:59](../../src/core/system/dlfcn.h#L59) | wrapper declaration | `void* dlsym(void* handle, const char* symbol);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:49](../../tests/component/net/AddressResolutionPolicyTest.cpp#L49) | inline call | `static const auto real = reinterpret_cast<decltype(&getaddrinfo)>(dlsym(RTLD_NEXT, "getaddrinfo"));` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:78](../../tests/component/net/AddressResolutionPolicyTest.cpp#L78) | inline call | `static const auto real = reinterpret_cast<decltype(&connect)>(dlsym(RTLD_NEXT, "connect"));` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:117](../../tests/component/net/AddressResolutionPolicyTest.cpp#L117) | inline call | `static const auto real = reinterpret_cast<decltype(&getsockopt)>(dlsym(RTLD_NEXT, "getsockopt"));` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:129](../../tests/component/net/AddressResolutionPolicyTest.cpp#L129) | inline call | `static const auto real = reinterpret_cast<decltype(&close)>(dlsym(RTLD_NEXT, "close"));` |
| [tests/unit/utils/ConfigDirectoryInitializationTest.cpp:67](../../tests/unit/utils/ConfigDirectoryInitializationTest.cpp#L67) | inline call | `static const auto real = reinterpret_cast<decltype(&mkdir)>(dlsym(RTLD_NEXT, "mkdir"));` |

## dup

| Source | Role | Source line |
| --- | --- | --- |
| [tests/unit/log/SemanticTerminalColorRoutingTest.cpp:78](../../tests/unit/log/SemanticTerminalColorRoutingTest.cpp#L78) | inline call | `, saved(::dup(STDOUT_FILENO)) {` |

## dup2

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:97](../../tests/component/net/AddressResolutionPolicyTest.cpp#L97) | inline call | `if (dup2(pair[0], fd) < 0)` |
| [tests/unit/log/SemanticTerminalColorRoutingTest.cpp:80](../../tests/unit/log/SemanticTerminalColorRoutingTest.cpp#L80) | inline call | `::dup2(::fileno(file), STDOUT_FILENO);` |
| [tests/unit/log/SemanticTerminalColorRoutingTest.cpp:84](../../tests/unit/log/SemanticTerminalColorRoutingTest.cpp#L84) | inline call | `::dup2(saved, STDOUT_FILENO);` |

## endwin

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:97](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L97) | inline call | `endwin();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:453](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L453) | inline call | `endwin();` |

## epoll_create1

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/multiplexer/epoll/DescriptorEventPublisher.cpp:73](../../src/core/multiplexer/epoll/DescriptorEventPublisher.cpp#L73) | extracted caller | `epfd = core::system::epoll_create1(EPOLL_CLOEXEC);` |
| [src/core/multiplexer/epoll/EventMultiplexer.cpp:106](../../src/core/multiplexer/epoll/EventMultiplexer.cpp#L106) | extracted caller | `, epfd(core::system::epoll_create1(EPOLL_CLOEXEC)) {` |
| [src/core/system/epoll.cpp:52](../../src/core/system/epoll.cpp#L52) | wrapper definition | `int epoll_create1(int flags) {` |
| [src/core/system/epoll.cpp:54](../../src/core/system/epoll.cpp#L54) | wrapper implementation call | `return ::epoll_create1(flags);` |
| [src/core/system/epoll.h:58](../../src/core/system/epoll.h#L58) | wrapper declaration | `int epoll_create1(int flags);` |

## epoll_ctl

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/multiplexer/epoll/DescriptorEventPublisher.cpp:102](../../src/core/multiplexer/epoll/DescriptorEventPublisher.cpp#L102) | extracted caller | `if (core::system::epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ePollEvent) == 0) {` |
| [src/core/multiplexer/epoll/DescriptorEventPublisher.cpp:124](../../src/core/multiplexer/epoll/DescriptorEventPublisher.cpp#L124) | extracted caller | `if (core::system::epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) == 0) {` |
| [src/core/multiplexer/epoll/DescriptorEventPublisher.cpp:154](../../src/core/multiplexer/epoll/DescriptorEventPublisher.cpp#L154) | extracted caller | `if (core::system::epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ePollEvent) != 0) {` |
| [src/core/multiplexer/epoll/EventMultiplexer.cpp:82](../../src/core/multiplexer/epoll/EventMultiplexer.cpp#L82) | extracted caller | `if (core::system::epoll_ctl(epfd, EPOLL_CTL_ADD, publisherFd, &event) == 0) {` |
| [src/core/system/epoll.cpp:67](../../src/core/system/epoll.cpp#L67) | wrapper definition | `int epoll_ctl(int epfd, int op, int fd, epoll_event* event) {` |
| [src/core/system/epoll.cpp:69](../../src/core/system/epoll.cpp#L69) | wrapper implementation call | `return ::epoll_ctl(epfd, op, fd, event);` |
| [src/core/system/epoll.h:61](../../src/core/system/epoll.h#L61) | wrapper declaration | `int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);` |

## epoll_pwait

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/multiplexer/epoll/EventMultiplexer.cpp:127](../../src/core/multiplexer/epoll/EventMultiplexer.cpp#L127) | extracted caller | `return core::system::epoll_pwait(epfd, ePollEvents, 3, tickTimeout.getMs(), &sigMask);` |
| [src/core/system/epoll.cpp:62](../../src/core/system/epoll.cpp#L62) | wrapper definition | `int epoll_pwait(int epfd, struct epoll_event* events, int maxevents, int timeout, const sigset_t* sigMask) {` |
| [src/core/system/epoll.cpp:64](../../src/core/system/epoll.cpp#L64) | wrapper implementation call | `return ::epoll_pwait(epfd, events, maxevents, timeout, sigMask);` |
| [src/core/system/epoll.h:60](../../src/core/system/epoll.h#L60) | wrapper declaration | `int epoll_pwait(int epfd, struct epoll_event* events, int maxevents, int timeout, const sigset_t* sigMask);` |

## epoll_wait

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/multiplexer/epoll/DescriptorEventPublisher.cpp:210](../../src/core/multiplexer/epoll/DescriptorEventPublisher.cpp#L210) | extracted caller | `const int count = core::system::epoll_wait(ePollEvents.getEPFd(), ePollEvents.getEvents(), ePollEvents.getInterestCount(), 0);` |
| [src/core/system/epoll.cpp:57](../../src/core/system/epoll.cpp#L57) | wrapper definition | `int epoll_wait(int epfd, epoll_event* events, int maxevents, int timeout) {` |
| [src/core/system/epoll.cpp:59](../../src/core/system/epoll.cpp#L59) | wrapper implementation call | `return ::epoll_wait(epfd, events, maxevents, timeout);` |
| [src/core/system/epoll.h:59](../../src/core/system/epoll.h#L59) | wrapper declaration | `int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);` |

## erase

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:178](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L178) | inline call | `erase();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:193](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L193) | inline call | `erase();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:382](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L382) | inline call | `erase();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:542](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L542) | inline call | `erase();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:743](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L743) | inline call | `erase();` |

## exit

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/SNodeC.cpp:56](../../src/core/SNodeC.cpp#L56) | inline call | `exit(1);` |

## fchdir

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/core/FileReaderTest.cpp:133](../../tests/component/core/FileReaderTest.cpp#L133) | inline call | `static_cast<void>(::fchdir(savedWorkingDirectory));` |

## fclose

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/UnixLegacyServerClientCompositionTest.cpp:87](../../tests/component/net/UnixLegacyServerClientCompositionTest.cpp#L87) | inline call | `std::fclose(staleSocketPathFile);` |
| [tests/component/net/UnixLegacyServerMultipleClientsCompositionTest.cpp:87](../../tests/component/net/UnixLegacyServerMultipleClientsCompositionTest.cpp#L87) | inline call | `std::fclose(staleSocketPathFile);` |
| [tests/unit/log/SemanticTerminalColorRoutingTest.cpp:86](../../tests/unit/log/SemanticTerminalColorRoutingTest.cpp#L86) | inline call | `::fclose(file);` |

## fcntl

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/multiplexer/poll/EventMultiplexer.cpp:94](../../src/core/multiplexer/poll/EventMultiplexer.cpp#L94) | inline call | `descriptorFlags = ::fcntl(fd, F_GETFD);` |
| [src/core/multiplexer/select/DescriptorEventPublisher.cpp:98](../../src/core/multiplexer/select/DescriptorEventPublisher.cpp#L98) | inline call | `descriptorFlags = ::fcntl(fd, F_GETFD);` |
| [src/core/pipe/Pipe.cpp:81](../../src/core/pipe/Pipe.cpp#L81) | inline call | `flags = ::fcntl(fd, F_GETFL);` |
| [src/core/pipe/Pipe.cpp:91](../../src/core/pipe/Pipe.cpp#L91) | inline call | `result = ::fcntl(fd, F_SETFL, flags &#124; O_NONBLOCK);` |
| [src/utils/Config.cpp:795](../../src/utils/Config.cpp#L795) | inline call | `} else if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) >= 0) {` |
| [tests/component/core/FileReaderTest.cpp:35](../../tests/component/core/FileReaderTest.cpp#L35) | inline call | `return ::fcntl(fd, F_GETFD) < 0 && errno == EBADF;` |
| [tests/component/core/PipeBoundedQueueTest.cpp:37](../../tests/component/core/PipeBoundedQueueTest.cpp#L37) | inline call | `testResult.expectTrue((::fcntl(pipe.getReadFd(), F_GETFL) & O_NONBLOCK) == 0 && (::fcntl(pipe.getWriteFd(), F_GETFL) & O_NONBLOCK) == 0,` |
| [tests/component/core/PipeBoundedQueueTest.cpp:37](../../tests/component/core/PipeBoundedQueueTest.cpp#L37) | inline call | `testResult.expectTrue((::fcntl(pipe.getReadFd(), F_GETFL) & O_NONBLOCK) == 0 && (::fcntl(pipe.getWriteFd(), F_GETFL) & O_NONBLOCK) == 0,` |
| [tests/component/core/PipeBoundedQueueTest.cpp:61](../../tests/component/core/PipeBoundedQueueTest.cpp#L61) | inline call | `testResult.expectTrue((::fcntl(sourceFd, F_GETFL) & O_NONBLOCK) != 0 && (::fcntl(sinkFd, F_GETFL) & O_NONBLOCK) != 0,` |
| [tests/component/core/PipeBoundedQueueTest.cpp:61](../../tests/component/core/PipeBoundedQueueTest.cpp#L61) | inline call | `testResult.expectTrue((::fcntl(sourceFd, F_GETFL) & O_NONBLOCK) != 0 && (::fcntl(sinkFd, F_GETFL) & O_NONBLOCK) != 0,` |
| [tests/component/core/PipeBoundedQueueTest.cpp:69](../../tests/component/core/PipeBoundedQueueTest.cpp#L69) | inline call | `sourceDescriptorClosedBeforeCallback = ::fcntl(sourceFd, F_GETFD) < 0 && errno == EBADF;` |
| [tests/component/core/PipeBoundedQueueTest.cpp:83](../../tests/component/core/PipeBoundedQueueTest.cpp#L83) | inline call | `sinkDescriptorClosedBeforeCallback = ::fcntl(sinkFd, F_GETFD) < 0 && errno == EBADF;` |
| [tests/component/core/PipeOwnershipTest.cpp:24](../../tests/component/core/PipeOwnershipTest.cpp#L24) | inline call | `return ::fcntl(fd, F_GETFD) < 0 && errno == EBADF;` |
| [tests/component/core/PipeOwnershipTest.cpp:47](../../tests/component/core/PipeOwnershipTest.cpp#L47) | inline call | `const int readStatusFlags = ::fcntl(destructorReadFd, F_GETFL);` |
| [tests/component/core/PipeOwnershipTest.cpp:48](../../tests/component/core/PipeOwnershipTest.cpp#L48) | inline call | `const int writeStatusFlags = ::fcntl(destructorWriteFd, F_GETFL);` |
| [tests/component/core/PipeOwnershipTest.cpp:49](../../tests/component/core/PipeOwnershipTest.cpp#L49) | inline call | `const int readDescriptorFlags = ::fcntl(destructorReadFd, F_GETFD);` |
| [tests/component/core/PipeOwnershipTest.cpp:50](../../tests/component/core/PipeOwnershipTest.cpp#L50) | inline call | `const int writeDescriptorFlags = ::fcntl(destructorWriteFd, F_GETFD);` |
| [tests/component/core/PipeTimeoutTest.cpp:65](../../tests/component/core/PipeTimeoutTest.cpp#L65) | inline call | `descriptorClosedBeforeCallback = ::fcntl(sinkFd, F_GETFD) < 0 && errno == EBADF;` |
| [tests/support/NetworkTest.h:101](../../tests/support/NetworkTest.h#L101) | inline call | `require(fcntl(fd, F_SETFL, O_NONBLOCK) == 0, "peer nonblocking");` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:125](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L125) | inline call | `const int flags = ::fcntl(fd, F_GETFL, 0);` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:127](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L127) | inline call | `static_cast<void>(::fcntl(fd, F_SETFL, flags &#124; O_NONBLOCK));` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:409](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L409) | inline call | `const int flags = ::fcntl(fd, F_GETFL, 0);` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:411](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L411) | inline call | `static_cast<void>(::fcntl(fd, F_SETFL, flags &#124; O_NONBLOCK));` |

## fflush

| Source | Role | Source line |
| --- | --- | --- |
| [tests/unit/log/SemanticTerminalColorRoutingTest.cpp:83](../../tests/unit/log/SemanticTerminalColorRoutingTest.cpp#L83) | inline call | `::fflush(stdout);` |
| [tests/unit/log/SemanticTerminalColorRoutingTest.cpp:89](../../tests/unit/log/SemanticTerminalColorRoutingTest.cpp#L89) | inline call | `::fflush(stdout);` |

## fileno

| Source | Role | Source line |
| --- | --- | --- |
| [src/log/detail/SpdlogBackend.cpp:152](../../src/log/detail/SpdlogBackend.cpp#L152) | inline call | `disableColor = ::isatty(::fileno(stdout)) == 0;` |
| [tests/unit/log/SemanticTerminalColorRoutingTest.cpp:80](../../tests/unit/log/SemanticTerminalColorRoutingTest.cpp#L80) | inline call | `::dup2(::fileno(file), STDOUT_FILENO);` |

## flock

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/unistd.cpp:82](../../src/core/system/unistd.cpp#L82) | wrapper definition | `int flock(int lockFd, int operation) {` |
| [src/core/system/unistd.cpp:84](../../src/core/system/unistd.cpp#L84) | wrapper implementation call | `return ::flock(lockFd, operation);` |
| [src/core/system/unistd.h:71](../../src/core/system/unistd.h#L71) | wrapper declaration | `int flock(int lockFd, int operation);` |
| [src/net/un/phy/PhysicalSocket.hpp:240](../../src/net/un/phy/PhysicalSocket.hpp#L240) | extracted caller | `if (core::system::flock(lockFd, LOCK_UN) == 0) {` |
| [src/net/un/phy/PhysicalSocket.hpp:335](../../src/net/un/phy/PhysicalSocket.hpp#L335) | extracted caller | `if (core::system::flock(lockFd, LOCK_EX &#124; LOCK_NB) != 0) {` |

## fopen

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/UnixLegacyServerClientCompositionTest.cpp:83](../../tests/component/net/UnixLegacyServerClientCompositionTest.cpp#L83) | inline call | `FILE* staleSocketPathFile = std::fopen(socketPath.c_str(), "w");` |
| [tests/component/net/UnixLegacyServerMultipleClientsCompositionTest.cpp:83](../../tests/component/net/UnixLegacyServerMultipleClientsCompositionTest.cpp#L83) | inline call | `FILE* staleSocketPathFile = std::fopen(socketPath.c_str(), "w");` |
| [tests/unit/log/SemanticTerminalColorRoutingTest.cpp:79](../../tests/unit/log/SemanticTerminalColorRoutingTest.cpp#L79) | inline call | `file = ::fopen(this->path.c_str(), "w+");` |

## fork

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Daemon.cpp:82](../../src/utils/Daemon.cpp#L82) | inline call | `pid_t pid = fork();` |
| [src/utils/Daemon.cpp:103](../../src/utils/Daemon.cpp#L103) | inline call | `pid = fork();` |
| [tests/component/core/ShutdownReceiverNotificationTest.cpp:94](../../tests/component/core/ShutdownReceiverNotificationTest.cpp#L94) | inline call | `signalerPid = ::fork();` |

## free

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:103](../../src/core/socket/stream/tls/ssl_utils.cpp#L103) | inline call | `free(u);` |
| [src/utils/CLI11.hpp:1312](../../src/utils/CLI11.hpp#L1312) | inline call | `free(buffer);` |

## freeaddrinfo

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/netdb.cpp:67](../../src/core/system/netdb.cpp#L67) | wrapper definition | `void freeaddrinfo(struct addrinfo* res) {` |
| [src/core/system/netdb.cpp:69](../../src/core/system/netdb.cpp#L69) | wrapper implementation call | `::freeaddrinfo(res);` |
| [src/core/system/netdb.h:63](../../src/core/system/netdb.h#L63) | wrapper declaration | `void freeaddrinfo(struct addrinfo* res);` |
| [src/net/in/SocketAddrInfo.cpp:63](../../src/net/in/SocketAddrInfo.cpp#L63) | callback reference | `addrInfo.reset(resolved, freeaddrinfo);` |
| [src/net/in6/SocketAddrInfo.cpp:63](../../src/net/in6/SocketAddrInfo.cpp#L63) | callback reference | `addrInfo.reset(resolved, freeaddrinfo);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:66](../../tests/component/net/AddressResolutionPolicyTest.cpp#L66) | inline call | `freeaddrinfo(*output);` |

## freopen

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Daemon.cpp:152](../../src/utils/Daemon.cpp#L152) | inline call | `if (std::freopen("/dev/null", "r", stdin) == nullptr) {` |
| [src/utils/Daemon.cpp:154](../../src/utils/Daemon.cpp#L154) | inline call | `if (std::freopen("/dev/null", "w+", stdout) == nullptr) {` |
| [src/utils/Daemon.cpp:156](../../src/utils/Daemon.cpp#L156) | inline call | `if (std::freopen("/dev/null", "w+", stderr) == nullptr) {` |

## fstat

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/phy/PhysicalSocket.hpp:318](../../src/net/un/phy/PhysicalSocket.hpp#L318) | inline call | `const int lockStatResult = ::fstat(lockFd, &lockStatus);` |
| [src/net/un/phy/PhysicalSocket.hpp:370](../../src/net/un/phy/PhysicalSocket.hpp#L370) | inline call | `if (::fstat(lockFd, &markerStatus) != 0 &#124;&#124; !detail::validateFilesystemLockMarker(lockFd, markerStatus)) {` |

## ftruncate

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/phy/PhysicalSocket.hpp:86](../../src/net/un/phy/PhysicalSocket.hpp#L86) | inline call | `return ::ftruncate(fd, static_cast<off_t>(FILESYSTEM_LOCK_MARKER_SIZE)) == 0;` |

## gai_strerror

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/in/SocketAddress.cpp:106](../../src/net/in/SocketAddress.cpp#L106) | inline call | `state, aiErrCode == EAI_SYSTEM ? strerror(errno) : gai_strerror(aiErrCode), aiErrCode == EAI_SYSTEM ? errno : aiErrCode);` |
| [src/net/in/SocketAddress.cpp:138](../../src/net/in/SocketAddress.cpp#L138) | inline call | `(aiErrCode == EAI_SYSTEM ? strerror(errno) : gai_strerror(aiErrCode)),` |
| [src/net/in6/SocketAddress.cpp:106](../../src/net/in6/SocketAddress.cpp#L106) | inline call | `state, aiErrCode == EAI_SYSTEM ? strerror(errno) : gai_strerror(aiErrCode), aiErrCode == EAI_SYSTEM ? errno : aiErrCode);` |
| [src/net/in6/SocketAddress.cpp:138](../../src/net/in6/SocketAddress.cpp#L138) | inline call | `(aiErrCode == EAI_SYSTEM ? strerror(errno) : gai_strerror(aiErrCode)),` |

## getaddrinfo

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/netdb.cpp:62](../../src/core/system/netdb.cpp#L62) | wrapper definition | `int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res) {` |
| [src/core/system/netdb.cpp:64](../../src/core/system/netdb.cpp#L64) | wrapper implementation call | `return ::getaddrinfo(node, service, hints, res);` |
| [src/core/system/netdb.h:60](../../src/core/system/netdb.h#L60) | wrapper declaration | `int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res);` |
| [src/net/in/SocketAddrInfo.cpp:61](../../src/net/in/SocketAddrInfo.cpp#L61) | extracted caller | `const int aiErrCode = core::system::getaddrinfo(node.c_str(), service.c_str(), &hints, &resolved);` |
| [src/net/in6/SocketAddrInfo.cpp:61](../../src/net/in6/SocketAddrInfo.cpp#L61) | extracted caller | `const int aiErrCode = core::system::getaddrinfo(node.c_str(), service.c_str(), &hints, &resolved);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:48](../../tests/component/net/AddressResolutionPolicyTest.cpp#L48) | test interposer definition | `extern "C" int getaddrinfo(const char* node, const char* service, const addrinfo* hints, addrinfo** output) {` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:49](../../tests/component/net/AddressResolutionPolicyTest.cpp#L49) | native symbol resolved for indirect call | `static const auto real = reinterpret_cast<decltype(&getaddrinfo)>(dlsym(RTLD_NEXT, "getaddrinfo"));` |

## getch

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:188](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L188) | inline call | `getch();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:205](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L205) | inline call | `const int ch = getch();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:256](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L256) | inline call | `const int ch = getch();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:396](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L396) | inline call | `choice = getch();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:641](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L641) | inline call | `const int ch = getch();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:758](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L758) | inline call | `choice = getch();` |

## getegid

| Source | Role | Source line |
| --- | --- | --- |
| [tests/unit/net/UnixPeerCredentialsTest.cpp:66](../../tests/unit/net/UnixPeerCredentialsTest.cpp#L66) | inline call | `result.expectTrue(credentials.gid == ::getegid(), "reported peer gid matches effective gid");` |
| [tests/unit/net/UnixPeerCredentialsTest.cpp:74](../../tests/unit/net/UnixPeerCredentialsTest.cpp#L74) | inline call | `result.expectTrue(credentials.gid == ::getegid(), "getpeereid reports the effective peer gid");` |

## getentropy

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/websocket/client/SocketContextUpgradeFactory.cpp:73](../../src/web/websocket/client/SocketContextUpgradeFactory.cpp#L73) | local fallback declaration/definition | `int getentropy(void* buf, size_t buflen);` |
| [src/web/websocket/client/SocketContextUpgradeFactory.cpp:74](../../src/web/websocket/client/SocketContextUpgradeFactory.cpp#L74) | local fallback declaration/definition | `int getentropy(void* buf, size_t buflen) {` |
| [src/web/websocket/client/SocketContextUpgradeFactory.cpp:92](../../src/web/websocket/client/SocketContextUpgradeFactory.cpp#L92) | inline call | `if (getentropy(ebytes, 16) != 0) {` |

## getenv

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:1319](../../src/utils/CLI11.hpp#L1319) | inline call | `buffer = std::getenv(env_name.c_str());` |
| [src/utils/Config.cpp:966](../../src/utils/Config.cpp#L966) | inline call | `const char* homedir = std::getenv("HOME");` |
| [src/utils/Config.cpp:971](../../src/utils/Config.cpp#L971) | inline call | `const char* xdgConfigHome = std::getenv("XDG_CONFIG_HOME");` |
| [src/utils/Config.cpp:976](../../src/utils/Config.cpp#L976) | inline call | `const char* xdgStateHome = std::getenv("XDG_STATE_HOME");` |
| [src/utils/Config.cpp:981](../../src/utils/Config.cpp#L981) | inline call | `const char* xdgRuntimeHome = std::getenv("XDG_RUNTIME_DIR");` |
| [src/web/http/client/SocketContextUpgradeFactorySelector.cpp:69](../../src/web/http/client/SocketContextUpgradeFactorySelector.cpp#L69) | inline call | `if (const char* httpUpgradeInstallLibdirEnv = std::getenv("HTTP_UPGRADE_INSTALL_LIBDIR")) {` |
| [src/web/http/server/SocketContextUpgradeFactorySelector.cpp:69](../../src/web/http/server/SocketContextUpgradeFactorySelector.cpp#L69) | inline call | `if (const char* httpUpgradeInstallLibdirEnv = std::getenv("HTTP_UPGRADE_INSTALL_LIBDIR")) {` |
| [src/web/websocket/client/SubProtocolFactorySelector.cpp:83](../../src/web/websocket/client/SubProtocolFactorySelector.cpp#L83) | inline call | `if (const char* websocketSubprotocolInstallLibdirEnv = std::getenv("WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR")) {` |
| [src/web/websocket/server/SubProtocolFactorySelector.cpp:83](../../src/web/websocket/server/SubProtocolFactorySelector.cpp#L83) | inline call | `if (const char* websocketSubprotocolInstallLibdirEnv = std::getenv("WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR")) {` |

## geteuid

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Config.cpp:961](../../src/utils/Config.cpp#L961) | inline call | `} else if ((euid = geteuid()) == 0) {` |
| [src/utils/Config.cpp:1000](../../src/utils/Config.cpp#L1000) | inline call | `if (geteuid() == 0) {` |
| [src/utils/Config.cpp:1022](../../src/utils/Config.cpp#L1022) | inline call | `if (geteuid() == 0) {` |
| [src/utils/Config.cpp:1042](../../src/utils/Config.cpp#L1042) | inline call | `geteuid() == 0 ? std::filesystem::perms::owner_all &#124; std::filesystem::perms::group_all` |
| [src/utils/Config.cpp:1044](../../src/utils/Config.cpp#L1044) | inline call | `if (geteuid() == 0) {` |
| [tests/support/TestResult.cpp:56](../../tests/support/TestResult.cpp#L56) | inline call | `const bool skip = geteuid() == 0 && getgrnam("snodec") == nullptr;` |
| [tests/unit/core/SNodeCAsyncLogDrainTest.cpp:65](../../tests/unit/core/SNodeCAsyncLogDrainTest.cpp#L65) | inline call | `if (geteuid() == 0) {` |
| [tests/unit/core/SNodeCInitFreeSmokeTest.cpp:56](../../tests/unit/core/SNodeCInitFreeSmokeTest.cpp#L56) | inline call | `if (geteuid() == 0) {` |
| [tests/unit/net/UnixPeerCredentialsTest.cpp:65](../../tests/unit/net/UnixPeerCredentialsTest.cpp#L65) | inline call | `result.expectTrue(credentials.uid == ::geteuid(), "reported peer uid matches effective uid");` |
| [tests/unit/net/UnixPeerCredentialsTest.cpp:73](../../tests/unit/net/UnixPeerCredentialsTest.cpp#L73) | inline call | `result.expectTrue(credentials.uid == ::geteuid(), "getpeereid reports the effective peer uid");` |
| [tests/unit/utils/ConfigDirectoryInitializationTest.cpp:77](../../tests/unit/utils/ConfigDirectoryInitializationTest.cpp#L77) | inline call | `if (geteuid() == 0) {` |

## getgid

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Daemon.cpp:206](../../src/utils/Daemon.cpp#L206) | inline call | `if (setegid(getgid()) != 0) {` |

## getgrgid

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Config.cpp:959](../../src/utils/Config.cpp#L959) | inline call | `} else if ((gr = getgrgid(pw->pw_gid)) == nullptr) {` |

## getgrnam

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Config.cpp:1002](../../src/utils/Config.cpp#L1002) | inline call | `if ((gr = getgrnam(XSTR(GROUP_NAME))) != nullptr) {` |
| [src/utils/Config.cpp:1024](../../src/utils/Config.cpp#L1024) | inline call | `if ((gr = getgrnam(XSTR(GROUP_NAME))) != nullptr) {` |
| [src/utils/Config.cpp:1046](../../src/utils/Config.cpp#L1046) | inline call | `if ((gr = getgrnam(XSTR(GROUP_NAME))) != nullptr) {` |
| [src/utils/Daemon.cpp:125](../../src/utils/Daemon.cpp#L125) | inline call | `if (((void) (errno = 0), gr = getgrnam(groupName.c_str())) == nullptr) {` |
| [tests/support/TestResult.cpp:56](../../tests/support/TestResult.cpp#L56) | inline call | `const bool skip = geteuid() == 0 && getgrnam("snodec") == nullptr;` |

## getmaxyx

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:181](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L181) | inline call | `getmaxyx(stdscr, maxY, maxX);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:196](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L196) | inline call | `getmaxyx(stdscr, maxY, maxX);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:243](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L243) | inline call | `getmaxyx(stdscr, maxY, maxX);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:385](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L385) | inline call | `getmaxyx(stdscr, maxY, maxX);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:545](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L545) | inline call | `getmaxyx(stdscr, maxY, maxX);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:746](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L746) | inline call | `getmaxyx(stdscr, maxY, maxX);` |

## getnameinfo

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/netdb.cpp:72](../../src/core/system/netdb.cpp#L72) | wrapper definition | `int getnameinfo(const sockaddr* addr, socklen_t addrlen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, int flags) {` |
| [src/core/system/netdb.cpp:74](../../src/core/system/netdb.cpp#L74) | wrapper implementation call | `return ::getnameinfo(addr, addrlen, host, HOSTLEN_CAST(hostlen), serv, SERVLEN_CAST(servlen), flags);` |
| [src/core/system/netdb.h:66](../../src/core/system/netdb.h#L66) | wrapper declaration | `int getnameinfo(const sockaddr* addr, socklen_t addrlen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, int flags);` |
| [src/net/in/SocketAddrInfo.cpp:102](../../src/net/in/SocketAddrInfo.cpp#L102) | inline call | `getnameinfo(currentAddrInfo->ai_addr,` |
| [src/net/in/SocketAddress.cpp:83](../../src/net/in/SocketAddress.cpp#L83) | extracted caller | `const int aiErrCode = core::system::getnameinfo(reinterpret_cast<const sockaddr*>(&sockAddr),` |
| [src/net/in6/SocketAddrInfo.cpp:95](../../src/net/in6/SocketAddrInfo.cpp#L95) | inline call | `getnameinfo(currentAddrInfo->ai_addr,` |
| [src/net/in6/SocketAddress.cpp:83](../../src/net/in6/SocketAddress.cpp#L83) | extracted caller | `const int aiErrCode = core::system::getnameinfo(reinterpret_cast<const sockaddr*>(&sockAddr),` |

## getpeereid

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/PeerCredentials.cpp:74](../../src/net/un/PeerCredentials.cpp#L74) | inline call | `if (::getpeereid(fd, &uid, &gid) < 0) {` |

## getpeername

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/socket.cpp:86](../../src/core/system/socket.cpp#L86) | wrapper definition | `int getpeername(int sockfd, sockaddr* addr, socklen_t* addrlen) {` |
| [src/core/system/socket.cpp:88](../../src/core/system/socket.cpp#L88) | wrapper implementation call | `return ::getpeername(sockfd, addr, addrlen);` |
| [src/core/system/socket.h:62](../../src/core/system/socket.h#L62) | wrapper declaration | `int getpeername(int sockfd, sockaddr* addr, socklen_t* addrlen);` |
| [src/net/phy/PhysicalSocket.hpp:158](../../src/net/phy/PhysicalSocket.hpp#L158) | extracted caller | `return core::system::getpeername(core::Descriptor::getFd(), reinterpret_cast<sockaddr*>(&remoteSockAddr), &remoteSockAddrLen);` |

## getpgrp

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:261](../../src/tools/snodec-control/src/ProcessRunner.cpp#L261) | inline call | `const pid_t parentPgrp = haveTerminal ? ::getpgrp() : -1;` |

## getpid

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/config/ConfigAddress.cpp:79](../../src/net/un/config/ConfigAddress.cpp#L79) | inline call | `std::string('\0' + instance->getInstanceName() + std::to_string(getpid()) + "_" + utils::Uuid::getUuid()),` |
| [src/utils/Config.cpp:793](../../src/utils/Config.cpp#L793) | inline call | `if (pidFile >> pid && getpid() == pid) {` |
| [src/utils/Config.cpp:903](../../src/utils/Config.cpp#L903) | inline call | `std::cout << "Pid: " << getpid() << ", child pid: " << e.getPid() << ": " << e.what() << std::endl;` |
| [tests/component/core/SNodeCReconfigureTest.cpp:72](../../tests/component/core/SNodeCReconfigureTest.cpp#L72) | inline call | `const pid_t processId = getpid();` |
| [tests/component/core/SNodeCReconfigureTest.cpp:105](../../tests/component/core/SNodeCReconfigureTest.cpp#L105) | inline call | `result.expectTrue(getpid() == processId, "runtime daemonize option does not fork the application");` |
| [tests/component/core/ShutdownReceiverNotificationTest.cpp:105](../../tests/component/core/ShutdownReceiverNotificationTest.cpp#L105) | inline call | `static_cast<void>(::kill(::getpid(), SIGTERM));` |
| [tests/component/express/UnixExpressTransportSmokeTest.cpp:26](../../tests/component/express/UnixExpressTransportSmokeTest.cpp#L26) | inline call | `return "/tmp/snodec-express-component-" + std::to_string(::getpid()) + ".sock";` |
| [tests/component/http/UnixHttpServerClientGetRoundTripTest.cpp:13](../../tests/component/http/UnixHttpServerClientGetRoundTripTest.cpp#L13) | inline call | `return "/tmp/snodec-http-component-" + std::to_string(::getpid()) + ".sock";` |
| [tests/component/http/UnixHttpServerClientPostBodyRoundTripTest.cpp:13](../../tests/component/http/UnixHttpServerClientPostBodyRoundTripTest.cpp#L13) | inline call | `return "/tmp/snodec-http-component-postbody-" + std::to_string(::getpid()) + ".sock";` |
| [tests/component/http/UnixHttpServerClientStatusVariantTest.cpp:13](../../tests/component/http/UnixHttpServerClientStatusVariantTest.cpp#L13) | inline call | `return "/tmp/snodec-http-component-statusvariant-" + std::to_string(::getpid()) + ".sock";` |
| [tests/component/net/UnixLegacyClientConnectFailureTest.cpp:72](../../tests/component/net/UnixLegacyClientConnectFailureTest.cpp#L72) | inline call | `return "/tmp/snodec-unix-connect-failure-" + std::to_string(getpid()) + ".sock";` |
| [tests/component/net/UnixLegacyServerClientCompositionTest.cpp:75](../../tests/component/net/UnixLegacyServerClientCompositionTest.cpp#L75) | inline call | `return "/tmp/snodec-unix-composition-" + std::to_string(getpid()) + ".sock";` |
| [tests/component/net/UnixLegacyServerClientDisconnectLifecycleTest.cpp:85](../../tests/component/net/UnixLegacyServerClientDisconnectLifecycleTest.cpp#L85) | inline call | `return "/tmp/snodec-unix-disconnect-" + std::to_string(getpid()) + ".sock";` |
| [tests/component/net/UnixLegacyServerClientFramedPayloadExchangeTest.cpp:291](../../tests/component/net/UnixLegacyServerClientFramedPayloadExchangeTest.cpp#L291) | inline call | `return "/tmp/snodec-unix-framed-" + std::to_string(getpid()) + ".sock";` |
| [tests/component/net/UnixLegacyServerClientLargePayloadExchangeTest.cpp:84](../../tests/component/net/UnixLegacyServerClientLargePayloadExchangeTest.cpp#L84) | inline call | `return "/tmp/snodec-unix-large-payload-" + std::to_string(getpid()) + ".sock";` |
| [tests/component/net/UnixLegacyServerClientMultiMessagePayloadExchangeTest.cpp:87](../../tests/component/net/UnixLegacyServerClientMultiMessagePayloadExchangeTest.cpp#L87) | inline call | `return "/tmp/snodec-unix-multi-message-" + std::to_string(getpid()) + ".sock";` |
| [tests/component/net/UnixLegacyServerClientPayloadExchangeTest.cpp:82](../../tests/component/net/UnixLegacyServerClientPayloadExchangeTest.cpp#L82) | inline call | `return "/tmp/snodec-unix-payload-" + std::to_string(getpid()) + ".sock";` |
| [tests/component/net/UnixLegacyServerControlledCloseLifecycleTest.cpp:69](../../tests/component/net/UnixLegacyServerControlledCloseLifecycleTest.cpp#L69) | inline call | `return "/tmp/snodec-unix-server-close-" + std::to_string(getpid()) + ".sock";` |
| [tests/component/net/UnixLegacyServerMultipleClientsCompositionTest.cpp:75](../../tests/component/net/UnixLegacyServerMultipleClientsCompositionTest.cpp#L75) | inline call | `return "/tmp/snodec-unix-multi-client-" + std::to_string(getpid()) + ".sock";` |
| [tests/component/net/UnixLegacyServerMultipleClientsDisconnectLifecycleTest.cpp:70](../../tests/component/net/UnixLegacyServerMultipleClientsDisconnectLifecycleTest.cpp#L70) | inline call | `return "/tmp/snodec-unix-multi-disconnect-" + std::to_string(getpid()) + ".sock";` |
| [tests/component/net/UnixLegacyServerMultipleClientsPayloadExchangeTest.cpp:88](../../tests/component/net/UnixLegacyServerMultipleClientsPayloadExchangeTest.cpp#L88) | inline call | `return "/tmp/snodec-unix-multi-payload-" + std::to_string(getpid()) + ".sock";` |
| [tests/component/websocket/UnixWebSocketServerClientTextEchoTest.cpp:13](../../tests/component/websocket/UnixWebSocketServerClientTextEchoTest.cpp#L13) | inline call | `return "/tmp/snodec-websocket-component-" + std::to_string(::getpid()) + ".sock";` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:591](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L591) | inline call | `static_cast<void>(::kill(::getpid(), SIGTERM));` |
| [tests/unit/net/UnixPeerCredentialsTest.cpp:69](../../tests/unit/net/UnixPeerCredentialsTest.cpp#L69) | inline call | `result.expectTrue(*credentials.pid == ::getpid(), "reported Linux peer pid matches process pid");` |

## getppid

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/core/ShutdownReceiverNotificationTest.cpp:98](../../tests/component/core/ShutdownReceiverNotificationTest.cpp#L98) | inline call | `const int killResult = ::kill(::getppid(), SIGINT);` |

## getpwnam

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Daemon.cpp:134](../../src/utils/Daemon.cpp#L134) | inline call | `if (((void) (errno = 0), (pw = getpwnam(userName.c_str())) == nullptr)) {` |

## getpwuid

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Config.cpp:957](../../src/utils/Config.cpp#L957) | inline call | `if ((pw = getpwuid(getuid())) == nullptr) {` |

## getsockname

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/socket.cpp:81](../../src/core/system/socket.cpp#L81) | wrapper definition | `int getsockname(int sockfd, sockaddr* addr, socklen_t* addrlen) {` |
| [src/core/system/socket.cpp:83](../../src/core/system/socket.cpp#L83) | wrapper implementation call | `return ::getsockname(sockfd, addr, addrlen);` |
| [src/core/system/socket.h:61](../../src/core/system/socket.h#L61) | wrapper declaration | `int getsockname(int sockfd, sockaddr* addr, socklen_t* addrlen);` |
| [src/net/phy/PhysicalSocket.hpp:152](../../src/net/phy/PhysicalSocket.hpp#L152) | extracted caller | `return core::system::getsockname(core::Descriptor::getFd(), reinterpret_cast<sockaddr*>(&localSockAddr), &localSockAddrLen);` |
| [tests/component/http/InetHttpClientConnectFailureTest.cpp:30](../../tests/component/http/InetHttpClientConnectFailureTest.cpp#L30) | inline call | `if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:196](../../tests/component/net/AddressResolutionPolicyTest.cpp#L196) | inline call | `getsockname(blocker, reinterpret_cast<sockaddr*>(&occupied), &length);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:268](../../tests/component/net/AddressResolutionPolicyTest.cpp#L268) | inline call | `getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length);` |
| [tests/unit/core/ConnectionAttemptLifecycleTest.cpp:17](../../tests/unit/core/ConnectionAttemptLifecycleTest.cpp#L17) | inline call | `getsockname(reservation, reinterpret_cast<sockaddr*>(&addr), &size);` |

## getsockopt

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/socket.cpp:101](../../src/core/system/socket.cpp#L101) | wrapper definition | `int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen) {` |
| [src/core/system/socket.cpp:103](../../src/core/system/socket.cpp#L103) | wrapper implementation call | `return ::getsockopt(sockfd, level, optname, optval, optlen);` |
| [src/core/system/socket.h:74](../../src/core/system/socket.h#L74) | wrapper declaration | `int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen);` |
| [src/net/phy/PhysicalSocket.hpp:146](../../src/net/phy/PhysicalSocket.hpp#L146) | extracted caller | `return core::system::getsockopt(PhysicalSocket::getFd(), level, optname, optval, optlen);` |
| [src/net/un/PeerCredentials.cpp:61](../../src/net/un/PeerCredentials.cpp#L61) | inline call | `if (::getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &credentials, &credentialsLength) < 0) {` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:116](../../tests/component/net/AddressResolutionPolicyTest.cpp#L116) | test interposer definition | `extern "C" int getsockopt(int fd, int level, int option, void* value, socklen_t* length) noexcept {` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:117](../../tests/component/net/AddressResolutionPolicyTest.cpp#L117) | native symbol resolved for indirect call | `static const auto real = reinterpret_cast<decltype(&getsockopt)>(dlsym(RTLD_NEXT, "getsockopt"));` |

## gettimeofday

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Timeval.cpp:78](../../src/utils/Timeval.cpp#L78) | extracted caller | `utils::system::gettimeofday(&currentTime, nullptr);` |
| [src/utils/system/time.cpp:57](../../src/utils/system/time.cpp#L57) | wrapper definition | `int gettimeofday(struct timeval* tv, struct timezone* tz) {` |
| [src/utils/system/time.cpp:59](../../src/utils/system/time.cpp#L59) | wrapper implementation call | `return ::gettimeofday(tv, tz);` |
| [src/utils/system/time.h:64](../../src/utils/system/time.h#L64) | wrapper declaration | `int gettimeofday(struct timeval* tv, struct timezone* tz);` |

## getuid

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Config.cpp:957](../../src/utils/Config.cpp#L957) | inline call | `if ((pw = getpwuid(getuid())) == nullptr) {` |
| [src/utils/Daemon.cpp:203](../../src/utils/Daemon.cpp#L203) | inline call | `if (seteuid(getuid()) != 0) {` |

## gmtime

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/SocketConnection.cpp:192](../../src/core/socket/stream/SocketConnection.cpp#L192) | inline call | `const std::tm* tm_ptr = std::gmtime(&time);` |
| [src/core/socket/stream/SocketContext.cpp:241](../../src/core/socket/stream/SocketContext.cpp#L241) | inline call | `const std::tm* tm_ptr = std::gmtime(&time);` |
| [src/utils/system/time.cpp:62](../../src/utils/system/time.cpp#L62) | wrapper definition | `struct tm* gmtime(const time_t* timep) {` |
| [src/utils/system/time.cpp:64](../../src/utils/system/time.cpp#L64) | wrapper implementation call | `return ::gmtime(timep);` |
| [src/utils/system/time.h:60](../../src/utils/system/time.h#L60) | wrapper declaration | `struct tm* gmtime(const time_t* timep);` |
| [src/web/http/http_utils.cpp:222](../../src/web/http/http_utils.cpp#L222) | extracted caller | `tm = utils::system::gmtime(&now);` |
| [src/web/http/http_utils.cpp:225](../../src/web/http/http_utils.cpp#L225) | extracted caller | `tm = utils::system::gmtime(&time);` |
| [src/web/http/http_utils.cpp:249](../../src/web/http/http_utils.cpp#L249) | inline call | `(void) strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&(attrib.st_mtime))); // TODO: to core::system` |
| [src/web/http/server/Response.cpp:197](../../src/web/http/server/Response.cpp#L197) | extracted caller | `opts["Expires"] = httputils::to_http_date(utils::system::gmtime(&time));` |

## gmtime_r

| Source | Role | Source line |
| --- | --- | --- |
| [src/log/SemanticLogger.cpp:509](../../src/log/SemanticLogger.cpp#L509) | inline call | `gmtime_r(&time, &tm);` |

## htobe16

| Source | Role | Source line |
| --- | --- | --- |
| [src/iot/mqtt/types/UInt16.cpp:59](../../src/iot/mqtt/types/UInt16.cpp#L59) | inline call | `*reinterpret_cast<uint16_t*>(value.data()) = htobe16(newValue);` |
| [src/web/websocket/SocketContextUpgrade.hpp:121](../../src/web/websocket/SocketContextUpgrade.hpp#L121) | inline call | `const uint16_t beStatus = htobe16(statusCode);` |
| [src/web/websocket/Transmitter.cpp:176](../../src/web/websocket/Transmitter.cpp#L176) | inline call | `uint16_t sendData = htobe16(data);` |

## htobe32

| Source | Role | Source line |
| --- | --- | --- |
| [src/iot/mqtt/types/UInt32.cpp:59](../../src/iot/mqtt/types/UInt32.cpp#L59) | inline call | `*reinterpret_cast<uint32_t*>(value.data()) = htobe32(newValue);` |
| [src/web/websocket/Transmitter.cpp:183](../../src/web/websocket/Transmitter.cpp#L183) | inline call | `uint32_t sendData = htobe32(data);` |

## htobe64

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/websocket/Transmitter.cpp:190](../../src/web/websocket/Transmitter.cpp#L190) | inline call | `uint64_t sendData = htobe64(data);` |

## htobs

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/l2/SocketAddress.cpp:82](../../src/net/l2/SocketAddress.cpp#L82) | inline call | `sockAddr.l2_psm = htobs(psm);` |

## htonl

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/http/InetHttpClientConnectFailureTest.cpp:22](../../tests/component/http/InetHttpClientConnectFailureTest.cpp#L22) | inline call | `addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);` |
| [tests/unit/core/ConnectionAttemptLifecycleTest.cpp:14](../../tests/unit/core/ConnectionAttemptLifecycleTest.cpp#L14) | inline call | `addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);` |
| [tests/unit/net/InetSocketAddressNativeTest.cpp:57](../../tests/unit/net/InetSocketAddressNativeTest.cpp#L57) | inline call | `loopbackSockAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);` |
| [tests/unit/net/InetSocketAddressNativeTest.cpp:73](../../tests/unit/net/InetSocketAddressNativeTest.cpp#L73) | inline call | `anySockAddr.sin_addr.s_addr = htonl(INADDR_ANY);` |

## htons

| Source | Role | Source line |
| --- | --- | --- |
| [tests/support/NetworkTest.h:98](../../tests/support/NetworkTest.h#L98) | inline call | `addr.sin_port = htons(port);` |
| [tests/unit/net/Inet6SocketAddressNativeTest.cpp:56](../../tests/unit/net/Inet6SocketAddressNativeTest.cpp#L56) | inline call | `loopbackSockAddr.sin6_port = htons(8080);` |
| [tests/unit/net/Inet6SocketAddressNativeTest.cpp:71](../../tests/unit/net/Inet6SocketAddressNativeTest.cpp#L71) | inline call | `anySockAddr.sin6_port = htons(0);` |
| [tests/unit/net/InetSocketAddressNativeTest.cpp:56](../../tests/unit/net/InetSocketAddressNativeTest.cpp#L56) | inline call | `loopbackSockAddr.sin_port = htons(8080);` |
| [tests/unit/net/InetSocketAddressNativeTest.cpp:72](../../tests/unit/net/InetSocketAddressNativeTest.cpp#L72) | inline call | `anySockAddr.sin_port = htons(0);` |

## inet_ntop

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:40](../../tests/component/net/AddressResolutionPolicyTest.cpp#L40) | inline call | `inet_ntop(AF_INET, &reinterpret_cast<const sockaddr_in*>(address)->sin_addr, host, sizeof(host));` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:42](../../tests/component/net/AddressResolutionPolicyTest.cpp#L42) | inline call | `inet_ntop(AF_INET6, &reinterpret_cast<const sockaddr_in6*>(address)->sin6_addr, host, sizeof(host));` |

## inet_pton

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:192](../../tests/component/net/AddressResolutionPolicyTest.cpp#L192) | inline call | `inet_pton(AF_INET, "127.0.0.2", &occupied.sin_addr);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:264](../../tests/component/net/AddressResolutionPolicyTest.cpp#L264) | inline call | `inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);` |
| [tests/support/NetworkTest.h:99](../../tests/support/NetworkTest.h#L99) | inline call | `inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);` |

## initscr

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:86](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L86) | inline call | `initscr();` |

## isalnum

| Source | Role | Source line |
| --- | --- | --- |
| [src/express/dispatcher/regex_utils.cpp:61](../../src/express/dispatcher/regex_utils.cpp#L61) | inline call | `return (std::isalnum(uc) != 0) &#124;&#124; (c == '_');` |
| [src/tools/snodec-control/src/CommandBuilder.cpp:57](../../src/tools/snodec-control/src/CommandBuilder.cpp#L57) | inline call | `return std::isalnum(static_cast<unsigned char>(character)) != 0 &#124;&#124; character == '.' &#124;&#124; character == '-' &#124;&#124;` |
| [src/tools/snodec-control/src/Materializer.cpp:62](../../src/tools/snodec-control/src/Materializer.cpp#L62) | inline call | `return std::isalnum(static_cast<unsigned char>(character)) != 0 &#124;&#124; character == '.' &#124;&#124; character == '-' &#124;&#124;` |
| [src/utils/base64.cpp:66](../../src/utils/base64.cpp#L66) | inline call | `return ((isalnum(c) != 0) &#124;&#124; (c == '+') &#124;&#124; (c == '/'));` |
| [src/web/http/http_utils.cpp:95](../../src/web/http/http_utils.cpp#L95) | inline call | `if ((isalnum(c) != 0) &#124;&#124; c == '-' &#124;&#124; c == '_' &#124;&#124; c == '.' &#124;&#124; c == '~') {` |

## isalpha

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:646](../../src/utils/CLI11.hpp#L646) | inline call | `inline bool isalpha(const std::string &str) {` |
| [src/utils/CLI11.hpp:647](../../src/utils/CLI11.hpp#L647) | inline call | `return std::all_of(str.begin(), str.end(), [](char c) { return std::isalpha(c, std::locale()); });` |
| [src/utils/CLI11.hpp:4905](../../src/utils/CLI11.hpp#L4905) | inline call | `while(unit_begin > input.begin() && std::isalpha(*(unit_begin - 1), std::locale())) {` |

## isatty

| Source | Role | Source line |
| --- | --- | --- |
| [src/log/detail/SpdlogBackend.cpp:152](../../src/log/detail/SpdlogBackend.cpp#L152) | inline call | `disableColor = ::isatty(::fileno(stdout)) == 0;` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:260](../../src/tools/snodec-control/src/ProcessRunner.cpp#L260) | inline call | `const bool haveTerminal = ::isatty(STDIN_FILENO) == 1;` |

## isblank

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/http/decoder/Fields.cpp:163](../../src/web/http/decoder/Fields.cpp#L163) | inline call | `} else if ((std::isblank(static_cast<unsigned char>(headerFieldName.back())) != 0) &#124;&#124;` |
| [src/web/http/decoder/Fields.cpp:164](../../src/web/http/decoder/Fields.cpp#L164) | inline call | `(std::isblank(static_cast<unsigned char>(headerFieldName.front())) != 0)) {` |

## isdigit

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/Json.cpp:252](../../src/tools/snodec-control/src/Json.cpp#L252) | inline call | `if (text[pos] == '-' &#124;&#124; (std::isdigit(static_cast<unsigned char>(text[pos])) != 0)) {` |
| [src/tools/snodec-control/src/Json.cpp:455](../../src/tools/snodec-control/src/Json.cpp#L455) | inline call | `if (pos >= text.size() &#124;&#124; std::isdigit(static_cast<unsigned char>(text[pos])) == 0) {` |
| [src/tools/snodec-control/src/Json.cpp:459](../../src/tools/snodec-control/src/Json.cpp#L459) | inline call | `while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos])) != 0) {` |
| [src/web/http/client/tools/EventSource.h:143](../../src/web/http/client/tools/EventSource.h#L143) | inline call | `if (std::isdigit(static_cast<unsigned char>(maybeDigit)) == 0) {` |
| [src/web/http/http_utils.cpp:65](../../src/web/http/http_utils.cpp#L65) | inline call | `return isdigit(ch) != 0 ? ch - '0' : tolower(ch) - 'a' + 10;` |
| [tests/unit/log/SemanticLoggerFormattingTest.cpp:42](../../tests/unit/log/SemanticLoggerFormattingTest.cpp#L42) | inline call | `while (j < value.size() && (std::isdigit(static_cast<unsigned char>(value[j])) &#124;&#124; value[j] == ';')) {` |
| [tests/unit/log/SemanticTerminalColorRoutingTest.cpp:34](../../tests/unit/log/SemanticTerminalColorRoutingTest.cpp#L34) | inline call | `while (j < value.size() && (std::isdigit(static_cast<unsigned char>(value[j])) &#124;&#124; value[j] == ';')) {` |

## isinf

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:4170](../../src/utils/CLI11.hpp#L4170) | inline call | `if(std::isinf(c) && !std::isinf(a) && !std::isinf(b)) {` |
| [src/utils/CLI11.hpp:4170](../../src/utils/CLI11.hpp#L4170) | inline call | `if(std::isinf(c) && !std::isinf(a) && !std::isinf(b)) {` |
| [src/utils/CLI11.hpp:4170](../../src/utils/CLI11.hpp#L4170) | inline call | `if(std::isinf(c) && !std::isinf(a) && !std::isinf(b)) {` |

## isprint

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:1168](../../src/utils/CLI11.hpp#L1168) | inline call | `if(isprint(static_cast<unsigned char>(c)) == 0) {` |
| [src/utils/CLI11.hpp:11204](../../src/utils/CLI11.hpp#L11204) | inline call | `return (isprint(static_cast<unsigned char>(x)) != 0 &#124;&#124; x == '\n' &#124;&#124; x == '\t');` |
| [src/utils/CLI11.hpp:11229](../../src/utils/CLI11.hpp#L11229) | inline call | `if(isprint(static_cast<unsigned char>(arg.front())) == 0) {` |

## isspace

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:2652](../../src/utils/CLI11.hpp#L2652) | inline call | `if(std::isspace(static_cast<unsigned char>(input.back()))) {` |
| [src/utils/CLI11.hpp:2714](../../src/utils/CLI11.hpp#L2714) | inline call | `if(std::isspace(static_cast<unsigned char>(input.back()))) {` |
| [src/utils/CLI11.hpp:2848](../../src/utils/CLI11.hpp#L2848) | inline call | `while(std::isspace(static_cast<unsigned char>(*val))) {` |

## isxdigit

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/http/decoder/Chunked.cpp:169](../../src/web/http/decoder/Chunked.cpp#L169) | inline call | `return std::isxdigit(c) != 0;` |

## keypad

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:89](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L89) | inline call | `keypad(stdscr, TRUE);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:477](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L477) | inline call | `keypad(stdscr, TRUE);` |

## kill

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Daemon.cpp:113](../../src/utils/Daemon.cpp#L113) | inline call | `kill(pid, SIGTERM);` |
| [src/utils/Daemon.cpp:180](../../src/utils/Daemon.cpp#L180) | inline call | `if (kill(pid, SIGTERM) != 0) {` |
| [src/utils/Daemon.cpp:194](../../src/utils/Daemon.cpp#L194) | inline call | `kill(pid, SIGKILL);` |
| [tests/component/core/ShutdownReceiverNotificationTest.cpp:98](../../tests/component/core/ShutdownReceiverNotificationTest.cpp#L98) | inline call | `const int killResult = ::kill(::getppid(), SIGINT);` |
| [tests/component/core/ShutdownReceiverNotificationTest.cpp:105](../../tests/component/core/ShutdownReceiverNotificationTest.cpp#L105) | inline call | `static_cast<void>(::kill(::getpid(), SIGTERM));` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:591](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L591) | inline call | `static_cast<void>(::kill(::getpid(), SIGTERM));` |

## listen

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/socket.cpp:62](../../src/core/system/socket.cpp#L62) | wrapper definition | `int listen(int sockfd, int backlog) {` |
| [src/core/system/socket.cpp:64](../../src/core/system/socket.cpp#L64) | wrapper implementation call | `return ::listen(sockfd, backlog);` |
| [src/core/system/socket.h:68](../../src/core/system/socket.h#L68) | wrapper declaration | `int listen(int sockfd, int backlog);` |
| [src/net/phy/stream/PhysicalSocketServer.hpp:54](../../src/net/phy/stream/PhysicalSocketServer.hpp#L54) | extracted caller | `return core::system::listen(Super::getFd(), backlog);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:194](../../tests/component/net/AddressResolutionPolicyTest.cpp#L194) | inline call | `result.expectEqual(0, listen(blocker, 1), "first address cannot be reused by the TLS listener");` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:266](../../tests/component/net/AddressResolutionPolicyTest.cpp#L266) | inline call | `result.expectEqual(0, listen(listener, 16), "raw fixture listens");` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:91](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L91) | inline call | `if (::bind(fd, &address.getSockAddr(), address.getSockAddrLen()) != 0 &#124;&#124; (listen && ::listen(fd, 4) != 0)) {` |

## localtime

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/oauth2/authorization_server/AuthorizationServer.cpp:72](../../src/apps/oauth2/authorization_server/AuthorizationServer.cpp#L72) | inline call | `ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");` |

## lstat

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/phy/PhysicalSocket.hpp:194](../../src/net/un/phy/PhysicalSocket.hpp#L194) | inline call | `if (::lstat(socketPath.c_str(), &socketStatus) == 0) {` |
| [src/net/un/phy/PhysicalSocket.hpp:221](../../src/net/un/phy/PhysicalSocket.hpp#L221) | inline call | `if (::lstat(lockPath.c_str(), &lockStatus) == 0) {` |
| [src/net/un/phy/PhysicalSocket.hpp:278](../../src/net/un/phy/PhysicalSocket.hpp#L278) | inline call | `if (::lstat(lockPath.c_str(), &status) != 0) {` |
| [src/net/un/phy/PhysicalSocket.hpp:379](../../src/net/un/phy/PhysicalSocket.hpp#L379) | inline call | `if (::lstat(socketPath.c_str(), &existingStatus) == 0) {` |
| [src/net/un/phy/PhysicalSocket.hpp:412](../../src/net/un/phy/PhysicalSocket.hpp#L412) | inline call | `if (::lstat(socketPath.c_str(), &unlinkStatus) != 0) {` |
| [src/net/un/phy/PhysicalSocket.hpp:469](../../src/net/un/phy/PhysicalSocket.hpp#L469) | inline call | `const int boundStatResult = ::lstat(socketPath.c_str(), &boundStatus);` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:60](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L60) | inline call | `return ::lstat(path.c_str(), &status) == 0;` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:66](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L66) | inline call | `return ::lstat(path.c_str(), &status) != 0 && errno == ENOENT;` |

## magic_close

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/http/MimeTypes.cpp:251](../../src/web/http/MimeTypes.cpp#L251) | inline call | `magic_close(magic);` |
| [src/web/http/MimeTypes.cpp:259](../../src/web/http/MimeTypes.cpp#L259) | inline call | `magic_close(MimeTypes::magic);` |

## magic_error

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/http/MimeTypes.cpp:250](../../src/web/http/MimeTypes.cpp#L250) | inline call | `<< "Cannot load magic database - " + std::string(magic_error(magic));` |

## magic_file

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/http/MimeTypes.cpp:273](../../src/web/http/MimeTypes.cpp#L273) | inline call | `const char* magicType = magic_file(MimeTypes::magic, file.c_str());` |

## magic_load

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/http/MimeTypes.cpp:248](../../src/web/http/MimeTypes.cpp#L248) | inline call | `if (magic_load(magic, nullptr) != 0) {` |

## magic_open

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/http/MimeTypes.cpp:246](../../src/web/http/MimeTypes.cpp#L246) | inline call | `MimeTypes::magic = magic_open(MAGIC_MIME);` |

## mbsrtowcs

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:411](../../src/utils/CLI11.hpp#L411) | inline call | `std::size_t new_size = std::mbsrtowcs(nullptr, &it, 0, &state);` |
| [src/utils/CLI11.hpp:417](../../src/utils/CLI11.hpp#L417) | inline call | `std::mbsrtowcs(const_cast<wchar_t *>(result.data()), &str, new_size, &state);` |

## memcmp

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/in/SocketAddrInfo.cpp:75](../../src/net/in/SocketAddrInfo.cpp#L75) | inline call | `std::memcmp(currentAddrInfo->ai_addr, currentAddrInfo->ai_next->ai_addr, sizeof(sockaddr_in)) == 0) {` |
| [src/net/un/phy/PhysicalSocket.hpp:108](../../src/net/un/phy/PhysicalSocket.hpp#L108) | inline call | `return std::memcmp(marker.data(), FILESYSTEM_LOCK_MARKER, marker.size()) == 0;` |

## memcpy

| Source | Role | Source line |
| --- | --- | --- |
| [src/log/detail/SpdlogBackend.cpp:110](../../src/log/detail/SpdlogBackend.cpp#L110) | inline call | `std::memcpy(&value, source.data(), sizeof(value));` |
| [src/net/un/SocketAddress.cpp:74](../../src/net/un/SocketAddress.cpp#L74) | inline call | `std::memcpy(sockAddr.sun_path, sunPath.data(), len);` |
| [src/web/http/client/commands/SendFragmentCommand.cpp:58](../../src/web/http/client/commands/SendFragmentCommand.cpp#L58) | inline call | `std::memcpy(this->chunk, chunk, chunkLen);` |
| [src/web/websocket/SocketContextUpgrade.hpp:122](../../src/web/websocket/SocketContextUpgrade.hpp#L122) | inline call | `std::memcpy(closePayload.data(), &beStatus, sizeof(beStatus));` |
| [src/web/websocket/SocketContextUpgrade.hpp:125](../../src/web/websocket/SocketContextUpgrade.hpp#L125) | inline call | `std::memcpy(closePayload.data() + 2, reason, reasonLength);` |
| [tests/unit/http/HttpMessageParserTest.cpp:69](../../tests/unit/http/HttpMessageParserTest.cpp#L69) | inline call | `std::memcpy(chunk, input.data() + offset, toRead);` |
| [tests/unit/http/HttpMessageTargetQueryEdgeTest.cpp:49](../../tests/unit/http/HttpMessageTargetQueryEdgeTest.cpp#L49) | inline call | `std::memcpy(chunk, input.data() + offset, toRead);` |
| [tests/unit/websocket/WebSocketReceiverValidationTest.cpp:39](../../tests/unit/websocket/WebSocketReceiverValidationTest.cpp#L39) | inline call | `std::memcpy(chunk, input.data() + offset, toRead);` |

## memset

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:102](../../src/core/socket/stream/tls/ssl_utils.cpp#L102) | inline call | `memset(u, 0, ::strlen(static_cast<char*>(u))); // garble Password` |
| [src/net/SocketAddress.hpp:55](../../src/net/SocketAddress.hpp#L55) | inline call | `std::memset(&sockAddr, 0, sizeof(sockAddr));` |
| [src/net/in/SocketAddrInfo.cpp:99](../../src/net/in/SocketAddrInfo.cpp#L99) | inline call | `std::memset(hostBfr, 0, NI_MAXHOST);` |
| [src/net/in/SocketAddrInfo.cpp:100](../../src/net/in/SocketAddrInfo.cpp#L100) | inline call | `std::memset(servBfr, 0, NI_MAXSERV);` |
| [src/net/in/SocketAddress.cpp:80](../../src/net/in/SocketAddress.cpp#L80) | inline call | `std::memset(hostC, 0, NI_MAXHOST);` |
| [src/net/in/SocketAddress.cpp:81](../../src/net/in/SocketAddress.cpp#L81) | inline call | `std::memset(servC, 0, NI_MAXSERV);` |
| [src/net/in6/SocketAddrInfo.cpp:92](../../src/net/in6/SocketAddrInfo.cpp#L92) | inline call | `std::memset(hostBfr, 0, NI_MAXHOST);` |
| [src/net/in6/SocketAddrInfo.cpp:93](../../src/net/in6/SocketAddrInfo.cpp#L93) | inline call | `std::memset(servBfr, 0, NI_MAXSERV);` |
| [src/net/in6/SocketAddress.cpp:80](../../src/net/in6/SocketAddress.cpp#L80) | inline call | `std::memset(hostC, 0, NI_MAXHOST);` |
| [src/net/in6/SocketAddress.cpp:81](../../src/net/in6/SocketAddress.cpp#L81) | inline call | `std::memset(servC, 0, NI_MAXSERV);` |

## mkdir

| Source | Role | Source line |
| --- | --- | --- |
| [tests/unit/utils/ConfigDirectoryInitializationTest.cpp:66](../../tests/unit/utils/ConfigDirectoryInitializationTest.cpp#L66) | test interposer definition | `extern "C" int mkdir(const char* path, mode_t mode) noexcept {` |
| [tests/unit/utils/ConfigDirectoryInitializationTest.cpp:67](../../tests/unit/utils/ConfigDirectoryInitializationTest.cpp#L67) | native symbol resolved for indirect call | `static const auto real = reinterpret_cast<decltype(&mkdir)>(dlsym(RTLD_NEXT, "mkdir"));` |

## mkdtemp

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/core/FileReaderTest.cpp:98](../../tests/component/core/FileReaderTest.cpp#L98) | inline call | `char* temporaryDirectoryName = ::mkdtemp(temporaryDirectoryTemplate.data());` |
| [tests/component/core/SNodeCReconfigureTest.cpp:49](../../tests/component/core/SNodeCReconfigureTest.cpp#L49) | inline call | `const char* createdDirectory = ::mkdtemp(directoryTemplate);` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:120](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L120) | inline call | `char* const directoryResult = ::mkdtemp(directoryTemplate.data());` |
| [tests/unit/utils/ConfigDirectoryInitializationTest.cpp:87](../../tests/unit/utils/ConfigDirectoryInitializationTest.cpp#L87) | inline call | `if (mkdtemp(temporary) == nullptr) {` |

## mkstemp

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ConfigActions.cpp:279](../../src/tools/snodec-control/src/ConfigActions.cpp#L279) | inline call | `const int fd = ::mkstemp(buffer.data());` |

## mktime

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/system/time.cpp:67](../../src/utils/system/time.cpp#L67) | wrapper definition | `time_t mktime(struct tm* tm) {` |
| [src/utils/system/time.cpp:69](../../src/utils/system/time.cpp#L69) | wrapper implementation call | `return ::mktime(tm);` |
| [src/utils/system/time.h:61](../../src/utils/system/time.h#L61) | wrapper declaration | `time_t mktime(struct tm* tm);` |
| [src/web/http/http_utils.cpp:224](../../src/web/http/http_utils.cpp#L224) | extracted caller | `const time_t time = utils::system::mktime(tm);` |

## mvhline

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:249](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L249) | inline call | `mvhline(maxY - 1, 0, ' ', maxX);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:603](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L603) | inline call | `mvhline(detailTop, 0, ACS_HLINE, maxX);` |

## mvprintw

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:172](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L172) | inline call | `mvprintw(y++, 0, "%s", fitToWidth(line, maxX).c_str());` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:185](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L185) | inline call | `mvprintw(maxY - 1, 0, "%s", fitToWidth(footer, maxX).c_str());` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:200](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L200) | inline call | `mvprintw(maxY - 1, 0, "%s", fitToWidth(question + " (y/n)", maxX).c_str());` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:252](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L252) | inline call | `mvprintw(maxY - 1, 0, "%s", fitToWidth(visible, maxX).c_str());` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:387](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L387) | inline call | `mvprintw(0, 0, "%s", fitToWidth("No saved config file yet to run with.", maxX).c_str());` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:388](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L388) | inline call | `mvprintw(1,` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:558](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L558) | inline call | `mvprintw(0, 0, "%s", fitToWidth(title, maxX).c_str());` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:593](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L593) | inline call | `mvprintw(1 + row, 0, "%s", fitToWidth(text, maxX).c_str());` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:617](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L617) | inline call | `mvprintw(maxY - 1, 0, "%s", fitToWidth(status, maxX).c_str());` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:749](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L749) | inline call | `mvprintw(maxY - 1,` |

## mysql_affected_rows

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/sync/MariaDBAffectedRowsCommand.cpp:58](../../src/database/mariadb/commands/sync/MariaDBAffectedRowsCommand.cpp#L58) | inline call | `affectedRows = mysql_affected_rows(mysql);` |

## mysql_autocommit_cont

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBAutoCommitCommand.cpp:64](../../src/database/mariadb/commands/async/MariaDBAutoCommitCommand.cpp#L64) | inline call | `return mysql_autocommit_cont(&ret, mysql, status);` |

## mysql_autocommit_start

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBAutoCommitCommand.cpp:60](../../src/database/mariadb/commands/async/MariaDBAutoCommitCommand.cpp#L60) | inline call | `return mysql_autocommit_start(&ret, mysql, autoCommit);` |

## mysql_close

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/MariaDBConnection.cpp:150](../../src/database/mariadb/MariaDBConnection.cpp#L150) | inline call | `mysql_close(mysql);` |

## mysql_commit_cont

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBCommitCommand.cpp:62](../../src/database/mariadb/commands/async/MariaDBCommitCommand.cpp#L62) | inline call | `return mysql_commit_cont(&ret, mysql, status);` |

## mysql_commit_start

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBCommitCommand.cpp:58](../../src/database/mariadb/commands/async/MariaDBCommitCommand.cpp#L58) | inline call | `return mysql_commit_start(&ret, mysql);` |

## mysql_errno

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/MariaDBConnection.cpp:90](../../src/database/mariadb/MariaDBConnection.cpp#L90) | inline call | `if (mysql_errno(mysql) == 0) {` |
| [src/database/mariadb/MariaDBConnection.cpp:138](../../src/database/mariadb/MariaDBConnection.cpp#L138) | inline call | `mariaDBCommand->commandError(mysql_error(mysql), mysql_errno(mysql));` |
| [src/database/mariadb/MariaDBConnection.cpp:173](../../src/database/mariadb/MariaDBConnection.cpp#L173) | inline call | `if (mysql_errno(mysql) == 0) {` |
| [src/database/mariadb/MariaDBConnection.cpp:177](../../src/database/mariadb/MariaDBConnection.cpp#L177) | inline call | `mariaDBCommand->commandError(mysql_error(mysql), mysql_errno(mysql));` |
| [src/database/mariadb/MariaDBConnection.cpp:286](../../src/database/mariadb/MariaDBConnection.cpp#L286) | inline call | `if (mysql_errno(mysql) == 0) {` |
| [src/database/mariadb/MariaDBConnection.cpp:292](../../src/database/mariadb/MariaDBConnection.cpp#L292) | inline call | `currentCommand->commandError(mysql_error(mysql), mysql_errno(mysql));` |
| [src/database/mariadb/MariaDBConnection.cpp:299](../../src/database/mariadb/MariaDBConnection.cpp#L299) | inline call | `currentCommand->commandError(mysql_error(mysql), mysql_errno(mysql));` |

## mysql_error

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/MariaDBConnection.cpp:138](../../src/database/mariadb/MariaDBConnection.cpp#L138) | inline call | `mariaDBCommand->commandError(mysql_error(mysql), mysql_errno(mysql));` |
| [src/database/mariadb/MariaDBConnection.cpp:177](../../src/database/mariadb/MariaDBConnection.cpp#L177) | inline call | `mariaDBCommand->commandError(mysql_error(mysql), mysql_errno(mysql));` |
| [src/database/mariadb/MariaDBConnection.cpp:292](../../src/database/mariadb/MariaDBConnection.cpp#L292) | inline call | `currentCommand->commandError(mysql_error(mysql), mysql_errno(mysql));` |
| [src/database/mariadb/MariaDBConnection.cpp:299](../../src/database/mariadb/MariaDBConnection.cpp#L299) | inline call | `currentCommand->commandError(mysql_error(mysql), mysql_errno(mysql));` |

## mysql_fetch_row_cont

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBFetchRowCommand.cpp:71](../../src/database/mariadb/commands/async/MariaDBFetchRowCommand.cpp#L71) | inline call | `return mysql_fetch_row_cont(&row, result, status);` |

## mysql_fetch_row_start

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBFetchRowCommand.cpp:64](../../src/database/mariadb/commands/async/MariaDBFetchRowCommand.cpp#L64) | inline call | `ret = mysql_fetch_row_start(&row, result);` |

## mysql_field_count

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/sync/MariaDBFieldCountCommand.cpp:58](../../src/database/mariadb/commands/sync/MariaDBFieldCountCommand.cpp#L58) | inline call | `fieldCount = mysql_field_count(mysql);` |

## mysql_free_result

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBFreeResultCommand.cpp:61](../../src/database/mariadb/commands/async/MariaDBFreeResultCommand.cpp#L61) | inline call | `mysql_free_result(result);` |

## mysql_free_result_cont

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBFreeResultCommand.cpp:76](../../src/database/mariadb/commands/async/MariaDBFreeResultCommand.cpp#L76) | inline call | `return mysql_free_result_cont(result, status);` |

## mysql_free_result_start

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBFreeResultCommand.cpp:69](../../src/database/mariadb/commands/async/MariaDBFreeResultCommand.cpp#L69) | inline call | `ret = mysql_free_result_start(result);` |

## mysql_get_socket

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/MariaDBConnection.cpp:91](../../src/database/mariadb/MariaDBConnection.cpp#L91) | inline call | `const int fd = mysql_get_socket(mysql);` |

## mysql_get_timeout_value

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/MariaDBConnection.cpp:276](../../src/database/mariadb/MariaDBConnection.cpp#L276) | inline call | `ReadEventReceiver::setTimeout(mysql_get_timeout_value(mysql));` |

## mysql_init

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/MariaDBConnection.cpp:84](../../src/database/mariadb/MariaDBConnection.cpp#L84) | inline call | `mysql = mysql_init(nullptr);` |

## mysql_library_end

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/MariaDBLibrary.cpp:69](../../src/database/mariadb/MariaDBLibrary.cpp#L69) | inline call | `mysql_library_end();` |

## mysql_library_init

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/MariaDBLibrary.cpp:61](../../src/database/mariadb/MariaDBLibrary.cpp#L61) | inline call | `const int rc = mysql_library_init(0, nullptr, nullptr);` |

## mysql_options

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/MariaDBConnection.cpp:85](../../src/database/mariadb/MariaDBConnection.cpp#L85) | inline call | `mysql_options(mysql, MYSQL_OPT_NONBLOCK, nullptr);` |

## mysql_real_connect_cont

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBConnectCommand.cpp:78](../../src/database/mariadb/commands/async/MariaDBConnectCommand.cpp#L78) | inline call | `return mysql_real_connect_cont(&ret, mysql, status);` |

## mysql_real_connect_start

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBConnectCommand.cpp:62](../../src/database/mariadb/commands/async/MariaDBConnectCommand.cpp#L62) | inline call | `const int status = mysql_real_connect_start(&ret,` |

## mysql_real_query_cont

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBExecCommand.cpp:64](../../src/database/mariadb/commands/async/MariaDBExecCommand.cpp#L64) | inline call | `return mysql_real_query_cont(&ret, mysql, status);` |
| [src/database/mariadb/commands/async/MariaDBQueryCommand.cpp:64](../../src/database/mariadb/commands/async/MariaDBQueryCommand.cpp#L64) | inline call | `return mysql_real_query_cont(&ret, mysql, status);` |

## mysql_real_query_start

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBExecCommand.cpp:60](../../src/database/mariadb/commands/async/MariaDBExecCommand.cpp#L60) | inline call | `return mysql_real_query_start(&ret, mysql, sql.c_str(), sql.length());` |
| [src/database/mariadb/commands/async/MariaDBQueryCommand.cpp:60](../../src/database/mariadb/commands/async/MariaDBQueryCommand.cpp#L60) | inline call | `return mysql_real_query_start(&ret, mysql, sql.c_str(), sql.length());` |

## mysql_rollback_cont

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBRollbackCommand.cpp:62](../../src/database/mariadb/commands/async/MariaDBRollbackCommand.cpp#L62) | inline call | `return mysql_rollback_cont(&ret, mysql, status);` |

## mysql_rollback_start

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/async/MariaDBRollbackCommand.cpp:58](../../src/database/mariadb/commands/async/MariaDBRollbackCommand.cpp#L58) | inline call | `return mysql_rollback_start(&ret, mysql);` |

## mysql_use_result

| Source | Role | Source line |
| --- | --- | --- |
| [src/database/mariadb/commands/sync/MariaDBUseResultCommand.cpp:58](../../src/database/mariadb/commands/sync/MariaDBUseResultCommand.cpp#L58) | inline call | `result = mysql_use_result(mysql);` |

## noecho

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:88](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L88) | inline call | `noecho();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:479](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L479) | inline call | `noecho();` |

## ntohs

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/http/InetHttpClientConnectFailureTest.cpp:34](../../tests/component/http/InetHttpClientConnectFailureTest.cpp#L34) | inline call | `const auto port = ntohs(addr.sin_port);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:80](../../tests/component/net/AddressResolutionPolicyTest.cpp#L80) | inline call | `const int port = address->sa_family == AF_INET6 ? ntohs(reinterpret_cast<const sockaddr_in6*>(address)->sin6_port)` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:81](../../tests/component/net/AddressResolutionPolicyTest.cpp#L81) | inline call | `: (inet ? ntohs(reinterpret_cast<const sockaddr_in*>(address)->sin_port) : 0);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:197](../../tests/component/net/AddressResolutionPolicyTest.cpp#L197) | inline call | `testPort = ntohs(occupied.sin_port);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:269](../../tests/component/net/AddressResolutionPolicyTest.cpp#L269) | inline call | `testPort = ntohs(address.sin_port);` |
| [tests/unit/core/ConnectionAttemptLifecycleTest.cpp:18](../../tests/unit/core/ConnectionAttemptLifecycleTest.cpp#L18) | inline call | `const unsigned short port = ntohs(addr.sin_port);` |

## open

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/file/FileReader.cpp:128](../../src/core/file/FileReader.cpp#L128) | extracted caller | `DescriptorGuard descriptor(core::system::open(path.c_str(), O_RDONLY));` |
| [src/core/system/unistd.cpp:52](../../src/core/system/unistd.cpp#L52) | wrapper definition | `int open(const char* pathname, int flags) {` |
| [src/core/system/unistd.cpp:54](../../src/core/system/unistd.cpp#L54) | wrapper implementation call | `return ::open(pathname, flags);` |
| [src/core/system/unistd.h:63](../../src/core/system/unistd.h#L63) | wrapper declaration | `int open(const char* pathname, int flags);` |
| [src/net/un/phy/PhysicalSocket.hpp:297](../../src/net/un/phy/PhysicalSocket.hpp#L297) | inline call | `lockFd = ::open(lockPath.c_str(), O_RDWR &#124; O_CREAT &#124; O_EXCL &#124; O_CLOEXEC &#124; O_NOFOLLOW &#124; O_NONBLOCK, 0600);` |
| [src/net/un/phy/PhysicalSocket.hpp:303](../../src/net/un/phy/PhysicalSocket.hpp#L303) | inline call | `lockFd = ::open(lockPath.c_str(), O_RDWR &#124; O_CLOEXEC &#124; O_NOFOLLOW &#124; O_NONBLOCK);` |
| [src/web/websocket/client/SocketContextUpgradeFactory.cpp:79](../../src/web/websocket/client/SocketContextUpgradeFactory.cpp#L79) | inline call | `const int fd = open("/dev/urandom", O_RDONLY);` |
| [tests/component/core/FileReaderTest.cpp:109](../../tests/component/core/FileReaderTest.cpp#L109) | inline call | `const int writeFd = ::open(filePath.c_str(), O_WRONLY &#124; O_CREAT &#124; O_TRUNC &#124; O_CLOEXEC, 0600);` |
| [tests/component/core/FileReaderTest.cpp:121](../../tests/component/core/FileReaderTest.cpp#L121) | inline call | `const int directoryFd = ::open(temporaryDirectory.c_str(), O_RDONLY &#124; O_DIRECTORY &#124; O_CLOEXEC);` |
| [tests/component/core/FileReaderTest.cpp:128](../../tests/component/core/FileReaderTest.cpp#L128) | inline call | `const int savedWorkingDirectory = ::open(".", O_RDONLY &#124; O_DIRECTORY &#124; O_CLOEXEC);` |
| [tests/component/core/FileReaderTest.cpp:137](../../tests/component/core/FileReaderTest.cpp#L137) | inline call | `const int adoptedFd = ::open(filePath.c_str(), O_RDONLY &#124; O_CLOEXEC);` |
| [tests/component/core/FileReaderTest.cpp:187](../../tests/component/core/FileReaderTest.cpp#L187) | inline call | `const int stoppedFd = ::open(filePath.c_str(), O_RDONLY &#124; O_CLOEXEC);` |

## openat

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/file/FileReader.cpp:153](../../src/core/file/FileReader.cpp#L153) | extracted caller | `DescriptorGuard descriptor(core::system::openat(directoryFd, pathname.c_str(), flags));` |
| [src/core/system/unistd.cpp:57](../../src/core/system/unistd.cpp#L57) | wrapper definition | `int openat(int directoryFd, const char* pathname, int flags) {` |
| [src/core/system/unistd.cpp:59](../../src/core/system/unistd.cpp#L59) | wrapper implementation call | `return ::openat(directoryFd, pathname, flags);` |
| [src/core/system/unistd.h:64](../../src/core/system/unistd.h#L64) | wrapper declaration | `int openat(int directoryFd, const char* pathname, int flags);` |

## pipe

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:71](../../src/tools/snodec-control/src/ProcessRunner.cpp#L71) | inline call | `if (::pipe(fds.data()) != 0) {` |

## pipe2

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/pipe/Pipe.cpp:109](../../src/core/pipe/Pipe.cpp#L109) | extracted caller | `if (core::system::pipe2(descriptors, flags) != 0) {` |
| [src/core/system/unistd.cpp:77](../../src/core/system/unistd.cpp#L77) | wrapper definition | `int pipe2(int pipefd[2], int flags) {` |
| [src/core/system/unistd.cpp:79](../../src/core/system/unistd.cpp#L79) | wrapper implementation call | `return ::pipe2(pipefd, flags);` |
| [src/core/system/unistd.h:70](../../src/core/system/unistd.h#L70) | wrapper declaration | `int pipe2(int pipefd[2], int flags);` |

## poll

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/poll.cpp:54](../../src/core/system/poll.cpp#L54) | wrapper definition | `int poll(pollfd* fds, nfds_t nfds, int timeout) {` |
| [src/core/system/poll.cpp:56](../../src/core/system/poll.cpp#L56) | wrapper implementation call | `return ::poll(fds, nfds, timeout);` |
| [src/core/system/poll.h:60](../../src/core/system/poll.h#L60) | wrapper declaration | `int poll(struct pollfd* fds, nfds_t nfds, int timeout);` |
| [src/utils/Daemon.cpp:187](../../src/utils/Daemon.cpp#L187) | inline call | `const int ready = poll(&pollfd, 1, 5000);` |

## posix_spawn

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:198](../../src/tools/snodec-control/src/ProcessRunner.cpp#L198) | inline call | `const int spawnRc = posix_spawn(&pid, executable.c_str(), &fileActions, nullptr, argv.data(), environ);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:235](../../src/tools/snodec-control/src/ProcessRunner.cpp#L235) | inline call | `const int spawnRc = posix_spawn(&pid, executable.c_str(), nullptr, nullptr, argv.data(), environ);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:301](../../src/tools/snodec-control/src/ProcessRunner.cpp#L301) | inline call | `const int spawnRc = posix_spawn(&pid, executable.c_str(), nullptr, &attr, argv.data(), environ);` |

## posix_spawn_file_actions_addclose

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:190](../../src/tools/snodec-control/src/ProcessRunner.cpp#L190) | inline call | `posix_spawn_file_actions_addclose(&fileActions, outPipe.readEnd);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:191](../../src/tools/snodec-control/src/ProcessRunner.cpp#L191) | inline call | `posix_spawn_file_actions_addclose(&fileActions, errPipe.readEnd);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:192](../../src/tools/snodec-control/src/ProcessRunner.cpp#L192) | inline call | `posix_spawn_file_actions_addclose(&fileActions, outPipe.writeEnd);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:193](../../src/tools/snodec-control/src/ProcessRunner.cpp#L193) | inline call | `posix_spawn_file_actions_addclose(&fileActions, errPipe.writeEnd);` |

## posix_spawn_file_actions_adddup2

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:188](../../src/tools/snodec-control/src/ProcessRunner.cpp#L188) | inline call | `posix_spawn_file_actions_adddup2(&fileActions, outPipe.writeEnd, STDOUT_FILENO);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:189](../../src/tools/snodec-control/src/ProcessRunner.cpp#L189) | inline call | `posix_spawn_file_actions_adddup2(&fileActions, errPipe.writeEnd, STDERR_FILENO);` |

## posix_spawn_file_actions_destroy

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:200](../../src/tools/snodec-control/src/ProcessRunner.cpp#L200) | inline call | `posix_spawn_file_actions_destroy(&fileActions);` |

## posix_spawn_file_actions_init

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:187](../../src/tools/snodec-control/src/ProcessRunner.cpp#L187) | inline call | `posix_spawn_file_actions_init(&fileActions);` |

## posix_spawnattr_destroy

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:303](../../src/tools/snodec-control/src/ProcessRunner.cpp#L303) | inline call | `posix_spawnattr_destroy(&attr);` |

## posix_spawnattr_init

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:272](../../src/tools/snodec-control/src/ProcessRunner.cpp#L272) | inline call | `posix_spawnattr_init(&attr);` |

## posix_spawnattr_setflags

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:298](../../src/tools/snodec-control/src/ProcessRunner.cpp#L298) | inline call | `posix_spawnattr_setflags(&attr, flags);` |

## posix_spawnattr_setpgroup

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:279](../../src/tools/snodec-control/src/ProcessRunner.cpp#L279) | inline call | `posix_spawnattr_setpgroup(&attr, 0);` |

## posix_spawnattr_setsigdefault

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:290](../../src/tools/snodec-control/src/ProcessRunner.cpp#L290) | inline call | `posix_spawnattr_setsigdefault(&attr, &defaultSignals);` |

## posix_spawnattr_setsigmask

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:295](../../src/tools/snodec-control/src/ProcessRunner.cpp#L295) | inline call | `posix_spawnattr_setsigmask(&attr, &emptyMask);` |

## ppoll

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/multiplexer/poll/EventMultiplexer.cpp:205](../../src/core/multiplexer/poll/EventMultiplexer.cpp#L205) | extracted caller | `return core::system::ppoll(pollFdsManager.getEvents(), pollFdsManager.getCurrentSize(), &timeSpec, &sigMask);` |
| [src/core/system/poll.cpp:59](../../src/core/system/poll.cpp#L59) | wrapper definition | `int ppoll(struct pollfd* fds, nfds_t nfds, const timespec* timeout, const sigset_t* sigMask) {` |
| [src/core/system/poll.cpp:61](../../src/core/system/poll.cpp#L61) | wrapper implementation call | `return ::ppoll(fds, nfds, timeout, sigMask);` |
| [src/core/system/poll.h:61](../../src/core/system/poll.h#L61) | wrapper declaration | `int ppoll(struct pollfd* fds, nfds_t nfds, const timespec* timeout, const sigset_t* sigMask);` |

## pread

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/phy/PhysicalSocket.hpp:98](../../src/net/un/phy/PhysicalSocket.hpp#L98) | inline call | `const ssize_t readResult = ::pread(fd, marker.data() + read, marker.size() - read, static_cast<off_t>(read));` |

## pselect

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/multiplexer/select/EventMultiplexer.cpp:90](../../src/core/multiplexer/select/EventMultiplexer.cpp#L90) | extracted caller | `return core::system::pselect(maxFd() + 1,` |
| [src/core/system/select.cpp:57](../../src/core/system/select.cpp#L57) | wrapper definition | `int pselect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const struct timespec* timeout, const sigset_t* sigMask) {` |
| [src/core/system/select.cpp:59](../../src/core/system/select.cpp#L59) | wrapper implementation call | `return ::pselect(nfds, readfds, writefds, exceptfds, timeout, sigMask);` |
| [src/core/system/select.h:62](../../src/core/system/select.h#L62) | wrapper declaration | `int pselect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const timespec* timeout, const sigset_t* sigMask);` |

## pthread_sigmask

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/EventLoop.cpp:222](../../src/core/EventLoop.cpp#L222) | inline call | `const bool signalMaskBlocked = logPthreadSigmaskFailure(pthread_sigmask(SIG_BLOCK, &newSet, &oldSet), "tick-block");` |
| [src/core/EventLoop.cpp:238](../../src/core/EventLoop.cpp#L238) | inline call | `logPthreadSigmaskFailure(pthread_sigmask(SIG_SETMASK, &oldSet, nullptr), "tick-restore");` |
| [src/log/detail/SpdlogBackend.cpp:48](../../src/log/detail/SpdlogBackend.cpp#L48) | inline call | `const int error = ::pthread_sigmask(SIG_BLOCK, &blockedSignals, &previousMask);` |
| [src/log/detail/SpdlogBackend.cpp:55](../../src/log/detail/SpdlogBackend.cpp#L55) | inline call | `static_cast<void>(::pthread_sigmask(SIG_SETMASK, &previousMask, nullptr));` |

## pwrite

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/phy/PhysicalSocket.hpp:73](../../src/net/un/phy/PhysicalSocket.hpp#L73) | inline call | `::pwrite(fd, FILESYSTEM_LOCK_MARKER + written, FILESYSTEM_LOCK_MARKER_SIZE - written, static_cast<off_t>(written));` |

## read

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/towercalculator/KeyboardReader.cpp:80](../../src/apps/towercalculator/KeyboardReader.cpp#L80) | inline call | `const ssize_t ret = read(STDIN_FILENO, buffer, 256);` |
| [src/core/file/FileReader.cpp:191](../../src/core/file/FileReader.cpp#L191) | extracted caller | `const ssize_t ret = core::system::read(getFd(), puffer.data(), puffer.capacity());` |
| [src/core/pipe/PipeSink.cpp:82](../../src/core/pipe/PipeSink.cpp#L82) | extracted caller | `const ssize_t ret = core::system::read(getRegisteredFd(), chunk.data(), readSize);` |
| [src/core/system/unistd.cpp:62](../../src/core/system/unistd.cpp#L62) | wrapper definition | `ssize_t read(int fd, void* buf, std::size_t count) {` |
| [src/core/system/unistd.cpp:64](../../src/core/system/unistd.cpp#L64) | wrapper implementation call | `return ::read(fd, buf, count);` |
| [src/core/system/unistd.h:67](../../src/core/system/unistd.h#L67) | wrapper declaration | `ssize_t read(int fd, void* buf, std::size_t count);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:112](../../src/tools/snodec-control/src/ProcessRunner.cpp#L112) | inline call | `const ssize_t bytesRead = ::read(outPipe.readEnd, buffer.data(), buffer.size());` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:123](../../src/tools/snodec-control/src/ProcessRunner.cpp#L123) | inline call | `const ssize_t bytesRead = ::read(errPipe.readEnd, buffer.data(), buffer.size());` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:353](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L353) | inline call | `const ssize_t readResult = ::read(STDIN_FILENO, &discard, 1);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:365](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L365) | inline call | `const ssize_t readResult = ::read(STDIN_FILENO, &discard, 1);` |
| [src/utils/Config.cpp:797](../../src/utils/Config.cpp#L797) | inline call | `while (read(STDIN_FILENO, buf, 1024) > 0) {` |
| [src/web/websocket/client/SocketContextUpgradeFactory.cpp:83](../../src/web/websocket/client/SocketContextUpgradeFactory.cpp#L83) | inline call | `const ssize_t ret = read(fd, buf, buflen);` |
| [tests/component/core/PipeImmediateCloseTest.cpp:89](../../tests/component/core/PipeImmediateCloseTest.cpp#L89) | inline call | `const ssize_t readResult = ::read(readFd, &byte, 1);` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:253](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L253) | inline call | `return ::read(getRegisteredFd(), chunk, chunkLen);` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:430](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L430) | inline call | `const ssize_t count = ::read(fd, bytes, sizeof(bytes));` |

## recv

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/SocketReader.cpp:88](../../src/core/socket/stream/SocketReader.cpp#L88) | extracted caller | `return core::system::recv(this->getRegisteredFd(), chunk, chunkLen, 0);` |
| [src/core/system/socket.cpp:91](../../src/core/system/socket.cpp#L91) | wrapper definition | `ssize_t recv(int sockfd, void* buf, std::size_t len, int flags) {` |
| [src/core/system/socket.cpp:93](../../src/core/system/socket.cpp#L93) | wrapper implementation call | `return ::recv(sockfd, buf, len, flags);` |
| [src/core/system/socket.h:72](../../src/core/system/socket.h#L72) | wrapper declaration | `ssize_t recv(int sockfd, void* buf, std::size_t len, int flags);` |
| [tests/support/NetworkTest.h:132](../../tests/support/NetworkTest.h#L132) | inline call | `return ssl ? SSL_read(ssl, bytes, size) : static_cast<int>(::recv(fd, bytes, static_cast<std::size_t>(size), 0));` |

## recvmsg

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/dgram/Socket.cpp:120](../../src/net/un/dgram/Socket.cpp#L120) | inline call | `if ((n = recvmsg(core::Descriptor::getFd(), &msg, 0)) > 0) {` |

## refresh

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:187](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L187) | inline call | `refresh();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:202](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L202) | inline call | `refresh();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:254](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L254) | inline call | `refresh();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:392](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L392) | inline call | `refresh();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:476](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L476) | inline call | `refresh();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:620](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L620) | inline call | `refresh();` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:754](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L754) | inline call | `refresh();` |

## reset_prog_mode

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:475](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L475) | inline call | `reset_prog_mode();` |

## rmdir

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:330](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L330) | inline call | `testResult.expectEqual(0, ::rmdir(directory.c_str()), "temporary Unix socket test directory is removed");` |

## select

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/select.cpp:52](../../src/core/system/select.cpp#L52) | wrapper definition | `int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout) {` |
| [src/core/system/select.cpp:54](../../src/core/system/select.cpp#L54) | wrapper implementation call | `return ::select(nfds, readfds, writefds, exceptfds, timeout);` |
| [src/core/system/select.h:61](../../src/core/system/select.h#L61) | wrapper declaration | `int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:103](../../src/tools/snodec-control/src/ProcessRunner.cpp#L103) | inline call | `const int selected = ::select(maxFd + 1, &readSet, nullptr, nullptr, nullptr);` |

## send

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/SocketWriter.cpp:105](../../src/core/socket/stream/SocketWriter.cpp#L105) | extracted caller | `return core::system::send(this->getRegisteredFd(), chunk, chunkLen, MSG_NOSIGNAL);` |
| [src/core/system/socket.cpp:96](../../src/core/system/socket.cpp#L96) | wrapper definition | `ssize_t send(int sockfd, const void* buf, std::size_t len, int flags) {` |
| [src/core/system/socket.cpp:98](../../src/core/system/socket.cpp#L98) | wrapper implementation call | `return ::send(sockfd, buf, len, flags);` |
| [src/core/system/socket.h:73](../../src/core/system/socket.h#L73) | wrapper declaration | `ssize_t send(int sockfd, const void* buf, std::size_t len, int flags);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:95](../../tests/component/net/AddressResolutionPolicyTest.cpp#L95) | inline call | `while (send(pair[0], data, sizeof(data), MSG_NOSIGNAL) > 0) {` |
| [tests/support/NetworkTest.h:129](../../tests/support/NetworkTest.h#L129) | inline call | `: static_cast<int>(::send(fd, bytes.data(), bytes.size(), MSG_NOSIGNAL));` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:44](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L44) | inline call | `require(::send(peer.fd, "not TLS at all\r\n", 16, MSG_NOSIGNAL) == 16, "invalid peer input");` |

## sendmsg

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/dgram/Socket.cpp:94](../../src/net/un/dgram/Socket.cpp#L94) | inline call | `return sendmsg(core::Descriptor::getFd(), &msg, 0);` |

## setegid

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Daemon.cpp:131](../../src/utils/Daemon.cpp#L131) | inline call | `if (setegid(gr->gr_gid) != 0) {` |
| [src/utils/Daemon.cpp:206](../../src/utils/Daemon.cpp#L206) | inline call | `if (setegid(getgid()) != 0) {` |

## setenv

| Source | Role | Source line |
| --- | --- | --- |
| [tests/unit/utils/ConfigDirectoryInitializationTest.cpp:95](../../tests/unit/utils/ConfigDirectoryInitializationTest.cpp#L95) | inline call | `setenv(variables[index], home.c_str(), 1);` |

## seteuid

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Daemon.cpp:140](../../src/utils/Daemon.cpp#L140) | inline call | `if (seteuid(pw->pw_uid) != 0) {` |
| [src/utils/Daemon.cpp:203](../../src/utils/Daemon.cpp#L203) | inline call | `if (seteuid(getuid()) != 0) {` |

## setlocale

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:339](../../src/utils/CLI11.hpp#L339) | inline call | `if(std::setlocale(LC_ALL, locale_name) != nullptr) {` |
| [src/utils/CLI11.hpp:376](../../src/utils/CLI11.hpp#L376) | inline call | `std::string old_locale = std::setlocale(LC_ALL, nullptr);` |
| [src/utils/CLI11.hpp:377](../../src/utils/CLI11.hpp#L377) | inline call | `auto sg = scope_guard([&] { std::setlocale(LC_ALL, old_locale.c_str()); });` |
| [src/utils/CLI11.hpp:407](../../src/utils/CLI11.hpp#L407) | inline call | `std::string old_locale = std::setlocale(LC_ALL, nullptr);` |
| [src/utils/CLI11.hpp:408](../../src/utils/CLI11.hpp#L408) | inline call | `auto sg = scope_guard([&] { std::setlocale(LC_ALL, old_locale.c_str()); });` |

## setsid

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Daemon.cpp:92](../../src/utils/Daemon.cpp#L92) | inline call | `if (setsid() < 0) {` |

## setsockopt

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/socket.cpp:106](../../src/core/system/socket.cpp#L106) | wrapper definition | `int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {` |
| [src/core/system/socket.cpp:108](../../src/core/system/socket.cpp#L108) | wrapper implementation call | `return ::setsockopt(sockfd, level, optname, optval, optlen);` |
| [src/core/system/socket.h:75](../../src/core/system/socket.h#L75) | wrapper declaration | `int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);` |
| [src/net/phy/PhysicalSocket.hpp:141](../../src/net/phy/PhysicalSocket.hpp#L141) | extracted caller | `return core::system::setsockopt(PhysicalSocket::getFd(), level, optname, optval, optlen);` |
| [tests/support/NetworkTest.h:122](../../tests/support/NetworkTest.h#L122) | inline call | `setsockopt(fd, SOL_SOCKET, SO_LINGER, &value, sizeof(value));` |

## shutdown

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/socket.cpp:111](../../src/core/system/socket.cpp#L111) | wrapper definition | `int shutdown(int sockfd, int how) {` |
| [src/core/system/socket.cpp:113](../../src/core/system/socket.cpp#L113) | wrapper implementation call | `return ::shutdown(sockfd, how);` |
| [src/core/system/socket.h:63](../../src/core/system/socket.h#L63) | wrapper declaration | `int shutdown(int sockfd, int how);` |
| [src/net/phy/stream/PhysicalSocket.hpp:52](../../src/net/phy/stream/PhysicalSocket.hpp#L52) | extracted caller | `return core::system::shutdown(core::Descriptor::getFd(), how);` |

## sigabbrev_np

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/websocket/subprotocol/client/echo/Echo.cpp:105](../../src/apps/websocket/subprotocol/client/echo/Echo.cpp#L105) | extracted caller | `<< utils::system::sigabbrev_np(sig) << " = " << sig << ")";` |
| [src/apps/websocket/subprotocol/server/echo/Echo.cpp:106](../../src/apps/websocket/subprotocol/server/echo/Echo.cpp#L106) | extracted caller | `<< utils::system::sigabbrev_np(sig) << " = " << sig << ")";` |
| [src/core/EventLoop.cpp:70](../../src/core/EventLoop.cpp#L70) | extracted caller | `std::string signal = "SIG" + utils::system::sigabbrev_np(signum);` |
| [src/core/EventLoop.cpp:232](../../src/core/EventLoop.cpp#L232) | extracted caller | `utils::system::sigabbrev_np(signalNumber),` |
| [src/core/EventLoop.cpp:363](../../src/core/EventLoop.cpp#L363) | extracted caller | `std::string signal = "SIG" + utils::system::sigabbrev_np(stopsig);` |
| [src/core/socket/stream/SocketConnection.hpp:405](../../src/core/socket/stream/SocketConnection.hpp#L405) | extracted caller | `utils::system::sigabbrev_np(signum),` |
| [src/iot/mqtt/SubProtocol.hpp:162](../../src/iot/mqtt/SubProtocol.hpp#L162) | extracted caller | `<< "exit due to signal SIG" << utils::system::sigabbrev_np(sig) << " (" << sig << ")";` |
| [src/utils/system/signal.cpp:59](../../src/utils/system/signal.cpp#L59) | wrapper definition | `std::string sigabbrev_np(int sig) {` |
| [src/utils/system/signal.h:60](../../src/utils/system/signal.h#L60) | wrapper declaration | `std::string sigabbrev_np(int sig);` |

## sigaction

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/EventLoop.cpp:178](../../src/core/EventLoop.cpp#L178) | inline call | `logSigactionFailure(sigaction(SIGPIPE, &sact, &oldPipeAct), SIGPIPE, "init-install-ignore");` |
| [src/core/EventLoop.cpp:181](../../src/core/EventLoop.cpp#L181) | inline call | `logSigactionFailure(sigaction(SIGINT, &sact, &oldIntAct), SIGINT, "init-install-ignore");` |
| [src/core/EventLoop.cpp:184](../../src/core/EventLoop.cpp#L184) | inline call | `logSigactionFailure(sigaction(SIGTERM, &sact, &oldTermAct), SIGTERM, "init-install-ignore");` |
| [src/core/EventLoop.cpp:187](../../src/core/EventLoop.cpp#L187) | inline call | `logSigactionFailure(sigaction(SIGALRM, &sact, &oldAlarmAct), SIGALRM, "init-install-ignore");` |
| [src/core/EventLoop.cpp:190](../../src/core/EventLoop.cpp#L190) | inline call | `logSigactionFailure(sigaction(SIGHUP, &sact, &oldHupAct), SIGHUP, "init-install-ignore");` |
| [src/core/EventLoop.cpp:200](../../src/core/EventLoop.cpp#L200) | inline call | `logSigactionFailure(sigaction(SIGPIPE, &oldPipeAct, nullptr), SIGPIPE, "init-restore");` |
| [src/core/EventLoop.cpp:201](../../src/core/EventLoop.cpp#L201) | inline call | `logSigactionFailure(sigaction(SIGINT, &oldIntAct, nullptr), SIGINT, "init-restore");` |
| [src/core/EventLoop.cpp:202](../../src/core/EventLoop.cpp#L202) | inline call | `logSigactionFailure(sigaction(SIGTERM, &oldTermAct, nullptr), SIGTERM, "init-restore");` |
| [src/core/EventLoop.cpp:203](../../src/core/EventLoop.cpp#L203) | inline call | `logSigactionFailure(sigaction(SIGALRM, &oldAlarmAct, nullptr), SIGALRM, "init-restore");` |
| [src/core/EventLoop.cpp:204](../../src/core/EventLoop.cpp#L204) | inline call | `logSigactionFailure(sigaction(SIGHUP, &oldHupAct, nullptr), SIGHUP, "init-restore");` |
| [src/core/EventLoop.cpp:254](../../src/core/EventLoop.cpp#L254) | inline call | `logSigactionFailure(sigaction(SIGPIPE, &sact, &oldPipeAct), SIGPIPE, "tick-install-ignore");` |
| [src/core/EventLoop.cpp:258](../../src/core/EventLoop.cpp#L258) | inline call | `logSigactionFailure(sigaction(SIGPIPE, &oldPipeAct, nullptr), SIGPIPE, "tick-restore");` |
| [src/core/EventLoop.cpp:280](../../src/core/EventLoop.cpp#L280) | inline call | `logSigactionFailure(sigaction(SIGPIPE, &sact, &oldPipeAct), SIGPIPE, "start-install-ignore");` |
| [src/core/EventLoop.cpp:285](../../src/core/EventLoop.cpp#L285) | inline call | `logSigactionFailure(sigaction(SIGINT, &sact, &oldIntAct), SIGINT, "start-install-handler");` |
| [src/core/EventLoop.cpp:288](../../src/core/EventLoop.cpp#L288) | inline call | `logSigactionFailure(sigaction(SIGTERM, &sact, &oldTermAct), SIGTERM, "start-install-handler");` |
| [src/core/EventLoop.cpp:291](../../src/core/EventLoop.cpp#L291) | inline call | `logSigactionFailure(sigaction(SIGALRM, &sact, &oldAlarmAct), SIGALRM, "start-install-handler");` |
| [src/core/EventLoop.cpp:294](../../src/core/EventLoop.cpp#L294) | inline call | `logSigactionFailure(sigaction(SIGHUP, &sact, &oldHupAct), SIGHUP, "start-install-handler");` |
| [src/core/EventLoop.cpp:331](../../src/core/EventLoop.cpp#L331) | inline call | `logSigactionFailure(sigaction(SIGPIPE, &oldPipeAct, nullptr), SIGPIPE, "start-restore");` |
| [src/core/EventLoop.cpp:332](../../src/core/EventLoop.cpp#L332) | inline call | `logSigactionFailure(sigaction(SIGTERM, &oldTermAct, nullptr), SIGTERM, "start-restore");` |
| [src/core/EventLoop.cpp:333](../../src/core/EventLoop.cpp#L333) | inline call | `logSigactionFailure(sigaction(SIGALRM, &oldAlarmAct, nullptr), SIGALRM, "start-restore");` |
| [src/core/EventLoop.cpp:334](../../src/core/EventLoop.cpp#L334) | inline call | `logSigactionFailure(sigaction(SIGHUP, &oldHupAct, nullptr), SIGHUP, "start-restore");` |
| [src/core/EventLoop.cpp:338](../../src/core/EventLoop.cpp#L338) | inline call | `logSigactionFailure(sigaction(SIGINT, &oldIntAct, nullptr), SIGINT, "start-restore");` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:164](../../src/tools/snodec-control/src/ProcessRunner.cpp#L164) | inline call | `::sigaction(jobControlSignals[i], &ignoreAction, &previous[i]);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:170](../../src/tools/snodec-control/src/ProcessRunner.cpp#L170) | inline call | `::sigaction(jobControlSignals[i], &previous[i], nullptr);` |

## sigaddset

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/EventLoop.cpp:216](../../src/core/EventLoop.cpp#L216) | inline call | `logSigaddsetFailure(sigaddset(&newSet, SIGINT), SIGINT, "tick-block-mask");` |
| [src/core/EventLoop.cpp:217](../../src/core/EventLoop.cpp#L217) | inline call | `logSigaddsetFailure(sigaddset(&newSet, SIGTERM), SIGTERM, "tick-block-mask");` |
| [src/core/EventLoop.cpp:218](../../src/core/EventLoop.cpp#L218) | inline call | `logSigaddsetFailure(sigaddset(&newSet, SIGALRM), SIGALRM, "tick-block-mask");` |
| [src/core/EventLoop.cpp:219](../../src/core/EventLoop.cpp#L219) | inline call | `logSigaddsetFailure(sigaddset(&newSet, SIGHUP), SIGHUP, "tick-block-mask");` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:288](../../src/tools/snodec-control/src/ProcessRunner.cpp#L288) | inline call | `sigaddset(&defaultSignals, signalNumber);` |

## sigemptyset

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/EventLoop.cpp:173](../../src/core/EventLoop.cpp#L173) | inline call | `logSignalFailure(sigemptyset(&sact.sa_mask), "sigemptyset", "init-install-mask");` |
| [src/core/EventLoop.cpp:215](../../src/core/EventLoop.cpp#L215) | inline call | `logSignalFailure(sigemptyset(&newSet), "sigemptyset", "tick-block-mask");` |
| [src/core/EventLoop.cpp:249](../../src/core/EventLoop.cpp#L249) | inline call | `logSignalFailure(sigemptyset(&sact.sa_mask), "sigemptyset", "tick-install-mask");` |
| [src/core/EventLoop.cpp:275](../../src/core/EventLoop.cpp#L275) | inline call | `logSignalFailure(sigemptyset(&sact.sa_mask), "sigemptyset", "start-install-mask");` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:161](../../src/tools/snodec-control/src/ProcessRunner.cpp#L161) | inline call | `sigemptyset(&ignoreAction.sa_mask);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:286](../../src/tools/snodec-control/src/ProcessRunner.cpp#L286) | inline call | `sigemptyset(&defaultSignals);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:294](../../src/tools/snodec-control/src/ProcessRunner.cpp#L294) | inline call | `sigemptyset(&emptyMask);` |
| [tests/unit/core/SocketWriterResourcePolicyTest.cpp:230](../../tests/unit/core/SocketWriterResourcePolicyTest.cpp#L230) | inline call | `sigemptyset(&sigmask);` |

## sigfillset

| Source | Role | Source line |
| --- | --- | --- |
| [src/log/detail/SpdlogBackend.cpp:44](../../src/log/detail/SpdlogBackend.cpp#L44) | inline call | `if (::sigfillset(&blockedSignals) != 0) {` |

## signal

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Daemon.cpp:97](../../src/utils/Daemon.cpp#L97) | inline call | `if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {` |
| [src/utils/system/signal.cpp:54](../../src/utils/system/signal.cpp#L54) | wrapper definition | `sighandler_t signal(int sig, sighandler_t handler) {` |
| [src/utils/system/signal.cpp:56](../../src/utils/system/signal.cpp#L56) | wrapper implementation call | `return std::signal(sig, handler);` |
| [src/utils/system/signal.h:58](../../src/utils/system/signal.h#L58) | wrapper declaration | `sighandler_t signal(int sig, sighandler_t handler);` |

## sk_GENERAL_NAME_num

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:137](../../src/apps/echo/model/clients.h#L137) | inline call | `const int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);` |
| [src/apps/echo/model/servers.h:137](../../src/apps/echo/model/servers.h#L137) | inline call | `int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);` |
| [src/apps/http/httplowlevelclient.cpp:193](../../src/apps/http/httplowlevelclient.cpp#L193) | inline call | `const int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);` |
| [src/apps/http/model/clients.h:723](../../src/apps/http/model/clients.h#L723) | inline call | `int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);` |
| [src/apps/testregex.cpp:390](../../src/apps/testregex.cpp#L390) | inline call | `const int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);` |
| [src/core/socket/stream/tls/ssl_utils.cpp:265](../../src/core/socket/stream/tls/ssl_utils.cpp#L265) | inline call | `const int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);` |

## sk_GENERAL_NAME_pop_free

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:157](../../src/apps/echo/model/clients.h#L157) | inline call | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |
| [src/apps/echo/model/servers.h:157](../../src/apps/echo/model/servers.h#L157) | inline call | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |
| [src/apps/http/httplowlevelclient.cpp:213](../../src/apps/http/httplowlevelclient.cpp#L213) | inline call | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |
| [src/apps/http/model/clients.h:743](../../src/apps/http/model/clients.h#L743) | inline call | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |
| [src/apps/http/model/servers.h:167](../../src/apps/http/model/servers.h#L167) | inline call | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |
| [src/apps/testregex.cpp:410](../../src/apps/testregex.cpp#L410) | inline call | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |
| [src/core/socket/stream/tls/ssl_utils.cpp:277](../../src/core/socket/stream/tls/ssl_utils.cpp#L277) | inline call | `sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);` |

## sk_GENERAL_NAME_value

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/echo/model/clients.h:141](../../src/apps/echo/model/clients.h#L141) | inline call | `GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);` |
| [src/apps/echo/model/servers.h:141](../../src/apps/echo/model/servers.h#L141) | inline call | `GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);` |
| [src/apps/http/httplowlevelclient.cpp:197](../../src/apps/http/httplowlevelclient.cpp#L197) | inline call | `GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);` |
| [src/apps/http/model/clients.h:727](../../src/apps/http/model/clients.h#L727) | inline call | `GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);` |
| [src/apps/http/model/servers.h:150](../../src/apps/http/model/servers.h#L150) | inline call | `GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);` |
| [src/apps/testregex.cpp:394](../../src/apps/testregex.cpp#L394) | inline call | `GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);` |
| [src/core/socket/stream/tls/ssl_utils.cpp:268](../../src/core/socket/stream/tls/ssl_utils.cpp#L268) | inline call | `const GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);` |

## snprintf

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/JsonWriter.cpp:90](../../src/tools/snodec-control/src/JsonWriter.cpp#L90) | inline call | `std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned int>(static_cast<unsigned char>(character)));` |

## socket

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/system/socket.cpp:52](../../src/core/system/socket.cpp#L52) | wrapper definition | `int socket(int domain, int type, int protocol) {` |
| [src/core/system/socket.cpp:54](../../src/core/system/socket.cpp#L54) | wrapper implementation call | `return ::socket(domain, type, protocol);` |
| [src/core/system/socket.h:66](../../src/core/system/socket.h#L66) | wrapper declaration | `int socket(int domain, int type, int protocol);` |
| [src/net/phy/PhysicalSocket.hpp:81](../../src/net/phy/PhysicalSocket.hpp#L81) | extracted caller | `int ret = Super::operator=(core::system::socket(domain, type &#124; flags, protocol)).getFd();` |
| [src/net/un/phy/PhysicalSocket.hpp:112](../../src/net/un/phy/PhysicalSocket.hpp#L112) | extracted caller | `const int probeFd = core::system::socket(PF_UNIX, type &#124; SOCK_NONBLOCK &#124; SOCK_CLOEXEC, protocol);` |
| [tests/component/http/InetHttpClientConnectFailureTest.cpp:19](../../tests/component/http/InetHttpClientConnectFailureTest.cpp#L19) | inline call | `const int fd = ::socket(AF_INET, SOCK_STREAM, 0);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:189](../../tests/component/net/AddressResolutionPolicyTest.cpp#L189) | inline call | `const int blocker = socket(AF_INET, SOCK_STREAM, 0);` |
| [tests/component/net/AddressResolutionPolicyTest.cpp:261](../../tests/component/net/AddressResolutionPolicyTest.cpp#L261) | inline call | `const int listener = socket(AF_INET, SOCK_STREAM, 0);` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:70](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L70) | inline call | `const int fd = ::socket(AF_UNIX, SOCK_STREAM &#124; SOCK_CLOEXEC, 0);` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:84](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L84) | inline call | `const int fd = ::socket(AF_UNIX, SOCK_STREAM &#124; SOCK_CLOEXEC, 0);` |
| [tests/support/NetworkTest.h:94](../../tests/support/NetworkTest.h#L94) | inline call | `fd = ::socket(AF_INET, SOCK_STREAM, 0);` |
| [tests/unit/core/ConnectionAttemptLifecycleTest.cpp:10](../../tests/unit/core/ConnectionAttemptLifecycleTest.cpp#L10) | inline call | `int reservation = socket(AF_INET, SOCK_STREAM, 0);` |

## socketpair

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/net/AddressResolutionPolicyTest.cpp:92](../../tests/component/net/AddressResolutionPolicyTest.cpp#L92) | inline call | `if (socketpair(AF_UNIX, SOCK_STREAM &#124; SOCK_NONBLOCK, 0, pair) != 0)` |
| [tests/unit/core/SocketWriterResourcePolicyTest.cpp:197](../../tests/unit/core/SocketWriterResourcePolicyTest.cpp#L197) | inline call | `if (::socketpair(AF_UNIX, SOCK_STREAM &#124; SOCK_CLOEXEC, 0, fds) != 0) {` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:74](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L74) | inline call | `if (::socketpair(AF_UNIX, SOCK_STREAM &#124; SOCK_CLOEXEC, 0, fds) == 0 && fds[0] > fds[1]) {` |
| [tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp:13](../../tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp#L13) | inline call | `require(socketpair(AF_UNIX, SOCK_STREAM &#124; SOCK_NONBLOCK, 0, sockets) == 0, "socketpair");` |
| [tests/unit/net/UnixPeerCredentialsTest.cpp:57](../../tests/unit/net/UnixPeerCredentialsTest.cpp#L57) | inline call | `const bool socketPairCreated = ::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0;` |

## stat

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:4324](../../src/utils/CLI11.hpp#L4324) | inline call | `if(stat(file, &buffer) == 0) {` |
| [src/web/http/http_utils.cpp:247](../../src/web/http/http_utils.cpp#L247) | inline call | `stat(filePath.c_str(), &attrib); // TODO: to core::system` |

## str2ba

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/l2/SocketAddress.cpp:83](../../src/net/l2/SocketAddress.cpp#L83) | inline call | `str2ba(btAddress.c_str(), &sockAddr.l2_bdaddr);` |
| [src/net/rc/SocketAddress.cpp:83](../../src/net/rc/SocketAddress.cpp#L83) | inline call | `str2ba(btAddress.c_str(), &sockAddr.rc_bdaddr);` |

## strcasecmp

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/http/CiStringMap.cpp:68](../../src/web/http/CiStringMap.cpp#L68) | inline call | `return ::strcasecmp(a.c_str(), b.c_str()) < 0;` |

## strdup

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:219](../../src/core/socket/stream/tls/ssl_utils.cpp#L219) | inline call | `SSL_CTX_set_default_passwd_cb_userdata(ctx, ::strdup(sslConfig.password.c_str()));` |

## strerror

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/websocket/echoserver.cpp:99](../../src/apps/websocket/echoserver.cpp#L99) | inline call | `snode::log::application().debug() << "HTTP response send file failed: " << std::strerror(errnum);` |
| [src/apps/websocket/echoserver.cpp:171](../../src/apps/websocket/echoserver.cpp#L171) | inline call | `snode::log::application().debug() << "HTTP response send file failed: " << std::strerror(errnum);` |
| [src/core/socket/State.cpp:59](../../src/core/socket/State.cpp#L59) | inline call | `, errstr(errnum != 0 ? std::strerror(errnum) : "") {` |
| [src/net/in/SocketAddress.cpp:106](../../src/net/in/SocketAddress.cpp#L106) | inline call | `state, aiErrCode == EAI_SYSTEM ? strerror(errno) : gai_strerror(aiErrCode), aiErrCode == EAI_SYSTEM ? errno : aiErrCode);` |
| [src/net/in/SocketAddress.cpp:138](../../src/net/in/SocketAddress.cpp#L138) | inline call | `(aiErrCode == EAI_SYSTEM ? strerror(errno) : gai_strerror(aiErrCode)),` |
| [src/net/in6/SocketAddress.cpp:106](../../src/net/in6/SocketAddress.cpp#L106) | inline call | `state, aiErrCode == EAI_SYSTEM ? strerror(errno) : gai_strerror(aiErrCode), aiErrCode == EAI_SYSTEM ? errno : aiErrCode);` |
| [src/net/in6/SocketAddress.cpp:138](../../src/net/in6/SocketAddress.cpp#L138) | inline call | `(aiErrCode == EAI_SYSTEM ? strerror(errno) : gai_strerror(aiErrCode)),` |
| [src/tools/snodec-control/src/ConfigActions.cpp:281](../../src/tools/snodec-control/src/ConfigActions.cpp#L281) | inline call | `error = std::strerror(errno);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:72](../../src/tools/snodec-control/src/ProcessRunner.cpp#L72) | inline call | `error = std::strerror(errno);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:208](../../src/tools/snodec-control/src/ProcessRunner.cpp#L208) | inline call | `result.spawnError = std::strerror(spawnRc);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:238](../../src/tools/snodec-control/src/ProcessRunner.cpp#L238) | inline call | `result.spawnError = std::strerror(spawnRc);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:306](../../src/tools/snodec-control/src/ProcessRunner.cpp#L306) | inline call | `result.spawnError = std::strerror(spawnRc);` |
| [src/utils/Config.cpp:491](../../src/utils/Config.cpp#L491) | inline call | `<< "] Writing config file: " << std::strerror(errno) << std::endl;` |
| [src/utils/Config.cpp:1005](../../src/utils/Config.cpp#L1005) | inline call | `<< "' to 'snodec':" << strerror(errno) << std::endl;` |
| [src/utils/Config.cpp:1026](../../src/utils/Config.cpp#L1026) | inline call | `std::cout << "Warning: Can not set group ownership of '" << logDirectory << "' to 'snodec':" << strerror(errno)` |
| [src/utils/Config.cpp:1048](../../src/utils/Config.cpp#L1048) | inline call | `std::cout << "Warning: Can not set group ownership of '" << pidDirectory << "' to 'snodec':" << strerror(errno)` |
| [src/utils/Daemon.cpp:69](../../src/utils/Daemon.cpp#L69) | inline call | `: DaemonFailure(errorMessage + ": " + std::strerror(errno)) {` |
| [tests/component/net/InetLegacyServerClientCompositionTest.cpp:80](../../tests/component/net/InetLegacyServerClientCompositionTest.cpp#L80) | inline call | `const bool addressInUse = state == core::socket::State::ERROR && stateMessage.find(std::strerror(EADDRINUSE)) != std::string::npos;` |

## strftime

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/SocketConnection.cpp:198](../../src/core/socket/stream/SocketConnection.cpp#L198) | inline call | `if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_ptr) > 0) {` |
| [src/core/socket/stream/SocketContext.cpp:247](../../src/core/socket/stream/SocketContext.cpp#L247) | inline call | `if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_ptr) > 0) {` |
| [src/web/http/http_utils.cpp:228](../../src/web/http/http_utils.cpp#L228) | inline call | `(void) strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", tm);` |
| [src/web/http/http_utils.cpp:249](../../src/web/http/http_utils.cpp#L249) | inline call | `(void) strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&(attrib.st_mtime))); // TODO: to core::system` |

## strlen

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:102](../../src/core/socket/stream/tls/ssl_utils.cpp#L102) | inline call | `memset(u, 0, ::strlen(static_cast<char*>(u))); // garble Password` |
| [src/core/socket/stream/tls/ssl_utils.cpp:105](../../src/core/socket/stream/tls/ssl_utils.cpp#L105) | inline call | `return static_cast<int>(std::strlen(buf));` |
| [src/utils/CLI11.hpp:436](../../src/utils/CLI11.hpp#L436) | inline call | `CLI11_INLINE std::wstring widen(const char *str) { return detail::widen_impl(str, std::strlen(str)); }` |

## strncpy

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/socket/stream/tls/ssl_utils.cpp:99](../../src/core/socket/stream/tls/ssl_utils.cpp#L99) | inline call | `strncpy(buf, static_cast<char*>(u), static_cast<std::size_t>(size));` |

## strptime

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/http/http_utils.cpp:237](../../src/web/http/http_utils.cpp#L237) | inline call | `strptime(http_date.c_str(), "%a, %d %b %Y %H:%M:%S", &tm);` |

## strsignal

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/websocket/subprotocol/client/echo/Echo.cpp:104](../../src/apps/websocket/subprotocol/client/echo/Echo.cpp#L104) | inline call | `snode::log::application().debug() << "SubProtocol 'echo' exit due to '" << strsignal(sig) << "' (SIG"` |
| [src/apps/websocket/subprotocol/server/echo/Echo.cpp:105](../../src/apps/websocket/subprotocol/server/echo/Echo.cpp#L105) | inline call | `snode::log::application().debug() << "SubProtocol 'echo' exit due to '" << strsignal(sig) << "' (SIG"` |
| [src/core/EventLoop.cpp:231](../../src/core/EventLoop.cpp#L231) | extracted caller | `utils::system::strsignal(signalNumber),` |
| [src/core/socket/stream/SocketConnection.hpp:404](../../src/core/socket/stream/SocketConnection.hpp#L404) | extracted caller | `utils::system::strsignal(signum),` |
| [src/utils/system/signal.cpp:70](../../src/utils/system/signal.cpp#L70) | wrapper definition | `std::string strsignal(int sig) {` |
| [src/utils/system/signal.cpp:71](../../src/utils/system/signal.cpp#L71) | wrapper implementation call | `return ::strsignal(sig);` |
| [src/utils/system/signal.h:62](../../src/utils/system/signal.h#L62) | wrapper declaration | `std::string strsignal(int sig);` |

## strtold

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:2843](../../src/utils/CLI11.hpp#L2843) | inline call | `auto output_ld = std::strtold(input.c_str(), &val);` |

## strtoll

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:2635](../../src/utils/CLI11.hpp#L2635) | inline call | `std::int64_t output_sll = std::strtoll(input.c_str(), &val, 0);` |
| [src/utils/CLI11.hpp:2690](../../src/utils/CLI11.hpp#L2690) | inline call | `std::int64_t output_ll = std::strtoll(input.c_str(), &val, 0);` |
| [src/utils/CLI11.hpp:2720](../../src/utils/CLI11.hpp#L2720) | inline call | `output_ll = std::strtoll(input.c_str() + 2, &val, 8);` |
| [src/utils/CLI11.hpp:2733](../../src/utils/CLI11.hpp#L2733) | inline call | `output_ll = std::strtoll(input.c_str() + 2, &val, 2);` |
| [src/utils/CLI11.hpp:2784](../../src/utils/CLI11.hpp#L2784) | inline call | `ret = std::strtoll(val.c_str(), &loc_ptr, 0);` |

## strtoul

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/sha1.cpp:261](../../src/utils/sha1.cpp#L261) | inline call | `buf.push_back(static_cast<unsigned char>(strtoul(hex_byte, &end_ptr, 16)));` |

## strtoull

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:2626](../../src/utils/CLI11.hpp#L2626) | inline call | `std::uint64_t output_ll = std::strtoull(input.c_str(), &val, 0);` |
| [src/utils/CLI11.hpp:2658](../../src/utils/CLI11.hpp#L2658) | inline call | `output_ll = std::strtoull(input.c_str() + 2, &val, 8);` |
| [src/utils/CLI11.hpp:2671](../../src/utils/CLI11.hpp#L2671) | inline call | `output_ll = std::strtoull(input.c_str() + 2, &val, 2);` |

## syscall:SYS_getrandom

| Source | Role | Source line |
| --- | --- | --- |
| [src/web/websocket/client/SocketContextUpgradeFactory.cpp:76](../../src/web/websocket/client/SocketContextUpgradeFactory.cpp#L76) | inline call | `const ssize_t ret = syscall(SYS_getrandom, buf, buflen, 0);` |

## syscall:SYS_pidfd_open

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Daemon.cpp:174](../../src/utils/Daemon.cpp#L174) | inline call | `const int pidfd = static_cast<int>(syscall(SYS_pidfd_open, pid, 0)); // NOLINT` |

## system

| Source | Role | Source line |
| --- | --- | --- |
| [src/apps/warema-jalousien.cpp:78](../../src/apps/warema-jalousien.cpp#L78) | inline call | `int ret = system(arguments.c_str());` |

## tcgetattr

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:350](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L350) | inline call | `if (::tcgetattr(STDIN_FILENO, &oldTermios) != 0) {` |

## tcsetattr

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:362](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L362) | inline call | `::tcsetattr(STDIN_FILENO, TCSANOW, &rawTermios);` |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:368](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L368) | inline call | `::tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);` |

## tcsetpgrp

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:318](../../src/tools/snodec-control/src/ProcessRunner.cpp#L318) | inline call | `::tcsetpgrp(STDIN_FILENO, pid);` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:339](../../src/tools/snodec-control/src/ProcessRunner.cpp#L339) | inline call | `::tcsetpgrp(STDIN_FILENO, parentPgrp);` |

## time

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/system/time.cpp:52](../../src/utils/system/time.cpp#L52) | wrapper definition | `time_t time(time_t* tloc) {` |
| [src/utils/system/time.cpp:54](../../src/utils/system/time.cpp#L54) | wrapper implementation call | `return ::time(tloc);` |
| [src/utils/system/time.h:59](../../src/utils/system/time.h#L59) | wrapper declaration | `time_t time(time_t* tloc);` |
| [src/web/http/http_utils.cpp:221](../../src/web/http/http_utils.cpp#L221) | extracted caller | `const time_t now = utils::system::time(nullptr);` |

## tolower

| Source | Role | Source line |
| --- | --- | --- |
| [src/express/dispatcher/regex_utils.cpp:545](../../src/express/dispatcher/regex_utils.cpp#L545) | inline call | `: (std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)));` |
| [src/express/dispatcher/regex_utils.cpp:545](../../src/express/dispatcher/regex_utils.cpp#L545) | inline call | `: (std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)));` |
| [src/express/dispatcher/regex_utils.h:67](../../src/express/dispatcher/regex_utils.h#L67) | inline call | `return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));` |
| [src/express/dispatcher/regex_utils.h:67](../../src/express/dispatcher/regex_utils.h#L67) | inline call | `return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));` |
| [src/utils/CLI11.hpp:653](../../src/utils/CLI11.hpp#L653) | inline call | `return std::tolower(x, std::locale());` |
| [src/utils/Config.cpp:94](../../src/utils/Config.cpp#L94) | inline call | `return static_cast<char>(std::tolower(ch));` |
| [src/utils/Formatter.cpp:117](../../src/utils/Formatter.cpp#L117) | inline call | `return static_cast<char>(std::tolower(c));` |
| [src/web/http/CiStringMap.cpp:55](../../src/web/http/CiStringMap.cpp#L55) | inline call | `return std::tolower(ch1) == std::tolower(ch2);` |
| [src/web/http/CiStringMap.cpp:55](../../src/web/http/CiStringMap.cpp#L55) | inline call | `return std::tolower(ch1) == std::tolower(ch2);` |
| [src/web/http/CiStringMap.cpp:63](../../src/web/http/CiStringMap.cpp#L63) | inline call | `return std::tolower(ch1) == std::tolower(ch2);` |
| [src/web/http/CiStringMap.cpp:63](../../src/web/http/CiStringMap.cpp#L63) | inline call | `return std::tolower(ch1) == std::tolower(ch2);` |
| [src/web/http/http_utils.cpp:65](../../src/web/http/http_utils.cpp#L65) | inline call | `return isdigit(ch) != 0 ? ch - '0' : tolower(ch) - 'a' + 10;` |
| [src/web/http/http_utils.cpp:140](../../src/web/http/http_utils.cpp#L140) | inline call | `return std::tolower(a) == std::tolower(b);` |
| [src/web/http/http_utils.cpp:140](../../src/web/http/http_utils.cpp#L140) | inline call | `return std::tolower(a) == std::tolower(b);` |

## toupper

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ui/CursesUi.cpp:126](../../src/tools/snodec-control/src/ui/CursesUi.cpp#L126) | inline call | `result.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(result.front())));` |

## umask

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/Daemon.cpp:143](../../src/utils/Daemon.cpp#L143) | inline call | `umask(0);` |

## unlink

| Source | Role | Source line |
| --- | --- | --- |
| [src/net/un/phy/PhysicalSocket.hpp:197](../../src/net/un/phy/PhysicalSocket.hpp#L197) | inline call | `if (::unlink(socketPath.c_str()) == 0) {` |
| [src/net/un/phy/PhysicalSocket.hpp:224](../../src/net/un/phy/PhysicalSocket.hpp#L224) | inline call | `if (::unlink(lockPath.c_str()) == 0) {` |
| [src/net/un/phy/PhysicalSocket.hpp:428](../../src/net/un/phy/PhysicalSocket.hpp#L428) | inline call | `} else if (::unlink(socketPath.c_str()) != 0) {` |
| [src/tools/snodec-control/src/ConfigActions.cpp:290](../../src/tools/snodec-control/src/ConfigActions.cpp#L290) | inline call | `::unlink(buffer.data());` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:183](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L183) | inline call | `::unlink(activePath.c_str());` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:257](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L257) | inline call | `::unlink(boundBeforeListenPath.c_str());` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:295](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L295) | inline call | `testResult.expectEqual(0, ::unlink(replacementPath.c_str()), "test unlinks the owned socket pathname");` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:299](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L299) | inline call | `testResult.expectEqual(0, ::unlink((replacementPath + ".lock").c_str()), "test unlinks the owned marker pathname");` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:321](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L321) | inline call | `::unlink(regularPath.c_str());` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:322](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L322) | inline call | `::unlink(activePath.c_str());` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:323](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L323) | inline call | `::unlink(unmarkedStalePath.c_str());` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:324](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L324) | inline call | `::unlink(ownedStalePath.c_str());` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:325](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L325) | inline call | `::unlink(boundBeforeListenPath.c_str());` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:326](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L326) | inline call | `::unlink(unrecognizedLockPath.c_str());` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:327](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L327) | inline call | `::unlink((unrecognizedLockPath + ".lock").c_str());` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:328](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L328) | inline call | `::unlink(replacementPath.c_str());` |
| [tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp:329](../../tests/component/net/UnixPhysicalSocketPathSafetyTest.cpp#L329) | inline call | `::unlink((replacementPath + ".lock").c_str());` |

## usleep

| Source | Role | Source line |
| --- | --- | --- |
| [tests/component/core/ShutdownReceiverNotificationTest.cpp:97](../../tests/component/core/ShutdownReceiverNotificationTest.cpp#L97) | inline call | `::usleep(100000);` |

## waitpid

| Source | Role | Source line |
| --- | --- | --- |
| [src/tools/snodec-control/src/ProcessRunner.cpp:217](../../src/tools/snodec-control/src/ProcessRunner.cpp#L217) | inline call | `if (::waitpid(pid, &status, 0) == pid) {` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:245](../../src/tools/snodec-control/src/ProcessRunner.cpp#L245) | inline call | `if (::waitpid(pid, &status, 0) == pid) {` |
| [src/tools/snodec-control/src/ProcessRunner.cpp:324](../../src/tools/snodec-control/src/ProcessRunner.cpp#L324) | inline call | `waited = ::waitpid(pid, &status, 0);` |
| [tests/component/core/ShutdownReceiverNotificationTest.cpp:119](../../tests/component/core/ShutdownReceiverNotificationTest.cpp#L119) | inline call | `testResult.expectEqual(signalerPid, ::waitpid(signalerPid, &signalerStatus, 0), "SIGINT sender process is reaped");` |

## wcslen

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:431](../../src/utils/CLI11.hpp#L431) | inline call | `CLI11_INLINE std::string narrow(const wchar_t *str) { return detail::narrow_impl(str, std::wcslen(str)); }` |

## wcsrtombs

| Source | Role | Source line |
| --- | --- | --- |
| [src/utils/CLI11.hpp:380](../../src/utils/CLI11.hpp#L380) | inline call | `std::size_t new_size = std::wcsrtombs(nullptr, &it, 0, &state);` |
| [src/utils/CLI11.hpp:386](../../src/utils/CLI11.hpp#L386) | inline call | `std::wcsrtombs(const_cast<char *>(result.data()), &str, new_size, &state);` |

## write

| Source | Role | Source line |
| --- | --- | --- |
| [src/core/pipe/PipeSource.cpp:146](../../src/core/pipe/PipeSource.cpp#L146) | extracted caller | `const ssize_t ret = core::system::write(getRegisteredFd(), writeBuffer.data() + writeOffset, writeSize);` |
| [src/core/system/unistd.cpp:67](../../src/core/system/unistd.cpp#L67) | wrapper definition | `ssize_t write(int fd, const void* buf, std::size_t count) {` |
| [src/core/system/unistd.cpp:69](../../src/core/system/unistd.cpp#L69) | wrapper implementation call | `return ::write(fd, buf, count);` |
| [src/core/system/unistd.h:68](../../src/core/system/unistd.h#L68) | wrapper declaration | `ssize_t write(int fd, const void* buf, std::size_t count);` |
| [src/tools/snodec-control/src/ConfigActions.cpp:285](../../src/tools/snodec-control/src/ConfigActions.cpp#L285) | inline call | `const ssize_t written = ::write(fd, content.data(), content.size());` |
| [tests/component/core/FileReaderTest.cpp:111](../../tests/component/core/FileReaderTest.cpp#L111) | inline call | `if (writeFd < 0 &#124;&#124; ::write(writeFd, payload.data(), payload.size()) != static_cast<ssize_t>(payload.size())) {` |
| [tests/component/core/PipeSinkFairnessTest.cpp:69](../../tests/component/core/PipeSinkFairnessTest.cpp#L69) | inline call | `static_cast<void>(::write(drainWriteFd, "efgh", 4));` |
| [tests/component/core/PipeSinkFairnessTest.cpp:97](../../tests/component/core/PipeSinkFairnessTest.cpp#L97) | inline call | `static_cast<void>(::write(drainWriteFd, "abcd", 4));` |
| [tests/component/core/PipeSinkFairnessTest.cpp:98](../../tests/component/core/PipeSinkFairnessTest.cpp#L98) | inline call | `static_cast<void>(::write(limitedWriteFd, "12345678", 8));` |
| [tests/unit/core/StreamFrameworkShutdownTest.cpp:265](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L265) | inline call | `return ::write(getRegisteredFd(), chunk, chunkLen);` |
