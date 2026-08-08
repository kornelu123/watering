# Server tests

These host-side unit tests exercise server code without Redis, TLS certificates,
Pico hardware, or the server's third-party runtime libraries.

Run the suite from `serv/`:

```sh
make test
```

The command configures the test project, builds it, and runs all tests through
CTest. Generated files are kept in `serv/tests/build/` and removed by
`make clean`.
