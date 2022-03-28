# bplustree

![build status](https://github.com/juliencombattelli/bplustree/actions/workflows/build.yml/badge.svg)

A STL-compliant B+ tree implementation, portable and header-only, written using C++17.

bplustree meets the following named-requirements:
*[Container](https://en.cppreference.com/w/cpp/named_req/Container)*,
*[AllocatorAwareContainer](https://en.cppreference.com/w/cpp/named_req/AllocatorAwareContainer)*,
*[SequenceContainer](https://en.cppreference.com/w/cpp/named_req/SequenceContainer)*,
*[ReversibleContainer](https://en.cppreference.com/w/cpp/named_req/ReversibleContainer)*.

## Building

### Requirements

A C++17-compliant compiler is required to use this library. Since it is header-only, it can just be downloaded or copy-pated into anyone's project.
However, it is recommended to use CMake to build the tests to ensure it behaves sanely. In that case CMake 3.14+ is needed.
Also, to build the fuzzy tests, Clang 12+ is mandatory.

### Build options

- **BPLUSTREE_ENABLE_TEST** — Build the unit tests (default: ON)
- **BPLUSTREE_ENABLE_FUZZ** *[not implemented]* — Build the fuzzy tests (default: OFF, needs BPLUSTREE_ENABLE_TEST=ON)
- **BPLUSTREE_ENABLE_COVERAGE** — Build with code coverage analysis (default: OFF, needs BPLUSTREE_ENABLE_TEST=ON)

### Build procedure

```bash
# Configure the project
cmake -S path-to-bplustree -B bplustree-build -DCMAKE_BUILD_TYPE=Debug
# Build BPlusTree
cmake --build bplustree-build --parallel 4
# Run tests
cmake --build bplustree-build --target test
```

or alternatively, if you have CMake 3.20+ installed, you can use the provided presets:

```bash
# Configure the project
cmake -S path-to-bplustree --preset ninja-debug
# Build BPlusTree
cmake --build --preset ninja-debug --parallel 4
# Run tests
cmake --build --preset ninja-debug --target test
```

## Contributing

If you want to get involved and suggest some additional features, signal a bug or submit a patch, please create
a pull request or open an issue on the [BPlusTree Github repository](https://github.com/juliencombattelli/bplustree).
