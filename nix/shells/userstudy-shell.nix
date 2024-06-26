{ pkgs }:
pkgs.mkShell {
  name = "userstudy-shell";
  packages = with pkgs; [   
    python311Packages.pymongo
    python311Packages.click
    python311Packages.bson
    python311Packages.tabulate
    python311Packages.pyvista
    python311Packages.kivy
    python311Packages.pandas
    python311Packages.scipy
    python311Packages.simpleaudio
    python311Packages.statsmodels
    python311Packages.seaborn
    python311Packages.matplotlib
    python311Packages.openpyxl
    python311Packages.trimesh
    (texlive.combine {
            inherit (texlive)
              scheme-full
              ;
          })
  ];

  shellHook =
    ''
    export PS1="\e[0;33m[\u@\h \W]\$ \e[m"

    # Define a function to run your Python script
    function study() {
      sudo -E LD_LIBRARY_PATH=$LD_LIBRARY_PATH PYTHONPATH=$PYTHONPATH python3 userstudy/cli.py "$@"
    }

    echo "Type 'study' to start the user study application."
    '';
}
