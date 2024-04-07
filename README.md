# Kapuc
General programming language for low-level performant program.

## Build instruction
- install bazel/bazelisk
- (not required, for development only) use `bazel run :refresh_compile_commands` to generate `compile_commands.json` for autocomplete.
- `bazel build //:kapuc` to build the binary, it should be at bazel-bin/kapuc. (Note: the build name will be `unstable (unknown)`)

## Development instruction
- Generate `compile_commands.json` with `bazel run :refresh_compile_commands` for autocomplete
- Look at
  - `spec.md` for the spec of the language
  - (TODO) `docs/**/*.md` for documentation of libraries and the compiler
  - `lib/env_args.{cpp,h}` for arguments of the program (kapuc compiler)
  - `src/kapuc` for the compiler stage
    - `src/kapuc/{lex,parse}.{c,h}` for syntax/parser
  - (TODO) `src/kapu/` for the main package manager/build system
- Before making pr, make sure
  - You run `clang-format` with all of the files you modified
  - You make sure the build is successful
  - (TODO) check clang-tidy with `clang-tidy --warnings-as-errors=* ./src/kapuc/*.{c,h} ./lib/env_args.{cpp,h}`

## Known issues
- Windows
  - Probably have an linking issues on build, no testing were done on Windows.
  - Cannot be built with `cl.exe` (Visual Studio C Compiler), please use Cygwin if you are on Windows or use WSL.
