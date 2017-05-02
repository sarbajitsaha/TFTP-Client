# TFTP Client

### Building

CMake is required to build.

#### OSX or Linux with Make
```bash
cmake --build ./cmake-build-debug --target clean -- -j 4
cmake --build ./cmake-build-debug --target MyTFTP -- -j 4

./cmake-build-debug/MyTFTP # the executable file
```

Default port no is 69. Run with command line argument server IP e.g. ./MyTFTP 127.0.0.1

To receive a file, use "get (filename)"

To send a file, use "put (filename)"

Follows the protocol specified in https://tools.ietf.org/html/rfc1350
