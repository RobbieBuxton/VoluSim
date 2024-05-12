self: super: {

  # Sets our default blas and lapack provider to Intels MKL
  blas = super.blas.override { blasProvider = self.mkl; };
  lapack = super.lapack.override { lapackProvider = self.mkl; };

  # Enables gtk2 support in opencv
  opencv = (super.opencv.override { enableGtk2 = true; });

  # Patch dlib to use CUDA (I fixed this upstream and it should be in the next release) https://github.com/NixOS/nixpkgs/pull/279927
  dlib = (
    let
      cudaDeps =
        if super.config.cudaSupport then [
          super.cudaPackages.cudatoolkit
          super.cudaPackages.cudnn
          self.fftw
          self.blas
        ] else
          [ ];
    in
    super.dlib.overrideAttrs (oldAttrs: {
      buildInputs = oldAttrs.buildInputs ++ cudaDeps;

      # Use CUDA stdenv if CUDA is enabled
      stdenv =
        if super.config.cudaSupport then
          super.cudaPackages.backendStdenv
        else
          oldAttrs.stdenv;
    })
  ).override {
    guiSupport = true;
    cudaSupport = true;
    avxSupport = true;
    sse4Support = true;
  };
}
