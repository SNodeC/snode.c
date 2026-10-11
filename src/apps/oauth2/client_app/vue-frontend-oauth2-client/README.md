# OAuth2 client-app frontend

This Vue 3 and TypeScript frontend belongs to SNode.C's `client_app` OAuth2 example. The [application source](../ClientApp.cpp) serves the Vite-built `dist/` files through SNode.C's HTTP/Express handlers.

## Build

Use Node.js and npm compatible with the checked-in Vite dependency. In this frontend directory:

```sh
npm ci
npm run build
```

The package's build script type-checks with `vue-tsc` and builds with Vite. The output is `dist/`; `npm run dev` starts the Vite development server and `npm run preview` previews a completed build. See [the package scripts](package.json).

The application source currently names absolute example paths for `dist/`. Adjust those paths to your checkout/deployment before expecting the application to serve your build; building the frontend does not change them.
