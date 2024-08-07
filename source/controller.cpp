#include "controller.h"

#include "constants.h"
#include "fs_manager.h"
#include "meta_manager.h"

#include "ui/ui_error.h"

#include <iomanip>

#include <tesla.hpp>

Controller controller;

void Controller::init() {

  // Get the title ID of the currently running game:
  u64 processId;
  GuiError::tryResult(pmdmntGetApplicationProcessId(&processId), "pmDmntPID");
  GuiError::tryResult(pminfoGetProgramId(&this->titleId, processId), "pmInfoPID");

  GuiError::tryResult(fsOpenSdCardFileSystem(&FsManager::sdSystem), "fsOpenSD");
}

/**
 * Formats u64 title ID into a hexidecimal string
 */
std::string Controller::getHexTitleId() {
  return MetaManager::getHexTitleId(this->titleId);
}

/**
 * Checks if the currenty-running game has a folder set up for Mod Alchemist
 */
bool Controller::doesGameHaveFolder() {
  return FsManager::doesFolderExist(this->getGamePath());
}

/**
 * Load all groups from the game folder
 */
std::vector<std::string> Controller::loadGroups() {
  return FsManager::listNames(this->getGamePath());
}

/**
 * Load all source options within the specified group
 * 
 * @requirement: group must be set
 */
std::vector<std::string> Controller::loadSources() {
  return FsManager::listNames(this->getGroupPath());
}

/**
 * Load all mod options that could be activated for the moddable source in the group
 * 
 * @requirement: group and source must be set
 */
std::vector<std::string> Controller::loadMods() {
  return FsManager::listNames(this->getSourcePath());
}


/*
 * Loads map of mods names to each rating
 * 
 * @requirement: group and source must be set
 */
std::map<std::string, u8> Controller::loadRatings() {
  std::map<std::string, u8> ratings;

  FsDir dir = FsManager::openFolder(this->getSourcePath(), FsDirOpenMode_ReadDirs);

  FsDirectoryEntry entry;
  s64 readCount = 0;
  while (R_SUCCEEDED(fsDirRead(&dir, &readCount, 1, &entry)) && readCount) {
    if (entry.type == FsDirEntryType_Dir) {
      std::string mod = MetaManager::parseName(entry.name);
      ratings[mod] = MetaManager::parseRating(entry.name);
    }
  }

  fsDirClose(&dir);

  return ratings;
}

/*
 * Loads the rating for the source (for using no mod)
 * 
 * @requirement: group and source must be set
 */
u8 Controller::loadDefaultRating() {
  u8 rating;

  FsDir dir = FsManager::openFolder(this->getGroupPath(), FsDirOpenMode_ReadDirs);

  FsDirectoryEntry entry;
  s64 readCount = 0;
  while (R_SUCCEEDED(fsDirRead(&dir, &readCount, 1, &entry)) && readCount) {
    if (entry.type == FsDirEntryType_Dir && this->source == MetaManager::parseName(entry.name)) {
      rating = MetaManager::parseRating(entry.name);
    }
  }

  fsDirClose(&dir);

  return rating;
}

/*
 * Saves the ratings for each mod
 * 
 * @requirement: group and source must be set
 */
void Controller::saveRatings(const std::map<std::string, u8>& ratings) {
  for (const auto& [mod, rating]: ratings) {
    std::string currentPath = this->getModPath(mod);
    std::string newPath = this->getSourcePath() + "/" + MetaManager::buildFolderName(mod, rating);

    GuiError::tryResult(
      fsFsRenameDirectory(&FsManager::sdSystem, FsManager::toPathBuffer(currentPath), FsManager::toPathBuffer(newPath)),
      "fsRatingChange"
    );
  }
}

/*
 * Saves the rating for using no mod for the current source
 */
void Controller::saveDefaultRating(const u8& rating) {;
  std::string newPath = this->getGroupPath() + "/" + MetaManager::buildFolderName(this->source, rating);

  GuiError::tryResult(
    fsFsRenameDirectory(&FsManager::sdSystem, FsManager::toPathBuffer(this->getSourcePath()), FsManager::toPathBuffer(newPath)),
    "fsRatingChange"
  );
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
  FsDir sourceDir = FsManager::openFolder(this->getSourcePath(), FsDirOpenMode_ReadFiles);

  // Find the .txt file in the directory. The name would be the active mod:
  FsDirectoryEntry entry;
  s64 readCount = 0;
  std::string_view activeMod = "";
  std::string_view name;

  while (R_SUCCEEDED(fsDirRead(&sourceDir, &readCount, 1, &entry)) && readCount) {
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
 * 
 * @requirement: group and source must be set
 */
void Controller::activateMod(const std::string& mod) {

  // Skip if already active
  if (this->getActiveMod() == mod) { return; }

  // Path to the "mod" folder in alchemy's directory:
  std::string modPath = this->getModPath(mod);
  // Path of the txt file for the active mod:
  std::string movedFilesFilePath = this->getMovedFilesListFilePath(mod);

  FsDir dir = FsManager::openFolder(modPath, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles);

  // Iterartor for current entry in the current directory:
  short i = 0;

  // Used for "storing" where the iteration left off at when traversing deeper into the hierarchy:
  std::vector<u64> iStorage;

  // The directory we are currently at:
  std::string currentDirectory = modPath;

  // Position in the txt file where we should write the next file path:
  s64 txtOffset = 0;

  // The index of the current entry we're iterating over in the current directory:
  short entryIndex = 0;

  // The current number of files read at a time
  // It reads 1 at a time, so it will always be either 1 or 0 (0 if all have been read)
  s64 readCount = 0;

  FsDirectoryEntry entry;

  // The entry path we will look at in the following iteration in the loop:
  std::string nextAlchPath;
  std::string nextAtmoPath;

  FsManager::createFolderIfNeeded(this->getAtmospherePath());

  while (R_SUCCEEDED(fsDirRead(&dir, &readCount, 1, &entry))) {

    // Continue iterating the index until it catches up with the iteration we should be on (if needed):
    entryIndex++;
    if (entryIndex > i) {
      i++;

      if (readCount > 0) {
        nextAlchPath = currentDirectory + "/" + entry.name;
        nextAtmoPath = this->getAtmosphereModPath(modPath.size(), nextAlchPath);

        // If the next entry is a file, we will move it and record it as moved as long as there isn't a conflict:
        if (entry.type == FsDirEntryType_File && !FsManager::doesFileExist(nextAtmoPath)) {
          FsManager::recordFile(nextAtmoPath + "\n", movedFilesFilePath, txtOffset);
          FsManager::moveFile(nextAlchPath, nextAtmoPath);
        // If the next entry is a folder, we will traverse within it:
        } else if (entry.type == FsDirEntryType_Dir) {
          FsManager::createFolderIfNeeded(nextAtmoPath);

          // Add the current count to the storage:
          iStorage.push_back(i);

          currentDirectory = nextAlchPath;
          FsManager::changeFolder(dir, currentDirectory, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles);

          // Reset the index & iterator because we're starting in a new folder:
          entryIndex = 0;
          i = 0;
        }
      } else {
        // If there's nothing left in our count storage, we've navigated everything, so we're done:
        if (iStorage.size() == 0) { break; }

        // Otherwise, let's get back the count data of where we left off in the parent:
        i = iStorage.back();
        iStorage.pop_back();

        // Remove the string portion after the last '/' to get the parent's path:
        std::size_t lastSlashIndex = currentDirectory.rfind('/');
        currentDirectory = currentDirectory.substr(0, lastSlashIndex);
        FsManager::changeFolder(dir, currentDirectory, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles);

        // Reset the entry index because it will start at the beginning again:
        entryIndex = 0;
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

  // If no active mod:
  if (activeMod.empty()) { return; }

  this->returnFiles(activeMod);
}

void Controller::deactivateAll() {
  std::vector<std::string> groups = this->loadGroups();

  for (const std::string& group : groups) {
    this->group = group;
    std::vector<std::string> sources = this->loadSources();

    for (const std::string& source : sources) {
      this->source = source;
      std::string activeMod(this->getActiveMod());

      if (!activeMod.empty()) {
        this->returnFiles(activeMod);
      }
    }
  }

  this->group = "";
  this->source = "";
}

/**
 * Randomly activates/deactivates all mods based upon their ratings
 */
void Controller::randomize() {
  std::vector<std::string> groups = this->loadGroups();

  for (const std::string& group : groups) {
    this->group = group;
    std::vector<std::string> sources = this->loadSources();

    for (const std::string& source : sources) {
      this->source = source;
      this->pickMod();
    }
  }

  this->group = "";
  this->source = "";
}

/**
 * Randomly activates a mod from the current group and source
 * 
 * @requirement: group and source must be set
 */
void Controller::pickMod() {
  std::map<std::string, u8> ratings = this->loadRatings();
  u8 defaultRating = this->loadDefaultRating();

  u16 ratingTotal = defaultRating;
  for (const auto& [mod, rating]: ratings) {
    ratingTotal += rating;
  }

  u16 randomChoice = rand() % ratingTotal;

  if (randomChoice < defaultRating) {
    this->deactivateMod();
  } else {
    randomChoice -= defaultRating;

    for (const auto& [mod, rating]: ratings) {
      if (randomChoice < rating) {
        this->deactivateMod();
        this->activateMod(mod);
        return;
      }

      randomChoice -= rating;
    }
  }
}

/**
 * Unmount SD card when destroyed 
 */
Controller::~Controller() {
  fsFsClose(&FsManager::sdSystem);
}

void Controller::returnFiles(const std::string& mod) {

  // Try to open the active mod's txt file to get the list of files that were moved to atmosphere's folder:
  FsFile movedFilesList;
  GuiError::tryResult(
    fsFsOpenFile(
      &FsManager::sdSystem,
      FsManager::toPathBuffer(this->getMovedFilesListFilePath(mod)),
      FsOpenMode_Read,
      &movedFilesList
    ),
    "fsReadMoved"
  );

  s64 fileSize;
  GuiError::tryResult(
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
    GuiError::tryResult(
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
      alchemyPath = this->getModPath(mod) + atmoPath.substr(this->getAtmospherePath().size());

      FsManager::moveFile(atmoPath, alchemyPath);

      // Not sure why, but the file needs to be re-opened after each time a file moved:
      GuiError::tryResult(
        fsFsOpenFile(
          &FsManager::sdSystem,
          FsManager::toPathBuffer(this->getMovedFilesListFilePath(mod)),
          FsOpenMode_Read,
          &movedFilesList
        ),
        "fsReadMoved"
      );
    }

    offset += FILE_LIST_BUFFER_SIZE;
  }

  delete[] buffer;

  fsFileClose(&movedFilesList);

  // Once all the files have been returned, delete the txt list:
  fsFsDeleteFile(&FsManager::sdSystem, FsManager::toPathBuffer(this->getMovedFilesListFilePath(mod)));
}

/*
 * Gets Mod Alchemist's game directory:
 */
std::string Controller::getGamePath() {
  return ALCHEMIST_PATH + MetaManager::getHexTitleId(this->titleId);
}

/*
 * Gets the file path for the specified group
 * 
 * @requirement: group must be set
 */
std::string Controller::getGroupPath() {
  return this->getGamePath() + "/" + this->group;
}

/*
 * Gets the file path for the specified source within the group
 * 
 * @requirement: group and source must be set
 */
std::string Controller::getSourcePath() {
  std::string groupPath = this->getGroupPath();
  return groupPath + "/" + FsManager::getFolderName(groupPath, this->source);
}

/*
 * Get the file path for the specified mod within the moddable source
 * 
 * @requirement: group and source must be set
 */
std::string Controller::getModPath(const std::string& mod) {
  std::string sourcePath = this->getSourcePath();
  return sourcePath + "/" + FsManager::getFolderName(sourcePath, mod);
}

/**
 * Gets the game's path that's stored within Atmosphere's directory
 */
std::string Controller::getAtmospherePath() {
  return ATMOSPHERE_PATH + MetaManager::getHexTitleId(this->titleId);
}

/**
 * Builds the path a mod's file should have once we intend to move it into Atmosphere's folder
 * It is built off of its current path within the Mod Alchemist's directory structure.
 */
std::string Controller::getAtmosphereModPath(std::size_t alchemistModFolderPathSize, const std::string& alchemistModFilePath) {
  return this->getAtmospherePath() + alchemistModFilePath.substr(alchemistModFolderPathSize);
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
