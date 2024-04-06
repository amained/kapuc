# Kapuc

- install bazel/bazelisk
- use `bazel run :refresh_compile_commands` to generate `compile_commands.json` for autocomplete.
- `bazel build //:kapuc` to build the binary, it should be at bazel-bin/kapuc.

## Current Issues
- Windows
  - Probably have an linking issues on build, no testing were done on Windows.
  - Cannot be built with `cl.exe` (Visual Studio C Compiler), please use Cygwin if you are on Windows or use WSL.
