#ifndef GUI_GROUPS_HPP
#define GUI_GROUPS_HPP

#include "constants.h"

#include <string>
#include <vector>

#include <tesla.hpp>    // The Tesla Header

class GuiGroups : public tsl::Gui {
  private:
    EditMode editMode;

  public:
    GuiGroups(EditMode editMode);

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