<!-- snodec:begin page-header -->
<a id="page-overview"></a>
<p>
  <a href="../../README.md#project-overview" title="SNode.C repository"><img src="../../docs/media/page-banner.svg" alt="SNode.C repository" width="100%"></a>
</p>
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
