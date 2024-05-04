#pragma once

#include <switch.h>

#include <vector>
#include <string>
#include <fstream>

#include "mod.h"

class Controller {
  private:
    FsFileSystem sdSystem;
    u64 titleId; // The current Game's Title ID
    bool isSdCardOpen; // Whether the SD card has been mounted or not

  public:
    Controller(u64 titleId) {
      this->titleId = titleId;
    }

    /*
     * Load all groups from the game folder
     */
    std::vector<std::string> loadGroups();

    /*
     * Load all source options within the specified group
     */
    std::vector<std::string> loadSources(const std::string& group);

    /*
     * Load all mod options that could be activated for the moddable source in the group
     */
    std::vector<Mod> loadMods(const std::string& source, const std::string& group);

    /*
     * Gets the mod currently activated for the moddable source in the group
     *
     * Returns an empty string if no mod is active and vanilla files are being used
     */
    std::string_view getActiveMod(const std::string& source, const std::string& group);

    /*
     * Activates the specified mod, moving all its files into the atmosphere folder for the game
     */
    void activateMod(const std::string& source, const std::string& group, const std::string& mod);

    /**
     * Deactivates the currently active mod, restoring the moddable source to its vanilla state
     */
    void deactivateMod(const std::string& source, const std::string& group);

    u8 getModlessRating(const std::string& group, const std::string& source);

    void saveModlessRating(const u8& rating, const std::string& source, const std::string& group);

    void saveRatings(std::vector<Mod> mods, const std::string& source, const std::string& group);

    /**
     * Unmount SD card when destroyed 
     */
    ~Controller();

  private:

    /**
     * Mounts the SD card if it hasn't been mounted yet
     * 
     * We need to be sure this always runs before attempting to perform any file operations 
     */
    void openSdCardIfNeeded();

    /**
     * Creates a new open FsDir object for the specified path
     * 
     * Don't forget to close when done
     */
    FsDir openDirectory(const std::string& path, u32 mode);

    /**
     * Changes an FsDir instance to the specified path
     */
    void changeDirectory(FsDir& dir, const std::string& path, u32 mode);

    bool doesFileExist(const std::string& path);
    bool doesFileNotExist(const std::string& path);

    /**
     * Gets a vector of all folder names that are directly within the specified path
     */
    std::vector<std::string> listSubfolderNames(const std::string& path);

    /**
     * Records the line parameter (should end in \n) for a file being moved in modPath
     * "line" is appended to the movedFilesListPath file 
     * 
     * offset is expected to be at the end of the file,
     * and it's updated to the new position at the end of file
     */
    void recordFile(const std::string& line, const std::string& movedFilesListPath, s64& offset);

    /**
     * Changes the fromPath file parameter's location to what's specified as the toPath parameter
     */
    void moveFile(const std::string& fromPath, const std::string& toPath);

    /**
     * Gets Mod Alchemist's game directory:
     */
    std::string getGamePath();

    /**
     * Gets the file path for the specified group
     */
    std::string getGroupPath(const std::string& group);

    /**
     * Gets the file path for the specified source within the group
     */
    std::string getSourcePath(const std::string& group, const std::string& source);

    /**
     * Get the file path for the specified mod within the moddable source
     */
    std::string getModPath(const std::string& group, const std::string& source, const std::string& mod);

    /**
     * Gets the game's path that's stored within Atmosphere's directory
     */
    std::string getAtmospherePath();

    /**
     * Builds the path a mod's file should have once we intend to move it into Atmosphere's folder
     * It is built off of its current path within the Mod Alchemist's directory structure.
     */
    std::string getAtmosphereModPath(std::size_t alchemistModFolderPathSize, const std::string& alchemistModFilePath);

    /**
     * Gets the file path for the list of moved files for the specified mod
     * 
     * The file should only exist if the mod is currently active
     */
    std::string getMovedFilesListFilePath(const std::string& group, const std::string& source, const std::string& mod);

    bool hasRating(const std::string& folderName);
    std::string getStringRatingFromName(const std::string& folderName);
    u8 getRatingFromName(const std::string& folderName);
    std::string trimRating(const std::string& folderName);
};
