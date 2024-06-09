# Kapuc
General programming language for low-level performant program.

## Build instruction
- nix users can use this
  ```bash
  nix-shell -p meson wezterm llvmPackages_17.libllvm ninja
  ```
- install llvm version >17.0
  - ubuntu/debian users use this
  ```bash
  wget https://apt.llvm.org/llvm.sh
  chmod +x llvm.sh
  ./llvm.sh 17 # run as root
  ```
- install meson and ninja (```python3 -m pip install --user meson ninja```)
- actually building
  ```bash
  mkdir build # or whatever equivalent of this on Windows
  meson setup build
  meson compile -v -C build
  ```

## Development instruction
- Look at
  - `spec.md` for the spec of the language
  - (TODO) `docs/**/*.md` for documentation of libraries and the compiler
  - `lib/env_args.{cpp,h}` for arguments of the program (kapuc compiler)
  - `src/kapuc` for the compiler stage
    - `src/kapuc/{lex,parse}.{c,h}` for syntax/parser
  - (TODO) `src/kapu/` for the main package manager/build system
- Before making pr, make sure
  - You run `clang-format` with all of the files you modified (or simpler, ```git clang-format --staged``` for staged changes or ```ninja -C build clang-format``` for full project)
  - You make sure the build is successful
  - (TODO) check clang-tidy with `clang-tidy --warnings-as-errors=* ./src/kapuc/*.{c,h} ./lib/env_args.{cpp,h}`

## Known issues
- Windows
  - Probably have an linking issues on build, no testing were done on Windows.
  - Cannot be built with `cl.exe` (Visual Studio C Compiler), please use Cygwin if you are on Windows or use WSL. 
    (TODO: this could work? we could remove posix parts out but why)
