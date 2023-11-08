{
  description = "A volumetric screen simulation";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/23.05";
    k4a.url = "github:RobbieBuxton/Azure-Kinect-Sensor-SDK/git-submodule-fix";
  };

  outputs = { self, nixpkgs, k4a}: 
  let 
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
    
  in
  {
  # Development shell used by running "nix develop".
  # It will configure vscode settings for finding the correct c++ libs for Intellisense
  devShells.${system}.default = pkgs.mkShell rec {
    name = "volumetricSim";

    packages = [ 
      k4a.packages.${system}.default

      #Setup and windown
      pkgs.gcc
      pkgs.python310Packages.glad2
      pkgs.glxinfo
      pkgs.killall
      pkgs.jq
      
      # Libs
      pkgs.glfw
      pkgs.glm
      pkgs.stb
      pkgs.xorg.libX11
      pkgs.xorg.libXrandr
      pkgs.xorg.libXi
    ];

    shellHook = let
      vscodeDir = ".vscode";
      vscodeCppConfig = { 
        configurations = [
          {
            name = "Linux"; 
            includePath = [
              "include" 
              ".vscode/glad/include"
              "${pkgs.glfw}/include"
              "${pkgs.glm}/include"
              "${pkgs.stb}/include"
            ]; 
            defines = []; 
            compilerPath = "${pkgs.gcc}/bin/gcc";
            cStandard = "c17"; 
            cppStandard = "gnu++17"; 
            intelliSenseMode = "linux-gcc-x64";
          }
        ]; 
        version = 4;
      };
      openGLVersion = "glxinfo | grep -oP '(?<=OpenGL version string: )[0-9]+.?[0-9]'";
      gladBuildDir = "${vscodeDir}/glad";
    in ''
      export PS1="\e[0;31m[\u@\h \W]\$ \e[m "
      glad --api gl:core=`${openGLVersion}` --out-path ${gladBuildDir} --reproducible --quiet
      jq --indent 4 -n '${builtins.toJSON vscodeCppConfig}' >> ${vscodeDir}/c_cpp_properties.json
      trap "rm ${vscodeDir} -rf;" exit 
    '';
  };

    packages.${system} = {
      default = pkgs.stdenv.mkDerivation {
        pname = "volumetricSim";
        version = "0.0.1";

        src = ./.; 

        buildInputs = with pkgs; [
          python310Packages.glad2
          glxinfo

          # Libs
          glfw
          glm
          stb
          xorg.libX11
          xorg.libXrandr
          xorg.libXi
        ];

      currentPath = toString ./.;

        buildPhase = let
          gccBuildLibs = "-lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -lm";
          openGLVersion = "glxinfo | grep -oP '(?<=OpenGL version string: )[0-9]+.?[0-9]'";
          gladBuildDir = "build/glad";
          sourceFiles = "src/main.cpp ${gladBuildDir}/src/gl.c";
          includePaths = "-I ${gladBuildDir}/include -I include";
        in ''
          glad --api gl:core=`${openGLVersion}` --out-path ${gladBuildDir} --reproducible 
          echo ${includePaths}
          g++ ${gccBuildLibs} ${sourceFiles} ${includePaths} -o volumetricSim 
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp volumetricSim $out/bin
        '';
      };

      # k4a =
      # let 
      #   azure-kinect-sensor-sdk = builtins.fetchGit {
      #     url = "https://github.com/RobbieBuxton/Azure-Kinect-Sensor-SDK/";
      #     rev = "eb1e6ddca3952774c07a2a5bfcb55e1241968bda";
      #     submodules = true;
      #   };  
      #   depthengine = builtins.fetchurl {
      #     url = https://packages.microsoft.com/ubuntu/18.04/prod/pool/main/libk/libk4a1.4/libk4a1.4_1.4.1_amd64.deb;
      #     sha256 = "sha256:0ackdiakllmmvlnhpcmj2miix7i2znjyai3a2ck17v8ycj0kzin1";
      #   };   
      # in 
      # pkgs.stdenv.mkDerivation {
      #   pname = "k4aviewer";
      #   version = "1.4.1";

      #   src = azure-kinect-sensor-sdk; 

      #   #Stops cmake killing itself
      #   dontUseCmakeConfigure = true;
      #   dontUseCmakeBuildDir = true; 
      #   nativeBuildInputs = with pkgs; [

    
      #   ];

      #   buildInputs = with pkgs; [
      #     #Try and fix build libs
      #     git
      #     patchelf
      #     gnused

      #     #needed bibs
      #     cmake
      #     pkg-config
      #     ninja
      #     doxygen
      #     python312
      #     nasm
      #     dpkg

      #     #Maybe need libs
      #     glfw
      #     xorg.libX11
      #     xorg.libXrandr
      #     xorg.libXinerama
      #     xorg.libXcursor
      #     openssl_legacy
      #     libsoundio
      #     libusb1
      #     libjpeg
      #     opencv
      #     libuuid
      #   ];

      #   configurePhase = ''
      #     mkdir -p build/bin 
      #     dpkg -x ${depthengine} build/libdepthengine
      #     cp build/libdepthengine/usr/lib/x86_64-linux-gnu/libk4a1.4/libdepthengine.so.2.0 build/bin/
      #     rm -r build/libdepthengine
      #     cd build
      #     cmake .. -GNinja
      #   '';

      #   buildPhase = ''
      #     ninja
      #     export BUILD=`pwd`
      #   '';

      #   installPhase = ''
      #     mkdir -p $out/bin
      #     cp -r bin $out
      #     mkdir -p $out/include
      #     cp -r ../include/k4a $out/include/
      #   '';
      
      #   #Removes any RPATH refrences to the temp build folder used during the configure and install phase
      #   fixupPhase = 
      #   let
      #     removeRPATH = file: path: "patchelf --set-rpath `patchelf --print-rpath ${file} | sed 's@'${path}'@@'` ${file}";
      #   in ''
      #     cd $out/bin
      #     for f in *; do if [[ "$f" =~ .*\..*  ]]; then : ignore;else ${removeRPATH "$f" "$BUILD/bin:"};fi; done
      #     ${removeRPATH "libk4arecord.so.1.4.0" "$BUILD/bin:"}
      #   '';
      # };
    };
  };
}
