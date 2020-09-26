# snode.c
## Copyright
(C) GNU LESSER GENERAL PUBLIC LICENSE

##### Initiator and Main developer
- Volker Christian (me_AT_vchrist.at, volker.christian_AT_fh-hagenberg.at)

##### Contributors (Students)
_Json Middleware_
- Marlene Mayr
- Anna Moser
- Matteo Prock
- Eric Thalhammer

_Regular-Expression Route-Mapping_
- Joelle Helgert
- Julia Gruber
- Patrick Brandstätter
- Fabian Mohr

_Runtime Module Loading_

## What is snode.c
snode.c (Simple NODE in C\+\+) is a lean, highly extendable, high-performance layer-based framework for network applications (servers and clients) in the spirit of node.js written entirely in C\+\+.
The development of the  framework has started during the summer semester 2020 in the context of the course **Network and Distributed Systems** of the master's program [**Interactive Media**](https://www.fh-ooe.at/en/hagenberg-campus/studiengaenge/master/interactive-media/) at the departement [**Informatics, Communications and Media**](https://www.fh-ooe.at/en/hagenberg-campus/) at the [**University of Applied Sciences Upper Austria, Campus Hagenberg**](https://www.fh-ooe.at/en/) to give students an insight into the fundamental techniques of network- and web-frameworks.
Main focus (but not only) of the framework is "Machine to Machine" (M2M) communication and here especially the field of "Internet of Things" (IoT).
Keep in mind, that the framework is still in heavy development, APIs can break from commit to commit. But the highest level API is considered stable.

### Feature List (not complete)
- Non-blocking, event-driven (asynchronous), single threaded, single tasking, layer based design supporting timer (single-shot, interval) and network communication (TCP/IPv4).
- Application protocol independent TCP server and TCP client functionality. Application protocols (HTTP, ...) can be connected easily and modularly.
- TLS/SSL encryption for server and client.
- Support of X.509 certificates. TLS/SSL connections can be protected and authenticated using certificates.
- Fully implemented HTTP(S) application protocol layer (server and client) to which application logic can be connected easily.
- High-Level Web API layer with JSON support similar to the API of node.js/express. (Speedup compared to node.js/express approx. 40)

### Requirements
- GCC Version 10: As snode.c uses most recent C++-20 language features
- libeasyloggingpp: For logging
- openssl: For SSL/TLS support
- doxygen: For creating the API-documentation
- cmake:
- iwyu: To check for correct and complete included headers
- libmagic: To recognize the type of data in a file using "magic" numbers
- clang-format: To format the sourcecode consistently
- nlohmann-json3-dev: For JSON support

### Components
- libnet (directory net) low level multiplexed network-layer (server/client) with event-loop supporting legacy- and tls-connections
- libhttp (directory http) low level http server and client
- libexpress (directory express) high level web-api similar to node.js's express module
- example applications (directory apps)

### TODOs:
- Add better error processing in HTTPResponseParser
- Extend regex-match in Router (path-to-regex in JS)
- Add some more complex example apps (Game, Skill for Alexa, ...)
- Add WebSocket support for server and client, legacy and tls
- Add support for Template-Engines (similar to express)
- Add additional transport protocols (udp, bluetooth, local, ipv6, ...)
- Implement other protocols (contexts) than http (e.g. mqtt, ftp would be interresting, telnet, smtp)
- Finalize cmake-rules to support install
- Add cmake description files
- Add "logical" events
- Support "promisses"
- Add OAUTH2 server/client authentification
- Write unit tests
- Porting snode.c to MS-Windows
- Porting snode.c to macOS
- Add onSSLConnect callback
