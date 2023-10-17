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
    # Development shell used by running "nix develop"
    devShells.${system}.default = pkgs.mkShell rec {
      name = "volumetricSim";

      packages = with pkgs; [ 
        glxinfo
        lshw
        python310Packages.glad2
      ];
    };

    packages.${system}.default = pkgs.stdenv.mkDerivation {
      pname = "volumetricSim";
      version = "0.0.1";

      src = ./src; 

      packages = with pkgs; [ 
        python310Packages.glad2
      ];

      buildPhase = ''
        gcc main.cpp -lstdc++ -o volumetricSim
      '';

      installPhase = ''
        mkdir -p $out/bin
        cp volumetricSim $out/bin
      '';
    };

    
  };
}
