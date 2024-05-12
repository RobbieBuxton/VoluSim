{ pkgs }:
pkgs.mkShell {
  name = "mongodb-shell";

  packages = with pkgs; [   
    python311Packages.pymongo
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

    mkdir -p ${databasePath}

    mongod --dbpath ${databasePath} --fork --logpath $(pwd)/mongodb.log

    trap 'echo "Shutting down MongoDB..."; mongod --shutdown --dbpath ${databasePath} ' EXIT

    echo "MongoDB started. Use mongo to connect. close the shell to stop the database."
    
    mongodb-compass
    '';
}

