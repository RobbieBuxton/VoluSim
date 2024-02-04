{ pkgs, k4apkgs, tinyobjloaderSrc }:
{
  default = pkgs.cudaPackages.backendStdenv.mkDerivation {
    pname = "volumetricSim";
    version = "0.0.1";

    enableParallelBuilding = true;

    src = ./.;

    buildInputs = [
      pkgs.python310Packages.glad2
      pkgs.bzip2
      pkgs.glxinfo

      # Libs
      pkgs.assimp
      pkgs.dlib
      pkgs.libpng
      pkgs.libjpeg
      pkgs.opencv
      pkgs.cudatoolkit
      pkgs.cudaPackages.cudnn
      pkgs.linuxPackages.nvidia_x11
      k4apkgs.libk4a-dev
      k4apkgs.k4a-tools
      pkgs.glfw
      pkgs.glm
      pkgs.stb
      pkgs.xorg.libX11
      pkgs.xorg.libXrandr
      pkgs.xorg.libXi
      pkgs.udev
      pkgs.mkl
    ];

    configurePhase =
      let
        faceLandmarksSrc = builtins.fetchurl {
          url =
            "http://dlib.net/files/shape_predictor_5_face_landmarks.dat.bz2";
          sha256 =
            "sha256:0wm4bbwnja7ik7r28pv00qrl3i1h6811zkgnjfvzv7jwpyz7ny3f";
        };
        mmodHumanFaceDetectorSrc = builtins.fetchurl {
          url = "http://dlib.net/files/mmod_human_face_detector.dat.bz2";
          sha256 =
            "sha256:15g6nm3zpay80a2qch9y81h55z972bk465m7dh1j45mcjx4cm3hw";
        };
      in
      ''
        cp ${tinyobjloaderSrc} ./include/tiny_obj_loader.h
        cd data
        cp ${faceLandmarksSrc} ./shape_predictor_5_face_landmarks.dat.bz2
        bzip2 -d ./shape_predictor_5_face_landmarks.dat.bz2
        cp ${mmodHumanFaceDetectorSrc} ./mmod_human_face_detector.dat.bz2
        bzip2 -d ./mmod_human_face_detector.dat.bz2       
        cd ..
      '';

    buildPhase =
      let
        gladBuildDir = "build/glad";
        flags = [
          "-Ofast"
          "-Wall"
          # "-fsanitize=address"
          # "-fsanitize=undefined"
          # "-fsanitize=thread"
          "-march=skylake" # This is platform specific need to find a way to optimise this
        ];
        sources = [
          "src/main.cpp"
          "src/display.cpp"
          "src/tracker.cpp"
          "src/model.cpp"
          "src/mesh.cpp"
          "${gladBuildDir}/src/gl.c"
        ];
        libs = [
          "-L ${k4apkgs.libk4a-dev}/lib/x86_64-linux-gnu"
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
          "-lcudnn"
          "-lcublas"
          "-lcurand"
          "-lcusolver"
          "-lmkl_intel_lp64"
        ];
        headers = [
          "-I ${pkgs.dlib}/include"
          "-I ${pkgs.opencv}/include/opencv4"
          "-I ${gladBuildDir}/include"
          "-I ${k4apkgs.libk4a-dev}/include"
          "-I include"
        ];
        macros = [ ''-DPACKAGE_PATH=\"$out\"'' ];
        openGLVersion =
          "glxinfo | grep -oP '(?<=OpenGL version string: )[0-9]+.?[0-9]'";
      in
      ''
        glad --api gl:core=`${openGLVersion}` --out-path ${gladBuildDir} --reproducible 
        g++ ${
          pkgs.lib.strings.concatStringsSep " "
          (macros ++ flags ++ sources ++ libs ++ headers)
        } -o volumetricSim
      '';

    installPhase = ''
      mkdir -p $out/bin
      cp volumetricSim $out/bin
      cp -a data $out/data
    '';
  };
}
