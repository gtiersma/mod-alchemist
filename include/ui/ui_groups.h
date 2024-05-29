#ifndef GUI_GROUPS_HPP
#define GUI_GROUPS_HPP

#include <string>
#include <map>
#include <vector>

#include <tesla.hpp>    // The Tesla Header

class GuiGroups : public tsl::Gui {
  public:
    GuiGroups();

    virtual tsl::elm::Element* createUI() override;

    virtual bool handleInput(
      u64 keysDown,
      u64 keysHeld,
      const HidTouchState &touchPos,
      HidAnalogStickState joyStickPosLeft,
      HidAnalogStickState joyStickPosRight
    ) override;
};

#endif // GUI_GROUPS_HPP