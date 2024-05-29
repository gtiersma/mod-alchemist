#pragma once

#include <switch.h>

#include <vector>
#include <map>
#include <string>

class Controller {
  private:
    FsFileSystem sdSystem;

  public:
    u64 titleId; // The current Game's Title ID
    std::string group;
    std::string source;

    void init();

    bool doesGameHaveFolder();

    /*
     * Load all groups from the game folder
     */
    std::map<std::string, std::vector<std::string>> loadGroups();

    /*
     * Load all mod options that could be activated for the moddable source in the group
     */
    std::vector<std::string> loadMods();

    /*
     * Gets the mod currently activated for the moddable source in the group
     *
     * Returns an empty string if no mod is active and vanilla files are being used
     */
    std::string_view getActiveMod();

    /*
     * Activates the specified mod, moving all its files into the atmosphere folder for the game
     */
    void activateMod(const std::string& mod);

    /**
     * Deactivates the currently active mod, restoring the moddable source to its vanilla state
     */
    void deactivateMod();

    /**
     * Unmount SD card when destroyed 
     */
    ~Controller();

  private:

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
    std::string getGroupPath(std::string& group);

    /**
     * Gets the file path for the specified source within the group
     */
    std::string getSourcePath();

    /**
     * Get the file path for the specified mod within the moddable source
     */
    std::string getModPath(const std::string& mod);

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
    std::string getMovedFilesListFilePath(const std::string& mod);

    char* toPathBuffer(const std::string& path);

    void tryResult(const Result& r, const std::string& alchemyCode);
};

extern Controller controller;