{ pkgs }:
pkgs.mkShell {
  name = "mongodb-shell";

  packages = with pkgs; [   
    mongodb
  ];

  shellHook =
    ''
    export PS1="\e[0;31m[\u@\h \W]\$ \e[m "
    echo "Starting MongoDB with data directory in the current working directory..."
    # Make sure the database directory exists
    mkdir -p $(pwd)/database
    # Start mongod as a background process
    mongod --dbpath $(pwd)/database --fork --logpath $(pwd)/mongodb.log

    # Trap the EXIT signal to cleanup on shell exit
    trap 'echo "Shutting down MongoDB..."; mongod --shutdown --dbpath $(pwd)/database' EXIT

    echo "MongoDB started. Use mongo to connect. close the shell to stop the database."
    '';
}

