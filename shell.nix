{ pkgs ? import <nixpkgs> {} }:

let
  clangd-wrapped = pkgs.writeShellScriptBin "clangd" ''
    exec ${pkgs.clang-tools}/bin/clangd \
      --query-driver="/nix/store/*/bin/clang++,/nix/store/*/bin/g++" \
      "$@"
  '';
in
pkgs.mkShell {
  packages = with pkgs; [
    clangd-wrapped   # Must be first in PATH to shadow clang-tools' clangd
    gcc
    clang
    clang-tools
    cmake
    ninja
    git
    curl
    gdb
  ];

  shellHook = ''
    export CC=clang
    export CXX=clang++
  '';
}
