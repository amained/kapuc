{pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs = with pkgs.buildPackages; [meson llvmPackages_17.libllvm clang-tools_17 ninja];
  }
