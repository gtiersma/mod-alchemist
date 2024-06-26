#pragma once

#include <switch.h>

#include <vector>
#include <string>

/**
 * Heper functions related to the filesystem
 */
namespace FsManager {
  extern FsFileSystem sdSystem;

  /**
   * Creates a new open FsDir object for the specified path
   * 
   * Don't forget to close when done
   */
  FsDir openFolder(const std::string& path, const u32& mode);

  /**
   * Changes an FsDir instance to the specified path
   */
  void changeFolder(FsDir& dir, const std::string& path, const u32& mode);

  void createFolderIfNeeded(const std::string& path);

  bool doesFolderExist(const std::string& path);
  bool doesFileExist(const std::string& path);

  /**
   * Gets a vector of all entity names that are directly within the specified path
   * (parsing the name from the folder name)
   */
  std::vector<std::string> listNames(const std::string& path);

  /**
   * Gets the folder name for an entity with the specified name
   */
  std::string getFolderName(const std::string& path, const std::string& name);

  /**
   * Records the line parameter in the filePath
   * 
   * offset is expected to be at the end of the file,
   * and it's updated to the new position at the end of file
   */
  void recordFile(const std::string& line, const std::string& filePath, s64& offset);

  /**
   * Changes the fromPath file parameter's location to what's specified as the toPath parameter
   */
  void moveFile(const std::string& fromPath, const std::string& toPath);

  /**
   * Formats a string as a char array that will work properly as a parameter for libnx's filesystem functions
   */
  char* toPathBuffer(const std::string& path);
}