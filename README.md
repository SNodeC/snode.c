# snode.c
snode.c is a slim C++-toolkit for web applications still in heavy development

_Copyright:_ GNU LESSER GENERAL PUBLIC LICENSE

_Requirements:_
* gcc version 10
* libeasyloggingpp
* openssl
* doxygen
* cmake
* iwyu
* libmagic
* clang-format
* nlohmann-json3-dev

_Components:_
* libnet (directory net) low level multiplexed network-layer (server/client) with event-loop supporting legacy- and tls-connections
* libhttp (directory http) low level http server and client
* libexpress (directory express) high level web-api similar to node.js's express module
* example applications (directory apps)

_TODOs:_
* Add better error processing in HTTPResponseParser
* Improve regex-match in Router (path-to-regex in JS)
* Add some more complex example apps (Game, Skill for Alexa, ...)
* Add WebSocket support for server and client, legacy and tls
* Add support for Template-Engines (similar to express)
* Add additional transport protocols (udp, bluetooth, local, ipv6, ...)
* Implement other protocols (contexts) than http (e.g. mqtt, ftp would be interresting, telnet, smtp)
* Finalize cmake-rules to support install
* Add cmake description files
* Add "logical" events
* Support "promisses"
* Add OAUTH2 server/client authentification
