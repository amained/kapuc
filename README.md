# Kapuc

- please install bazel
- use `bazel run @hedron_compile_commands//:refresh_all` to generate `compile_commands.json` for documentation
- `bazelisk build //:kapuc --define use_system_libgccjit=true` if you already have libgccjit installed (should come with gcc?)
