#pragma once

#include <switch.h>

#include <vector>
#include <map>
#include <string>

class Controller {
  public:
    u64 titleId; // The current Game's Title ID
    std::string group;
    std::string source;

    void init();

    /**
     * Formats u64 title ID into a hexidecimal string
     */
    std::string getHexTitleId();

    bool doesGameHaveFolder();

    /*
     * Load all groups from the game folder
     */
    std::vector<std::string> loadGroups();

    /*
     * Load all source options within the specified group
     */
    std::vector<std::string> loadSources();

    /*
     * Load all mod options that could be activated for the moddable source in the group
     */
    std::vector<std::string> loadMods();

    /*
     * Loads map of mods names to each rating
     */
    std::map<std::string, u8> loadRatings();

    /*
     * Loads the rating for the source (for using no mod)
     */
    u8 loadDefaultRating();

    /*
     * Saves the ratings for each mod
     */
    void saveRatings(const std::map<std::string, u8>& ratings);

    /*
     * Saves the rating for using no mod for the current source
     */
    void saveDefaultRating(const u8& rating);

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

    void deactivateAll();

    /**
     * Randomly activates/deactivates all mods based upon their ratings
     */
    void randomize();

    /**
     * Randomly activates a mod from the current group and source
     * 
     * @requirement: group and source must be set
     */
    void pickMod();

    /**
     * Unmount SD card when destroyed 
     */
    ~Controller();

  private:

    void returnFiles(const std::string& mod);

    /**
     * Gets Mod Alchemist's game directory:
     */
    std::string getGamePath();

    /**
     * Gets the file path for the specified group
     */
    std::string getGroupPath();

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
};

extern Controller controller;