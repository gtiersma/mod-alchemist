#include "controller.h"

#include "constants.cpp"

#include "ui/ui_error.h"

#include <tesla.hpp>

/**
 * Load all groups from the game folder
 */
std::vector<std::string> Controller::loadGroups() {
  this->openSdCardIfNeeded();
  return this->listSubfolderNames(this->getGamePath());
}

/**
 * Load all source options within the specified group
 */
std::vector<std::string> Controller::loadSources(const std::string& group) {
  this->openSdCardIfNeeded();

  std::vector sources = this->listSubfolderNames(this->getGroupPath(group));

  for (std::string& source : sources) {
    source = this->trimRating(source);
  }

  return sources;
}

/**
 * Load all mod options that could be activated for the moddable source in the group
 */
std::vector<Mod> Controller::loadMods(const std::string& source, const std::string& group) {
  this->openSdCardIfNeeded();

  std::vector<Mod> mods = std::vector<Mod>();
  std::string sourcePath = this->getSourcePath(group, source);

  FsDir dir = this->openDirectory(sourcePath, FsDirOpenMode_ReadDirs);

  FsDirectoryEntry entry;
  s64 readCount;
  while (R_SUCCEEDED(fsDirRead(&dir, &readCount, 1, &entry))) {
    if (entry.type == FsDirEntryType_Dir) {
      mods.push_back({
        name: entry.name,
        rating: this->getRatingFromName(entry.name)
      });
    }
  }

  fsDirClose(&dir);

  return mods;
}

/**
 * Gets the mod currently activated for the moddable source in the group
 *
 * Returns an empty string if no mod is active and vanilla files are being used
 */
std::string_view Controller::getActiveMod(const std::string& source, const std::string& group) {
  this->openSdCardIfNeeded();

  // Open to the correct source directory
  FsDir sourceDir = this->openDirectory(this->getSourcePath(group, source), FsDirOpenMode_ReadFiles);

  // Find the .txt file in the directory. The name would be the active mod:
  FsDirectoryEntry entry;
  s64 readCount;
  std::string_view activeMod = "";
  std::string_view name;
  while (R_SUCCEEDED(fsDirRead(&sourceDir, &readCount, 1, &entry))) {
    if (entry.type == FsDirEntryType_File) {
      name = entry.name;
      if (name.find(TXT_EXT) != std::string::npos) {
        activeMod = name.substr(0, name.size() - TXT_EXT.size());
        break;
      }
    }
  }
  fsDirClose(&sourceDir);

  return activeMod;
}

/**
 * Activates the specified mod, moving all its files into the atmosphere folder for the game
 * 
 * Make sure to deactivate any existing active mod for this source if there is one
 */
void Controller::activateMod(const std::string& source, const std::string& group, const std::string& mod) {
  this->openSdCardIfNeeded();

  // Path to the "mod" folder in alchemy's directory:
  std::string modPath = this->getModPath(group, source, mod);
  // Path of the txt file for the active mod:
  std::string movedFilesFilePath = this->getMovedFilesListFilePath(group, source, mod);

  FsDir dir = this->openDirectory(modPath, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles);

  // Used for "storing" data of parent folders when traversing deeper into the hierarchy:
  std::vector<u64> entryCounts;
  std::vector<u64> readCounts;

  // The directory we are currently at:
  std::string currentDirectory = modPath;

  // Position in the txt file where we should write the next file path:
  s64 txtOffset = 0;

  // Counts for entries of currentDirectory:
  s64 currentReadCount = 0;
  s64 currentEntryCount;
  fsDirGetEntryCount(&dir, &currentEntryCount);

  FsDirectoryEntry entry;

  // The entry path we will look at in the following iteration in the loop:
  std::string nextPath;
  std::string nextAtmoPath;

  // Set to true once we have traversed the entire folder directory within the mod folder:
  bool isDone = false;
  while (!isDone) {

    // If we have analyzed all entries in the current directory, navigate back up to where we left off in the parent:
    if (currentEntryCount == currentReadCount) {

      // If there's nothing left in our count storage, we've navigated everything, so we're done:
      if (entryCounts.size() == 0) {
        isDone = true;
        break;
      } else {
        // Otherwise, let's get back the count data of where we left off in the parent:
        currentEntryCount = entryCounts.back();
        currentReadCount = readCounts.back();
        entryCounts.pop_back();
        readCounts.pop_back();

        // Remove the string portion after the last '/' to get the parent's path:
        std::size_t lastSlashIndex = currentDirectory.rfind('/');
        currentDirectory = currentDirectory.substr(0, lastSlashIndex);
        this->changeDirectory(dir, currentDirectory, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles);
      }
    }

    if (R_SUCCEEDED(fsDirRead(&dir, &currentReadCount, 1, &entry))) {

      nextPath = currentDirectory + "/" + entry.name;

      // If the next entry is a file, we will move it and record it as moved as long as there isn't a conflict:
      if (entry.type == FsDirEntryType_File) {
        nextAtmoPath = this->getAtmosphereModPath(modPath.size(), nextPath);

        if (this->doesFileNotExist(nextAtmoPath)) {
          this->recordFile(nextAtmoPath + "\n", movedFilesFilePath, txtOffset);
          this->moveFile(nextPath, nextAtmoPath);
        }
      // If the next entry is a folder, we will traverse within it:
      } else if (entry.type == FsDirEntryType_Dir) {

        // Add the current counts to the storage:
        entryCounts.push_back(currentEntryCount);
        readCounts.push_back(currentReadCount);

        currentDirectory = nextPath;
        this->changeDirectory(dir, currentDirectory, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles);

        // Have the counts ready for the next directory:
        fsDirGetEntryCount(&dir, &currentEntryCount);
        currentReadCount = 0;
      }
    }
  }

  fsDirClose(&dir);
}

/**
 * Deactivates the currently active mod, restoring the moddable source to its vanilla state
 */
void Controller::deactivateMod(const std::string& source, const std::string& group) {
  this->openSdCardIfNeeded();

  std::string activeMod(this->getActiveMod(source, group));

  // Path of the txt file for the active mod:
  std::string movedFilesListPath = this->getMovedFilesListFilePath(group, source, activeMod);

  // Try to open the active mod's txt file to get the list of files that were moved to atmosphere's folder:
  FsFile movedFilesList;
  if (R_FAILED(fsFsOpenFile(&this->sdSystem, movedFilesListPath.c_str(), FsOpenMode_Read, &movedFilesList))) {
    tsl::changeTo<GuiError>("Error: Failed to open file at " + movedFilesListPath);
    abort();
  }

  s64 fileSize;
  if (R_FAILED(fsFileGetSize(&movedFilesList, &fileSize))) {
    tsl::changeTo<GuiError>("Error: Failed to get file size at " + movedFilesListPath);
    abort();
  }

  // Used to stream the file a small number of bytes at a time to minimize memory consumption:
  s64 offset = 0;
  char* buffer = new char[FILE_LIST_BUFFER_SIZE];
  std::string pathBuilder = "";

  std::string atmoPath;
  std::string alchemyPath;

  // As long as there is still data in the file:
  while (offset < fileSize) {

    // Read some of the text into our buffer:
    if (R_FAILED(fsFileRead(&movedFilesList, offset, buffer, FILE_LIST_BUFFER_SIZE, FsReadOption_None, nullptr))) {
      tsl::changeTo<GuiError>("Error: Failed to read file at " + movedFilesListPath);
      abort();
    }

    // Append it to the string we're using to build the next path:
    pathBuilder += std::string_view(buffer, FILE_LIST_BUFFER_SIZE);

    // If the path builder got a new line character from the buffer, we have a full path:
    std::size_t newLinePos = pathBuilder.find('\n');
    if (newLinePos != std::string::npos) {
      // Trim the new line and any characters that were gathered after it to get the cleaned atmosphere file path:
      atmoPath = pathBuilder.substr(0, newLinePos);

      // Move any characters gathered after the new line to the pathBuilder string for the next path:
      pathBuilder = pathBuilder.substr(newLinePos + 1);

      // The file's original location can be built by replacing the atmosphere portion of the path with alchemy's portion:
      alchemyPath = this->getModPath(group, source, activeMod) + atmoPath.substr(this->getAtmospherePath().size());

      this->moveFile(atmoPath, alchemyPath);
    }

    offset += FILE_LIST_BUFFER_SIZE;
  }

  delete[] buffer;

  fsFileClose(&movedFilesList);

  // Once all the files have been returned, delete the txt list:
  fsFsDeleteFile(&this->sdSystem, movedFilesListPath.c_str());
}

u8 Controller::getModlessRating(const std::string& source, const std::string& group) {
  this->openSdCardIfNeeded();

  u8 rating = DEFAULT_RATING;
  std::string groupPath = this->getGroupPath(group);

  FsDir dir = this->openDirectory(groupPath, FsDirOpenMode_ReadDirs);

  FsDirectoryEntry entry;
  s64 readCount;
  while (R_SUCCEEDED(fsDirRead(&dir, &readCount, 1, &entry))) {
    if (entry.type == FsDirEntryType_Dir && source == this->trimRating(entry.name)) {
      rating = this->getRatingFromName(entry.name);
      break;
    }
  }

  fsDirClose(&dir);

  return rating;
}

void Controller::saveModlessRating(const u8& rating, const std::string& source, const std::string& group) {
  this->openSdCardIfNeeded();

  std::string groupPath = this->getGroupPath(group);
  FsDir dir = this->openDirectory(groupPath, FsDirOpenMode_ReadDirs);

  FsDirectoryEntry entry;
  s64 readCount;
  while (R_SUCCEEDED(fsDirRead(&dir, &readCount, 1, &entry))) {
    std::string ratinglessName = this->trimRating(entry.name);
    if (entry.type == FsDirEntryType_Dir && source == ratinglessName) {
      std::string oldPath = groupPath + "/" + entry.name;
      std::string newPath = groupPath + "/" + ratinglessName;

      if (rating != DEFAULT_RATING) {
        newPath += "_" + std::to_string(rating);
      }

      if (R_FAILED(fsFsRenameDirectory(&this->sdSystem, oldPath.c_str(), newPath.c_str()))) {
        tsl::changeTo<GuiError>("Error: Failed to rename directory from " + oldPath + " to " + newPath);
        abort();
      }

      break;
    }
  }

  fsDirClose(&dir);
}

void Controller::saveRatings(std::vector<Mod> mods, const std::string& source, const std::string& group) {
  this->openSdCardIfNeeded();

  std::string sourcePath = this->getSourcePath(group, source);
  FsDir dir = this->openDirectory(sourcePath, FsDirOpenMode_ReadDirs);

  FsDirectoryEntry entry;
  s64 readCount;
  while (R_SUCCEEDED(fsDirRead(&dir, &readCount, 1, &entry))) {
    std::string ratinglessName = this->trimRating(entry.name);
    if (entry.type == FsDirEntryType_Dir && mods.back().name == ratinglessName) {
      std::string oldPath = sourcePath + "/" + entry.name;
      std::string newPath = sourcePath + "/" + ratinglessName;

      if (mods.back().rating != DEFAULT_RATING) {
        newPath += "_" + std::to_string(mods.back().rating);
      }

      if (R_FAILED(fsFsRenameDirectory(&this->sdSystem, oldPath.c_str(), newPath.c_str()))) {
        tsl::changeTo<GuiError>("Error: Failed to rename directory from " + oldPath + " to " + newPath);
        abort();
      }
      
      mods.pop_back();
    }
  }

  fsDirClose(&dir);
}

/**
 * Unmount SD card when destroyed 
 */
Controller::~Controller() {
  if (this->isSdCardOpen) {
    fsFsClose(&this->sdSystem);
  }
}

/*
 * Mounts the SD card if it hasn't been mounted yet
 */
void Controller::openSdCardIfNeeded() {
    if (this->isSdCardOpen) {
        return;
    }

    this->isSdCardOpen = true;
    Result result;
    result = fsOpenSdCardFileSystem(&this->sdSystem);
    if (R_FAILED(result)) {
        tsl::changeTo<GuiError>("Error: Failed to open SD card filesystem.");
        abort();
    }
}

/**
 * Creates a new open FsDir object for the specified path
 * 
 * Don't forget to close when done
 */
FsDir Controller::openDirectory(const std::string& path, u32 mode) {
  FsDir dir;
  this->changeDirectory(dir, path, mode);
  return dir;
}

/**
 * Changes an FsDir instance to the specified path
 */
void Controller::changeDirectory(FsDir& dir, const std::string& path, u32 mode) {
  if (R_FAILED(fsFsOpenDirectory(&this->sdSystem, path.c_str(), mode, &dir))) {
    tsl::changeTo<GuiError>("Error: Failed to open directory at " + path);
    abort();
  }
}

bool Controller::doesFileExist(const std::string& path) {
  FsDir dir;
  // If the file can be opened, it exists:
  bool exists = R_SUCCEEDED(fsFsOpenDirectory(&this->sdSystem, path.c_str(), FsDirOpenMode_ReadFiles, &dir));
  fsDirClose(&dir);
  return exists;
}

bool Controller::doesFileNotExist(const std::string& path) {
  return !this->doesFileExist(path);
}

/**
 * Gets a vector of all folder names that are directly within the specified path
 */
std::vector<std::string> Controller::listSubfolderNames(const std::string& path) {
  std::vector<std::string> subfolders;

  FsDir dir = this->openDirectory(path, FsDirOpenMode_ReadDirs);

  FsDirectoryEntry entry;
  s64 readCount;
  while (R_SUCCEEDED(fsDirRead(&dir, &readCount, 1, &entry))) {
    if (entry.type == FsDirEntryType_Dir) {
      subfolders.push_back(entry.name);
    }
  }

  fsDirClose(&dir);

  return subfolders;
}

/**
 * Records the line parameter (should end in \n) for a file being moved in modPath
 * "line" is appended to the movedFilesListPath file 
 * 
 * offset is expected to be at the end of the file,
 * and it's updated to the new position at the end of file
 */
void Controller::recordFile(const std::string& line, const std::string& movedFilesListPath, s64& offset) {

  // If the file hasn't been created yet, create it:
  if (this->doesFileNotExist(movedFilesListPath)) {
    if (R_FAILED(fsFsCreateFile(&this->sdSystem, movedFilesListPath.c_str(), 0, 0))) {
      tsl::changeTo<GuiError>("Error: Failed to create file at " + movedFilesListPath);
      abort();
    }
  }
  
  // Open the file:
  FsFile movedListFile;
  if (R_FAILED(fsFsOpenFile(&this->sdSystem, movedFilesListPath.c_str(), FsOpenMode_Write, &movedListFile))) {
    tsl::changeTo<GuiError>("Error: Failed to open file at " + movedFilesListPath);
    abort();
  }

  // Write the path to the end of the list:
  if (R_FAILED(fsFileWrite(&movedListFile, offset, line.c_str(), line.size(), FsWriteOption_Flush))) {
    tsl::changeTo<GuiError>("Error: Failed to write to file at " + movedFilesListPath);
    abort();
  }
  fsFileClose(&movedListFile);

  // Update the offset to the end of the file:
  offset += line.size();
}

/**
 * Changes the fromPath file parameter's location to what's specified as the toPath parameter
 */
void Controller::moveFile(const std::string& fromPath, const std::string& toPath) {
  if (R_FAILED(fsFsRenameFile(&this->sdSystem, fromPath.c_str(), toPath.c_str()))) {
    tsl::changeTo<GuiError>("Error: Failed to move file from " + fromPath + " to " + toPath);
    abort();
  }
}

/*
 * Gets Mod Alchemist's game directory:
 */
std::string Controller::getGamePath() {
  return ALCHEMIST_PATH + std::to_string(this->titleId);
}

/*
 * Gets the file path for the specified group
 */
std::string Controller::getGroupPath(const std::string& group) {
  return this->getGamePath() + "/" + group;
}

/*
 * Gets the file path for the specified source within the group
 */
std::string Controller::getSourcePath(const std::string& group, const std::string& source) {
  return this->getGroupPath(group) + "/" + source;
}

/*
 * Get the file path for the specified mod within the moddable source
 */
std::string Controller::getModPath(const std::string& group, const std::string& source, const std::string& mod) {
  return this->getSourcePath(group, source) + "/" + mod;
}

/**
 * Gets the game's path that's stored within Atmosphere's directory
 */
std::string Controller::getAtmospherePath() {
  return ATMOSPHERE_PATH + std::to_string(this->titleId);
}

/**
 * Builds the path a mod's file should have once we intend to move it into Atmosphere's folder
 * It is built off of its current path within the Mod Alchemist's directory structure.
 */
std::string Controller::getAtmosphereModPath(std::size_t alchemistModFolderPathSize, const std::string& alchemistModFilePath) {
  return this->getAtmospherePath() + "/" + alchemistModFilePath.substr(alchemistModFolderPathSize);
}

/**
 * Gets the file path for the list of moved files for the specified mod
 * 
 * The file should only exist if the mod is currently active
 */
std::string Controller::getMovedFilesListFilePath(const std::string& group, const std::string& source, const std::string& mod) {
  return this->getSourcePath(group, source) + "/" + mod + TXT_EXT;
}

bool Controller::hasRating(const std::string& folderName) {
    if (folderName.size() < 3) {
        return false;
    }

    char thirdLast = folderName[folderName.size() - 3];
    char secondLast = folderName[folderName.size() - 2];
    char last = folderName[folderName.size() - 1];

    return thirdLast == '_' && std::isdigit(secondLast) && std::isdigit(last);
}

std::string Controller::getStringRatingFromName(const std::string& folderName) {
  if (this->hasRating(folderName)) {
    return folderName.substr(folderName.size() - 3);
  }
  return "";
}

u8 Controller::getRatingFromName(const std::string& folderName) {
  std::string rawRating = this->getStringRatingFromName(folderName);
  if (rawRating.empty()) {
    return DEFAULT_RATING;
  }
  return std::stoi(rawRating);
}

std::string Controller::trimRating(const std::string& folderName) {
  if (this->hasRating(folderName)) {
    return folderName.substr(0, folderName.size() - 3);
  }
  return folderName;
}