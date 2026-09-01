# source/library

Reusable, module-internal C++ code shared by several functions.

Dependency direction (enforced by convention):

```
source/functions   ->   source/library   ->   source/sdk
```

- A function may use library code.
- Library code may use the SDK (`<mta/sdk.hpp>` or `sdk/...` headers).
- Library code must NOT depend on anything in `source/functions/` and must
  not register Lua functions itself.
- A new `.cpp` here is picked up by the build automatically (the whole
  `source/**/*.cpp` tree is compiled).

`base/` holds framework-adjacent helpers that are useful to almost any
module. Organize anything else by domain: `library/http/`, `library/json/`,
`library/crypto/` — one folder per topic.