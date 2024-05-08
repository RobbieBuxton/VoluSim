#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
#include <cstdlib>
#include <filesystem>

#ifndef PACKAGE_PATH
#define PACKAGE_PATH ""
#endif


class FileSystem
{
private:
  typedef std::string (*Builder)(const std::string &path);

public:
  static std::string getPath(const std::string &path)
  {
    static std::string (*pathBuilder)(std::string const &) = getPathBuilder();
    return (*pathBuilder)(path);
  }

private:
  static std::string const &getRoot()
  {
    static std::string root = PACKAGE_PATH;
    return root;
  }

  static Builder getPathBuilder()
  {
    if (getRoot() != "")
      return &FileSystem::getPathRelativeRoot;
    else
    {
      std::cout << "ERROR: no package path was provided during compilation" << std::endl;
      abort();
    }
  }

  static std::string getPathRelativeRoot(const std::string &path)
  {
    return getRoot() + std::string("/") + path;
  }
};

#endif