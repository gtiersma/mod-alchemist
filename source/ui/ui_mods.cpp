#include "ui/ui_mods.h"

GuiMods::GuiMods(Controller& controller_, const std::string& source_, const std::string& group_) : controller(controller_), source(source_), group(group_) { }

tsl::elm::Element* GuiMods::createUI() {
  auto frame = new tsl::elm::OverlayFrame("The Mod Alchemist", this->source);

  std::vector<Mod> mods = controller.loadMods(this->source, this->group);
  std::string_view activeMod = controller.getActiveMod(this->source, this->group);

  auto list = new tsl::elm::List();

  // Used to disable any active mod:
  auto defaultToggle = new tsl::elm::ToggleListItem("Default " + this->source, activeMod == "");
  defaultToggle->setStateChangedListener([this](bool state) {
    if (state) {
      controller.deactivateMod(this->source, this->group);
    }
  });

  // Add the default option:
  this->toggles.push_back(defaultToggle);
  list->addItem(this->toggles[0]);

  // Add a toggle for each mod:
  for (const Mod &mod : mods) {
    auto item = new tsl::elm::ToggleListItem(mod.name, mod.name == activeMod);

    item->setStateChangedListener([this, mod](bool state) {
      if (state) {
        controller.deactivateMod(this->source, this->group);
        controller.activateMod(this->source, this->group, mod.name);
      } else {
        this->toggles[0]->setState(true);
        controller.deactivateMod(this->source, this->group);
      }
    });

    this->toggles.push_back(item);
    list->addItem(item);
  }

  frame->setContent(list);
  return frame;
}

bool GuiMods::handleInput(
  u64 keysDown,
  u64 keysHeld,
  const HidTouchState &touchPos,
  HidAnalogStickState joyStickPosLeft,
  HidAnalogStickState joyStickPosRight
) {
  if (keysDown & HidNpadButton_B) {
    tsl::goBack();
    return true;
  }
  return false;
}
