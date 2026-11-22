{
  description = "stigi: verified compiler seed in C11";

  inputs = {
    nixpkgs.url = "https://nixos.org/channels/nixos-unstable/nixexprs.tar.xz";
    tupu = {
      url = "path:/home/vic/hk/tupu";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, tupu }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAll = f: nixpkgs.lib.genAttrs systems (system: f system nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAll (system: pkgs:
        let
          tupuPkg = tupu.packages.${system}.default;
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "stigi-seed";
            version = "0.1.0";
            src = pkgs.lib.fileset.toSource {
              root = ./.;
              fileset = pkgs.lib.fileset.unions [ ./Justfile ./src ./tests ];
            };
            nativeBuildInputs = [ pkgs.just pkgs.gcc pkgs.clang ];
            dontConfigure = true;
            buildPhase = ''
              runHook preBuild
              just --set tupu_inc "${tupuPkg}/include" --set tupu_lib "${tupuPkg}/lib/libtupu.a" build
              runHook postBuild
            '';
            doCheck = true;
            checkPhase = ''
              runHook preCheck
              export TUPU_BIN="${tupuPkg}/bin/tupu"
              just --set tupu_inc "${tupuPkg}/include" --set tupu_lib "${tupuPkg}/lib/libtupu.a" test
              runHook postCheck
            '';
            installPhase = ''
              runHook preInstall
              install -Dm755 build/stigi-seed $out/bin/stigi-seed
              runHook postInstall
            '';
          };
        });

      devShells = forAll (_system: pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            gcc
            clang
            tinycc
            just
            mcpp
            b3sum
            valgrind
            perf
            hyperfine
          ];
        };
      });
    };
}
