# Kapuc

- install bazel/bazelisk
- use `bazelisk run :refresh_compile_commands` or `bazelisk run --define use_system_libgccjit=true :refresh_compile_commands` for system libgccjit to generate `compile_commands.json`
- `bazelisk build //:kapuc --define use_system_libgccjit=true` if you already have libgccjit installed (should come with gcc?)
