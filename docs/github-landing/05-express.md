# Express-style Module (`src/express/*`)

## Module role

`src/express` provides a high-level web API inspired by Node.js Express, built on SNode.C HTTP primitives while retaining event-driven C++ performance characteristics.

## Core concepts

- `Router`: route registration, middleware chaining, method-based handlers.
- `Request` / `Response`: Express-like request/response facade.
- `Next`: continuation token for middleware flow (`next()` semantics).
- `WebApp`: router + lifecycle facade (`init/start/tick/stop/free/state`) bridging to core runtime.
- `Controller`: controller-oriented registration helpers.
- dispatcher + route classes: route matching and invocation internals.

## Middleware ecosystem

Built-in middleware includes:

- `StaticMiddleware`: static file serving with configurable root/index/default headers/cookies.
- `JsonMiddleware`: JSON parsing/handling convenience.
- `BasicAuthentication`: HTTP basic auth helpers.
- `VHost`: host-based routing partitions.
- `VerboseRequest`: request logging middleware.

## Legacy and TLS deployment variants

Separate namespaces and wrappers exist for:

- legacy transports (`express/legacy/*`)
- TLS transports (`express/tls/*`)
- network families (`in`, `in6`, `rc`, `un`)

So application-level route code can remain stable while transport endpoint classes change.

## Compatibility track

`express-compat-suite` shows dedicated effort to compare/align behavior with Express patterns using test fixtures and adapter server logic.

This is strategically important: it lowers migration friction for teams with Node/Express mental models.

## Functionality summary

- Express-like routing and middleware in C++
- native integration with SNode.C lifecycle and network stack
- ready-to-use middleware for static, JSON, auth, vhost, and request tracing
- compatibility-oriented subsystem for behavioral parity work

## Pros

1. **Developer-friendly API surface** for web developers moving from Node.js.
2. **Strong middleware composition model** with explicit control flow.
3. **Transport flexibility** without route-level rewrites.
4. **Compatibility-suite mindset** signals maintainability and UX commitment.

## Cons / tradeoffs

1. **Not a full Express clone**: subtle behavioral gaps can exist.
2. **C++ template/macros and ownership model** remain more complex than dynamic-language frameworks.
3. **Higher conceptual stack** (core/net/http/express) for contributors who jump in at middleware level.

## Recommended messaging for landing page

Describe Express layer as **“familiar web ergonomics on top of deterministic C++ event-driven internals”** rather than as a strict clone.
