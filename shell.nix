{ nixpkgs ? (import <nixpkgs> {}).fetchFromGitHub {
    owner  = "NixOS";
    repo   = "nixpkgs";
    rev    = "309f965c9120afd24f406bf1d8ca5cb342294431";
    sha256 = "1i1c84l9a328s3gg3gszhmw06zap461iylpxlxwh1c9105cpyvj9";
  }}:

with import nixpkgs {};

libcxxStdenv.mkDerivation rec {
  name = "breakov";
  buildInputs = [
    ccache
    cmake
    clang
    libcxx
    libcxxabi
    llvm
    ninja
  ];
}
