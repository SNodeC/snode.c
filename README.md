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
