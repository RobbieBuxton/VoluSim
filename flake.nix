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
        python310Packages.glad2
        glxinfo
      ];
    };

    packages.${system}.default = pkgs.stdenv.mkDerivation {
      pname = "volumetricSim";
      version = "0.0.1";

      src = ./.; 

      buildInputs = with pkgs; [ 
        python310
        python310Packages.glad2
        
        # Libs
        glfw
        xorg.libX11
        xorg.libXrandr
        xorg.libXi
      ];

      # glad --api gl:core=4.6 --out-path glad
      buildPhase = ''
        gcc -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl src/main.cpp glad/src/gl.c -I glad/include -lstdc++ -o volumetricSim 
      '';

      installPhase = ''
        mkdir -p $out/bin
        cp volumetricSim $out/bin
      '';
    };

    
  };
}
