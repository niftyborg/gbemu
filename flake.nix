{
  description = "Game Boy Emulator - Pairing Jam";

  inputs.nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-25.05";

  outputs = { self, nixpkgs, ... }@inputs:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      devShells.${system}.default = pkgs.mkShell rec {
        buildInputs = with pkgs; [
          pkg-config
          raylib # NOTE: possible option for window/drawing
        ];
        nativeBuildInputs = with pkgs; [
          gcc
          gnumake
          gdb
          valgrind
        ];
        shellHook = ''
          export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${builtins.toString (pkgs.lib.makeLibraryPath buildInputs)}";
          echo "${builtins.toString (pkgs.lib.makeIncludePath buildInputs)}" > .dumbjump
          sed -i -e "s/:/\n/g" .dumbjump
          sed -i -e "s/^\//+\//" .dumbjump
        '';
      };
    };
}
