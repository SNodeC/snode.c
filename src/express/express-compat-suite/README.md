<!-- snodec:begin page-header -->
<a id="page-overview"></a>
<p>
  <a href="../../../README.md#project-overview" title="SNode.C repository"><img src="../../../docs/readme/media/page-banner.svg" alt="SNode.C repository" width="100%"></a>
</p>
<!-- snodec:end page-header -->

# Express Compatibility Suite (Node.js/Express v4 vs SNode.C/Express)

This bundle contains:
- A Node.js/Express reference server (`node/`)
- An SNode.C/Express compatibility server (`snodec/`)
- A test suite + runner (`tests/`) that can validate each server against expected values and/or compare both servers.

## 0) What this tests
Focused on behavioral parity for:
- case-insensitive routing default
- trailing-slash (strictRouting off)
- wildcard captures (`*` -> req.params["0"], ...)
- param decoding (`decodeURIComponent` behavior) incl. `%2F` and spaces
- query decoding (`%xx` and `+` -> space) + key casing (`A` and `a` are distinct)
- HEAD -> GET fallback and empty body on HEAD
- mounted routers: req.url / req.baseUrl / req.originalUrl stripping
- mergeParams semantics
- next('route') and next('router')
- invalid percent-escapes in params -> HTTP 400

## 1) Start Node.js/Express server
```bash
cd node
npm install
PORT=3000 node server.js
```

## 2) Start SNode.C/Express server
The framework already defines the `express-compat-server` target; do not apply the historical patch in `snodec/` to a current checkout.

From your SNode.C repo root:
```bash
cmake -S . -B build
cmake --build build --target express-compat-server
./build/src/apps/express-compat-server   # listens on port 8080 by default
```

## 3) Run the tests
In another terminal, from the suite root:

### Compare Node vs SNode (parity check)
```bash
./tests/run-compat.sh --node http://127.0.0.1:3000 --snode http://127.0.0.1:8080
```

### Validate a single server against expected outputs
```bash
./tests/run-compat.sh --expect-only --node http://127.0.0.1:3000
./tests/run-compat.sh --expect-only --snode http://127.0.0.1:8080
```

Dependencies: `bash`, `curl`, `python3`.
