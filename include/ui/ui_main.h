#pragma once

#include <tesla.hpp>    // The Tesla Header

class GuiMain : public tsl::Gui {
public:
    GuiMain();

    // Called when this Gui gets loaded to create the UI
    virtual tsl::elm::Element* createUI() override;

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(
      u64 keysDown,
      u64 keysHeld,
      const HidTouchState &touchPos,
      HidAnalogStickState joyStickPosLeft,
      HidAnalogStickState joyStickPosRight
    ) override;
};
