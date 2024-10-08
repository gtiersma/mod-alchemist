#include "ui/ui_locks.h"

#include <string>

#include "controller.h"

GuiLocks::GuiLocks() {}

tsl::elm::Element* GuiLocks::createUI() {
  auto frame = new tsl::elm::OverlayFrame("The Mod Alchemist", controller.group);

  auto groupList = new tsl::elm::List();

  std::map<std::string, bool> sources = controller.loadSourceLocks();

  // For when the group is empty for some reason:
  if (sources.empty()) {
    frame->setContent(new tsl::elm::CategoryHeader("Group is empty"));
    return frame;
  }

  // List all the group's source with active mods for locking/unlocking:
  for (const auto& [name, locked]: sources) {
    std::string activeMod = controller.getActiveMod(name);

    std::string label;
    if (activeMod.empty()) {
      label = name + " - no mod active";
    } else {
      label = activeMod + " (" + name + ")";
    }

    auto *item = new tsl::elm::ToggleListItem(label, locked);
    item->setClickListener([name, locked](u64 keys) {
      if (keys & HidNpadButton_A) {
        if (locked) {
          controller.unlockSource(name);
        } else {
          controller.lockSource(name);
        }
        return true;
      }
      return false;
    });

    groupList->addItem(item);
  }

  frame->setContent(groupList);
  return frame;
}

bool GuiLocks::handleInput(
  u64 keysDown,
  u64 keysHeld,
  const HidTouchState &touchPos,
  HidAnalogStickState joyStickPosLeft,
  HidAnalogStickState joyStickPosRight
) {
  if (keysDown & HidNpadButton_B) {
    controller.group = "";
    tsl::goBack();
    return true;
  }
  return false;
}
