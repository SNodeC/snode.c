<!-- snodec:begin page-header -->
<a id="page-overview"></a>
<p><picture><source media="(max-width: 640px)" srcset="../../docs/media/nav/banner-examples-compact.svg"><img src="../../docs/media/nav/banner-examples.svg" alt="SNode.C — Examples" width="100%"></picture></p>
<p><a href="../../README.md#project-overview" title="Up to SNode.C"><picture><source media="(prefers-color-scheme: dark)" srcset="../../docs/media/nav/up-snode-c-dark.svg"><img src="../../docs/media/nav/up-snode-c-light.svg" alt="Up to SNode.C" title="Up to SNode.C" width="160" height="28"></picture></a><a href="https://github.com/SNodeC" title="SNodeC organization"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../../docs/media/nav/organization-icon-dark.svg"><source media="(max-width: 640px)" srcset="../../docs/media/nav/organization-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../../docs/media/nav/organization-dark.svg"><img align="right" src="../../docs/media/nav/organization-light.svg" alt="SNodeC organization" title="SNodeC organization"></picture></a><a href="../../README.md#project-overview" title="SNode.C repository"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../../docs/media/nav/repository-icon-dark.svg"><source media="(max-width: 640px)" srcset="../../docs/media/nav/repository-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../../docs/media/nav/repository-dark.svg"><img align="right" src="../../docs/media/nav/repository-light.svg" alt="SNode.C repository" title="SNode.C repository"></picture></a><br clear="all"></p>
<!-- snodec:end page-header -->

# Example application inventory

These applications are built as part of SNode.C when their required components are enabled. The directory CMake files define the available executables (names containing `${STREAM}` / `${NET}` are CMake templates expanded for enabled variants); this inventory does not imply that every reduced build includes every target.

| Directory | Purpose | Executable targets |
| --- | --- | --- |
| [echo](echo/) | Framework echo endpoints and composition variants | `echoclient-${STREAM}-${NET}`, `echoserver-${STREAM}-${NET}` |
| [http](http/) | HTTP clients, Express servers and middleware examples | `httpserver-${STREAM}-${NET}`, `httpclient-${STREAM}-${NET}`, `httplowlevelclient`, `verysimpleserver`, `vhostserver`, `testbasicauthentication`, `testexpressnext` |
| [websocket](websocket/) | WebSocket clients and servers | `wsechoserver`, `wsechoclient` |
| [database](database/) | Database client examples | `testmariadb` |
| [oauth2](oauth2/) | OAuth2 authorization, client and resource applications | `authorization_server`, `client_app`, `resource_server` |
| [tlslegacy](tlslegacy/) | TLS and plain-stream examples | `tlslegacyserver`, `tlslegacyclient` |
| [towercalculator](towercalculator/) | Protocol/application composition example | `towerCalculation` |

For the complete installed-consumer tutorial, follow the [standalone echo pair](../../examples/echo/README.md#page-overview). Its annotated source, configuration, run commands and four application-level CTests now have one owner there. The root README retains its shorter [first success](../../README.md#first-success).

Top-level utilities include `configtest`, `jsonserver`, `jsonclient`, `testpipe`, `testpost`, `testregex`, and `express-compat-server`; consult [the application build file](CMakeLists.txt) for their required libraries and install components.

## OAuth2 frontends

- [Authorization-server frontend](oauth2/authorization_server/vue-frontend-oauth2-auth-server/README.md)
- [Client-app frontend](oauth2/client_app/vue-frontend-oauth2-client/README.md)

## Express testing

- [Express compatibility suite](../express/express-compat-suite/README.md#page-overview)
