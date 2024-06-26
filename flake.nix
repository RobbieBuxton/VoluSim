{
  description = "A volumetric screen simulation";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/23.11";
    k4a.url = "github:RobbieBuxton/azure-kinect-nix/a1b7f36314b8c52e7ea6e8cded280abdb8c7d299";
    libmediapipe.url = "github:RobbieBuxton/libmediapipe-nix";
  };

  outputs = { self, nixpkgs, k4a, libmediapipe }:
    let
      system = "x86_64-linux";

      # Fetch the tinyobjloader header file
      tolHeader = builtins.fetchurl {
        url = "https://raw.githubusercontent.com/tinyobjloader/tinyobjloader/release/tiny_obj_loader.h";
        sha256 = "sha256:1gjybpwhcsqwb468bhkvw785vclcprfa8qpghjg0xalwl8rhjnm5";
      };

      jsonHeader = builtins.fetchurl {
        url = "https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp";
        sha256 = "sha256:1jna4dgk3a10p04m7s5adg8gsm304is66j16b45q5yb7ilq3fjsv";
      };

      # Import the k4a package set (Azure Kinect Packages) 
      k4apkgs = k4a.packages.${system};
      libmediapipepkg = libmediapipe.packages.${system}.default;
      pkgs = import nixpkgs {
        inherit system;
        overlays = [ (import ./nix/overlays/overlays.nix) ];
        config.cudaSupport = true;
        config.allowUnfree = true;
      };
    in
    {
      # Development shell used by running "nix develop".
      # It will configure vscode settings for finding the correct c++ libs for Intellisense
      devShells.${system} = {
        default = import ./nix/shells/dev-shell.nix {
          inherit pkgs k4apkgs tolHeader jsonHeader libmediapipepkg;
        };

        start-mongodb = import ./nix/shells/mongodb-shell.nix {
          inherit pkgs;
        };

        userstudy = import ./nix/shells/userstudy-shell.nix {
          inherit pkgs;
        };
      };
      # The volumetric screen simulation package
      packages.${system} = pkgs.callPackage ./nix/packages/package.nix {
        inherit pkgs k4apkgs tolHeader jsonHeader libmediapipepkg;
      };
    };
}

