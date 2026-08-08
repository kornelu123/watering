# Server tests

The server unit tests run on the host and do not require Redis, TLS certificates,
Pico hardware, or the server's third-party runtime libraries.

From `serv/`, run:

```sh
make test
```

This configures a small CMake/CTest project, builds every test with strict compiler
warnings, and runs the packet-creator, ring-buffer, and protocol-helper tests. Test
artifacts are written to `serv/tests/build/` and removed by `make clean`.
