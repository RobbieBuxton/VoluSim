{
  description = "A volumetric screen simulation";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/23.05";
  };

  outputs = { self, nixpkgs }: 
  let 
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in
  {
    # Development shell used by running "nix develop" for debugging
    devShells.${system}.default = pkgs.mkShell rec {
      name = "volumetricSim";

      packages = with pkgs; [ 
        glxinfo
      ];
    };

    packages.${system}.default = pkgs.stdenv.mkDerivation {
      pname = "volumetricSim";
      version = "0.0.1";

      src = ./.; 

      buildInputs = with pkgs; [
        python310Packages.glad2
        glxinfo

        # Libs
        glfw
        xorg.libX11
        xorg.libXrandr
        xorg.libXi
      ];

      buildPhase = let
        gccBuildLibs = "-lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl";
        openGLVersion = "glxinfo | grep -oP '(?<=OpenGL version string: )[0-9]+.?[0-9]'";
        gladBuildDir = "glad";
      in ''
        glad --api gl:core=`${openGLVersion}` --out-path ${gladBuildDir} --reproducible 
        gcc ${gccBuildLibs} src/main.cpp ${gladBuildDir}/src/gl.c -I ${gladBuildDir}/include -lstdc++ -o volumetricSim 
      '';

      installPhase = ''
        mkdir -p $out/bin
        cp volumetricSim $out/bin
      '';
    };

    
  };
}
