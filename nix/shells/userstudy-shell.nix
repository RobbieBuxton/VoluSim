{ pkgs }:
pkgs.mkShell {
  name = "userstudy-shell";

  packages = with pkgs; [   
    python311Packages.pymongo
    python311Packages.click
  ];

  shellHook =
    ''
    export PS1="\e[0;33m[\u@\h \W]\$ \e[m"

    # Define a function to run your Python script
    function userstudy() {
      sudo LD_LIBRARY_PATH=$LD_LIBRARY_PATH PYTHONPATH=$PYTHONPATH python3 userstudy/user_study.py "$@"
    }

    echo "Type 'userstudy' to start the user study application."
    '';
}
