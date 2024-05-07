{ pkgs, k4apkgs, tolHeader, jsonHeader, libmediapipepkg }:
pkgs.mkShell {
  name = "volumetricSim";

  packages = with pkgs; [   
    #Remove this if you want the shell to compile in a reasonable amount of time.
    python311Packages.pyvista
    python311Packages.pymongo
    python311Packages.glad2
    glxinfo
    killall
    jq
    nil
    cntr #For debugging in theory
  ] ++ (with k4apkgs; [
    k4a-tools
  ]);

  # This is a hook that is run when the shell is entered (it creates the .vscode directory and the c_cpp_properties.json file for vscode to use for intellisense)
  shellHook =
    let
      vscodeDir = ".vscode";
      vscodeCppConfig = {
        configurations = [{
          name = "Linux";
          includePath = [
            "include"
            ".vscode/include"
            ".vscode/glad/include"
            "${pkgs.glfw}/include"
            "${pkgs.glm}/include"
            "${pkgs.stb}/include"
            "${pkgs.opencv}/include/opencv4"
            "${pkgs.dlib}/include"
            "${k4apkgs.libk4a-dev}/include"
            "${libmediapipepkg}/include"
          ];
          defines = [ ];
          compilerPath = "${pkgs.gcc}/bin/gcc";
          cStandard = "c17";
          cppStandard = "gnu++17";
          intelliSenseMode = "linux-gcc-x64";
        }];
        version = 4;
      };
      openGLVersion =
        "glxinfo | grep -oP '(?<=OpenGL version string: )[0-9]+.?[0-9]'";
      gladBuildDir = "${vscodeDir}/glad";
    in
    ''
      export PS1="\e[0;31m[\u@\h \W]\$ \e[m "
      glad --api gl:core=`${openGLVersion}` --out-path ${gladBuildDir} --reproducible --quiet
      jq --indent 4 -n '${
        builtins.toJSON vscodeCppConfig
      }' >> ${vscodeDir}/c_cpp_properties.json
      mkdir -p ${vscodeDir}/include && cp ${tolHeader} ./${vscodeDir}/include/tiny_obj_loader.h
      cp ${jsonHeader} ./${vscodeDir}/include/json.hpp
      trap "rm ${vscodeDir} -rf;" exit 
    '';
}
