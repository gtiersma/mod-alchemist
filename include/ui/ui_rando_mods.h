#ifndef UI_RANDO_MODS_HPP
#define UI_RANDO_MODS_HPP

#include <string>
#include <vector>

#include <tesla.hpp>    // The Tesla Header

#include "mod.h"
#include "../controller.h"

class GuiRandoMods : public tsl::Gui {
  private:
    Controller controller;
    std::string source;
    std::string group;
    std::vector<Mod> mods;
    std::vector<tsl::elm::TrackBar*> sliders;
    tsl::elm::TrackBar* defaultSlider;

  public:
    GuiRandoMods(
      Controller& controller_,
      const std::string& source_,
      const std::string& group_,
      const std::vector<Mod>& mods_
    );

    virtual tsl::elm::Element* createUI() override;

    virtual bool handleInput(
      u64 keysDown,
      u64 keysHeld,
      const HidTouchState &touchPos,
      HidAnalogStickState joyStickPosLeft,
      HidAnalogStickState joyStickPosRight
    ) override;
};

#endif // UI_RANDO_MODS_HPP