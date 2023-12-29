{
  description = "A volumetric screen simulation";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/23.11";
    k4a.url = "github:RobbieBuxton/k4a-nix";
  };

  outputs = { self, nixpkgs, k4a}: 
  let 
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; config.cudaSupport = true; config.allowUnfree = true; };
    opencv = (pkgs.opencv.override { enableGtk2 = true; });
    dlib = (pkgs.dlib.override { guiSupport = true; });
    concatStringsWithSpace = pkgs.lib.strings.concatStringsSep " ";
  in
  {
  # Development shell used by running "nix develop".
  # It will configure vscode settings for finding the correct c++ libs for Intellisense
  devShells.${system}.default = pkgs.mkShell rec {
    name = "volumetricSim";

    enableParallelBuilding = true;

    packages = [ 
      k4a.packages.${system}.libk4a-dev
      k4a.packages.${system}.k4a-tools

      #Setup and window
      pkgs.gcc
      pkgs.python310Packages.glad2
      pkgs.glxinfo
      pkgs.killall
      pkgs.jq
      
      # Libs
      dlib
      opencv
      pkgs.cudatoolkit # Both as sus
      pkgs.linuxPackages.nvidia_x11 # Both as sus
      pkgs.glfw
      pkgs.glm
      pkgs.stb
      pkgs.xorg.libX11
      pkgs.xorg.libXrandr
      pkgs.xorg.libXi

      # Debug 
      pkgs.dpkg
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
              "${opencv}/include/opencv4"
              "${dlib}/include"
              "${k4a.packages.${system}.libk4a-dev}/include"
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

        enableParallelBuilding = true;

        src = ./.; 

        buildInputs = [
          pkgs.python310Packages.glad2
          pkgs.bzip2
          pkgs.glxinfo

          # Libs
          dlib
          pkgs.libpng
          pkgs.libjpeg
          opencv
          pkgs.cudatoolkit # Both as sus
          pkgs.linuxPackages.nvidia_x11 # Both as sus
          k4a.packages.${system}.libk4a-dev
          k4a.packages.${system}.k4a-tools
          pkgs.glfw
          pkgs.glm
          pkgs.stb
          pkgs.xorg.libX11
          pkgs.xorg.libXrandr
          pkgs.xorg.libXi
          pkgs.udev
        ];

        configurePhase = let
          faceLandmarksSrc = builtins.fetchurl {
            url  = "http://dlib.net/files/shape_predictor_5_face_landmarks.dat.bz2";
            sha256 = "sha256:0wm4bbwnja7ik7r28pv00qrl3i1h6811zkgnjfvzv7jwpyz7ny3f";
          }; 
        in  ''
          cd data
          cp ${faceLandmarksSrc} ./shape_predictor_5_face_landmarks.dat.bz2
          bzip2 -d ./shape_predictor_5_face_landmarks.dat.bz2
          cd ..
        '';

        buildPhase = let
          gladBuildDir = "build/glad";
          flags = [
            "-Ofast"
            "-Wall"
          ];
          sources = [
            "src/main.cpp"
            "src/display.cpp"
            "src/tracker.cpp"
            "${gladBuildDir}/src/gl.c"
          ];
          libs = [ 
            "-L ${k4a.packages.${system}.libk4a-dev}/lib/x86_64-linux-gnu"
            "-lglfw"
            "-lGL"
            "-lX11"
            "-lpthread"
            "-lXrandr"
            "-lXi"
            "-ldl"
            "-lm"
            "-ludev"
            "-lk4a"
            "-lopencv_core" 
            "-lopencv_imgproc"
            "-lopencv_highgui"
            "-lopencv_imgcodecs"
            "-lopencv_cudafeatures2d"
            "-lopencv_cudafilters"
            "-lopencv_cudawarping"
            "-lopencv_features2d"
            "-lopencv_flann" 
            "-ldlib"
            "-lcudart"
            "-lcuda" 
          ];
          headers = [
            "-I ${dlib}/include"
            "-I ${opencv}/include/opencv4"
            "-I ${gladBuildDir}/include"
            "-I ${k4a.packages.${system}.libk4a-dev}/include"
            "-I include"
          ];
          macros = [
            "-DPACKAGE_PATH=\\\"\$out\\\""
          ];
          openGLVersion = "glxinfo | grep -oP '(?<=OpenGL version string: )[0-9]+.?[0-9]'";
        in ''
          glad --api gl:core=`${openGLVersion}` --out-path ${gladBuildDir} --reproducible 
          g++ ${concatStringsWithSpace (macros ++ flags ++ sources ++ libs ++ headers)} -o volumetricSim
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp volumetricSim $out/bin
          cp -a data $out/data
        '';
      };
    };
  };
}
