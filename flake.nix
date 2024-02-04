{
  description = "A volumetric screen simulation";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/23.11";
    k4a.url = "github:RobbieBuxton/k4a-nix";
  };

  outputs = { self, nixpkgs, k4a }:
    let
      system = "x86_64-linux";

      tinyobjloaderSrc = builtins.fetchurl {
        url = "https://raw.githubusercontent.com/tinyobjloader/tinyobjloader/release/tiny_obj_loader.h";
        sha256 = "sha256:1yhdm96zmpak0gma019xh9d0v1m99vm0akk5qy7v4gyaj0a50690";
      };
      k4apkgs = k4a.packages.${system};

      pkgs = import nixpkgs {
        inherit system;
        overlays = [ (import ./overlays.nix) ];
        config.cudaSupport = true;
        config.allowUnfree = true;
      };
    in
    {
      # Development shell used by running "nix develop".
      # It will configure vscode settings for finding the correct c++ libs for Intellisense
      devShells.${system}.default = import ./shell.nix {
        inherit pkgs k4apkgs tinyobjloaderSrc;
      };

      packages.${system} = pkgs.callPackage ./default.nix {
        inherit pkgs k4apkgs tinyobjloaderSrc;
      };
    };
}

