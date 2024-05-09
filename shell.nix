{pkgs ? import <nixpkgs> {config.allowUnfree = true;} }:
  pkgs.mkShell {
    nativeBuildInputs = with pkgs.buildPackages; [meson llvmPackages_17.libllvm clang-tools_17 ninja
    (vscode-with-extensions.override {
    vscodeExtensions = with vscode-extensions; [
      llvm-vs-code-extensions.vscode-clangd
    ];
  })];
  }
