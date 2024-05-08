{ pkgs }:
pkgs.mkShell {
  name = "mongodb-shell";

  packages = with pkgs; [   
    mongodb
    mongodb-compass
  ];

  shellHook =
  let
    databasePath = ''$(pwd)/userstudy/database'';
  in 
    ''
    export PS1="\e[0;34m[\u@\h \W]\$ \e[m"
    echo "Starting MongoDB with data directory in the current working directory..."
    # Make sure the database directory exists
    mkdir -p ${databasePath}
    # Start mongod as a background process
    mongod --dbpath ${databasePath} --fork --logpath $(pwd)/mongodb.log

    # Trap the EXIT signal to cleanup on shell exit
    trap 'echo "Shutting down MongoDB..."; mongod --shutdown --dbpath ${databasePath} ' EXIT

    echo "MongoDB started. Use mongo to connect. close the shell to stop the database."
    mongodb-compass
    '';
}

