{pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    nativeBuildInputs = with pkgs; [meson llvmPackages_17.libllvm clang-tools_17 ninja];
  }
