#include "controller.h"
#include "constants.h"

#include "ui/ui_error.h"

#include <tesla.hpp>

Controller controller;

void Controller::init() {

  // Get the title ID of the currently running game:
  u64 processId;
  this->tryResult(pmdmntGetApplicationProcessId(&processId), "pmDmntPID");
  this->tryResult(pminfoGetProgramId(&this->titleId, processId), "pmInfoPID");

  this->tryResult(fsOpenSdCardFileSystem(&this->sdSystem), "fsOpenSD");
}

/**
 * Checks if the currenty-running game has a folder set up for Mod Alchemist
 */
bool Controller::doesGameHaveFolder() {
  return this->doesFileExist(this->getGamePath());
}

/**
 * Load all groups and sources from the game folder
 * Key: Group
 * Value: Sources
 */
std::map<std::string, std::vector<std::string>> Controller::loadGroups() {
  std::map<std::string, std::vector<std::string>> groups;

  FsDir dir = this->openDirectory(this->getGamePath(), FsDirOpenMode_ReadDirs);

  FsDirectoryEntry entry;
  s64 entryCount;
  fsDirGetEntryCount(&dir, &entryCount);

  for (s64 i = 0; i < entryCount; i++) {
    if (R_SUCCEEDED(fsDirRead(&dir, &entryCount, 1, &entry))) {
      if (entry.type == FsDirEntryType_Dir) {
        std::string entryName = entry.name;
        std::vector<std::string> sources = this->listSubfolderNames(this->getGroupPath(entryName));
        groups[entryName] = sources;
      }
    }
  }

  return groups;
}

/**
 * Load all mod options that could be activated for the moddable source in the group
 * 
 * @requirement: group and source must be set
 */
std::vector<std::string> Controller::loadMods() {
  return this->listSubfolderNames(this->getSourcePath());
}

/**
 * Gets the mod currently activated for the moddable source in the group
 *
 * Returns an empty string if no mod is active and vanilla files are being used
 * 
 * @requirement: group and source must be set
 */
std::string_view Controller::getActiveMod() {

  // Open to the correct source directory
  FsDir sourceDir = this->openDirectory(this->getSourcePath(), FsDirOpenMode_ReadFiles);

  // Find the .txt file in the directory. The name would be the active mod:
  FsDirectoryEntry entry;
  s64 total;
  fsDirGetEntryCount(&sourceDir, &total);
  std::string_view activeMod = "";
  std::string_view name;

  for (s64 i = 0; i < total; i++) {
    if (R_SUCCEEDED(fsDirRead(&sourceDir, &total, 1, &entry))) {
      if (entry.type == FsDirEntryType_File) {
        name = entry.name;
        if (name.find(TXT_EXT) != std::string::npos) {
          activeMod = name.substr(0, name.size() - TXT_EXT.size());
          break;
        }
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
 * 
 * @requirement: group and source must be set
 */
void Controller::activateMod(const std::string& mod) {
  // Path to the "mod" folder in alchemy's directory:
  std::string modPath = this->getModPath(mod);
  // Path of the txt file for the active mod:
  std::string movedFilesFilePath = this->getMovedFilesListFilePath(mod);

  FsDir dir = this->openDirectory(modPath, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles);

  // Used for "storing" the where the iteration left off at when traversing deeper into the hierarchy:
  std::vector<u64> counts;

  // The directory we are currently at:
  std::string currentDirectory = modPath;

  // Position in the txt file where we should write the next file path:
  s64 txtOffset = 0;

  // Counts for entries of currentDirectory:
  s64 currentTotal = 0;
  s64 i = 0;
  fsDirGetEntryCount(&dir, &currentTotal);

  FsDirectoryEntry entry;

  // The entry path we will look at in the following iteration in the loop:
  std::string nextAlchPath;
  std::string nextAtmoPath;

  // Set to true once we have traversed the entire folder directory within the mod folder:
  bool isDone = false;
  while (!isDone) {

    // If we have analyzed all entries in the current directory, navigate back up to where we left off in the parent:
    if (i == currentTotal) {

      // If there's nothing left in our count storage, we've navigated everything, so we're done:
      if (counts.size() == 0) {
        isDone = true;
        break;
      } else {
        // Otherwise, let's get back the count data of where we left off in the parent:
        i = counts.back();
        counts.pop_back();

        // Remove the string portion after the last '/' to get the parent's path:
        std::size_t lastSlashIndex = currentDirectory.rfind('/');
        currentDirectory = currentDirectory.substr(0, lastSlashIndex);
        this->changeDirectory(dir, currentDirectory, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles);
      }
    }

    if (R_SUCCEEDED(fsDirRead(&dir, &currentTotal, 1, &entry))) {

      nextAlchPath = currentDirectory + "/" + entry.name;

      // If the next entry is a file, we will move it and record it as moved as long as there isn't a conflict:
      if (entry.type == FsDirEntryType_File) {
        nextAtmoPath = this->getAtmosphereModPath(modPath.size(), nextAlchPath);

        if (!this->doesFileExist(nextAtmoPath)) {
          this->recordFile(nextAtmoPath + "\n", movedFilesFilePath, txtOffset);
          this->moveFile(nextAlchPath, nextAtmoPath);
        }
      // If the next entry is a folder, we will traverse within it:
      } else if (entry.type == FsDirEntryType_Dir) {

        // Add the current counts to the storage:
        counts.push_back(i);

        currentDirectory = nextAlchPath;
        this->changeDirectory(dir, currentDirectory, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles);

        // Have the counts ready for the next directory:
        fsDirGetEntryCount(&dir, &currentTotal);
        i = 0;
      }
    }
  }

  fsDirClose(&dir);
}

/**
 * Deactivates the currently active mod, restoring the moddable source to its vanilla state
 * 
 * @requirement: group and source must be set
 */
void Controller::deactivateMod() {
  std::string activeMod(this->getActiveMod());

  // Try to open the active mod's txt file to get the list of files that were moved to atmosphere's folder:
  FsFile movedFilesList;
  this->tryResult(
    fsFsOpenFile(
      &this->sdSystem,
      this->toPathBuffer(this->getMovedFilesListFilePath(activeMod)),
      FsOpenMode_Read,
      &movedFilesList
    ),
    "fsReadMoved"
  );

  s64 fileSize;
  this->tryResult(
    fsFileGetSize(&movedFilesList, &fileSize),
    "fsMovedSize"
  );

  // Small buffer to stream the file a small number of bytes at a time to minimize memory consumption:
  s64 offset = 0;
  char* buffer = new char[FILE_LIST_BUFFER_SIZE];
  std::string pathBuilder = "";

  std::string atmoPath;
  std::string alchemyPath;

  // As long as there is still data in the file:
  while (offset < fileSize) {

    // Read some of the text into our buffer:
    this->tryResult(
      fsFileRead(&movedFilesList, offset, buffer, FILE_LIST_BUFFER_SIZE, FsReadOption_None, nullptr),
      "fsReadPath"
    );

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
      alchemyPath = this->getModPath(activeMod) + atmoPath.substr(this->getAtmospherePath().size());

      this->moveFile(atmoPath, alchemyPath);
    }

    offset += FILE_LIST_BUFFER_SIZE;
  }

  delete[] buffer;

  fsFileClose(&movedFilesList);

  // Once all the files have been returned, delete the txt list:
  fsFsDeleteFile(&this->sdSystem, this->toPathBuffer(this->getMovedFilesListFilePath(activeMod)));
}

/**
 * Unmount SD card when destroyed 
 */
Controller::~Controller() {
  fsFsClose(&this->sdSystem);
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
  this->tryResult(
    fsFsOpenDirectory(&this->sdSystem, this->toPathBuffer(path), mode, &dir),
    "fsOpenDir"
  );
}

bool Controller::doesFileExist(const std::string& path) {
  FsDir dir;
  // If the file can be opened, it exists:
  bool exists = R_SUCCEEDED(fsFsOpenDirectory(&this->sdSystem, this->toPathBuffer(path), FsDirOpenMode_ReadFiles, &dir));
  fsDirClose(&dir);
  return exists;
}

/**
 * Gets a vector of all folder names that are directly within the specified path
 */
std::vector<std::string> Controller::listSubfolderNames(const std::string& path) {
  std::vector<std::string> subfolders;

  FsDir dir = this->openDirectory(path, FsDirOpenMode_ReadDirs);

  FsDirectoryEntry entry;
  s64 entryCount;
  fsDirGetEntryCount(&dir, &entryCount);

  for (s64 i = 0; i < entryCount; i++) {
    if (R_SUCCEEDED(fsDirRead(&dir, &entryCount, 1, &entry))) {
      if (entry.type == FsDirEntryType_Dir) {
        subfolders.push_back(entry.name);
      }
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
  if (!this->doesFileExist(movedFilesListPath)) {
    this->tryResult(
      fsFsCreateFile(&this->sdSystem, this->toPathBuffer(movedFilesListPath), 0, 0),
      "fsCreateMoved"
    );
  }
  
  // Open the file:
  FsFile movedListFile;
  this->tryResult(
    fsFsOpenFile(&this->sdSystem, this->toPathBuffer(movedFilesListPath), FsOpenMode_Write, &movedListFile),
    "fsWriteMoved"
  );

  // Write the path to the end of the list:
  this->tryResult(
    fsFileWrite(&movedListFile, offset, line.c_str(), line.size(), FsWriteOption_Flush),
    "fsWritePath"
  );
  fsFileClose(&movedListFile);

  // Update the offset to the end of the file:
  offset += line.size();
}

/**
 * Changes the fromPath file parameter's location to what's specified as the toPath parameter
 */
void Controller::moveFile(const std::string& fromPath, const std::string& toPath) {
  this->tryResult(
    fsFsRenameFile(&this->sdSystem, this->toPathBuffer(fromPath), this->toPathBuffer(toPath)),
    "fsMoveFile"
  );
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
std::string Controller::getGroupPath(std::string& group) {
  return this->getGamePath() + "/" + group;
}

/*
 * Gets the file path for the specified source within the group
 * 
 * @requirement: group and source must be set
 */
std::string Controller::getSourcePath() {
  return this->getGroupPath(this->group) + "/" + this->source;
}

/*
 * Get the file path for the specified mod within the moddable source
 * 
 * @requirement: group and source must be set
 */
std::string Controller::getModPath(const std::string& mod) {
  return this->getSourcePath() + "/" + mod;
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
 * 
 * @requirement: group and source must be set
 */
std::string Controller::getMovedFilesListFilePath(const std::string& mod) {
  return this->getSourcePath() + "/" + mod + TXT_EXT;
}

char* Controller::toPathBuffer(const std::string& path) {
  char* buffer = new char[FS_MAX_PATH];
  std::strcpy(buffer, path.c_str());
  return buffer;
}

/**
 * Displays an error code to the user if @param r is erroneous
 * 
 * @param alchemyCode: A short semi-readable unique code to indicate the origin of the error in Mod Alchemist's code
 */
void Controller::tryResult(const Result& r, const std::string& alchemyCode) {
  if (R_FAILED(r)) {
    fatalThrow(r);
    //tsl::changeTo<GuiError>("Error: " + alchemyCode + " " + std::to_string(r));
    //abort();
  }
}
