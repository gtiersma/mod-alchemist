#include "ui/ui_sources.h"
#include "ui/ui_mods.h"

#include <string>
#include <vector>

#include "controller.h"

/**
 * Debugging code transplanted from Masagrator's Status Monitor Overlay to help monitor RAM usage
 * 
 * https://github.com/masagrator/Status-Monitor-Overlay
 */

GuiSources::GuiSources() {}

tsl::elm::Element* GuiSources::createUI() {
  auto frame = new tsl::elm::OverlayFrame("The Mod Alchemist", controller.group);

  auto groupList = new tsl::elm::List();

    svcGetSystemInfo(&controller.RAM_Total_application_u, 0, INVALID_HANDLE, 0);
    svcGetSystemInfo(&controller.RAM_Total_applet_u, 0, INVALID_HANDLE, 1);
    svcGetSystemInfo(&controller.RAM_Total_system_u, 0, INVALID_HANDLE, 2);
    svcGetSystemInfo(&controller.RAM_Total_systemunsafe_u, 0, INVALID_HANDLE, 3);
    svcGetSystemInfo(&controller.RAM_Used_application_u, 1, INVALID_HANDLE, 0);
    svcGetSystemInfo(&controller.RAM_Used_applet_u, 1, INVALID_HANDLE, 1);
    svcGetSystemInfo(&controller.RAM_Used_system_u, 1, INVALID_HANDLE, 2);
    svcGetSystemInfo(&controller.RAM_Used_systemunsafe_u, 1, INVALID_HANDLE, 3);

    float RAM_Total_application_f = (float)controller.RAM_Total_application_u / 1024 / 1024;
    float RAM_Total_applet_f = (float)controller.RAM_Total_applet_u / 1024 / 1024;
    float RAM_Total_system_f = (float)controller.RAM_Total_system_u / 1024 / 1024;
    float RAM_Total_systemunsafe_f = (float)controller.RAM_Total_systemunsafe_u / 1024 / 1024;
    float RAM_Total_all_f = RAM_Total_application_f + RAM_Total_applet_f + RAM_Total_system_f + RAM_Total_systemunsafe_f;
    float RAM_Used_application_f = (float)controller.RAM_Used_application_u / 1024 / 1024;
    float RAM_Used_applet_f = (float)controller.RAM_Used_applet_u / 1024 / 1024;
    float RAM_Used_system_f = (float)controller.RAM_Used_system_u / 1024 / 1024;
    float RAM_Used_systemunsafe_f = (float)controller.RAM_Used_systemunsafe_u / 1024 / 1024;
    float RAM_Used_all_f = RAM_Used_application_f + RAM_Used_applet_f + RAM_Used_system_f + RAM_Used_systemunsafe_f;

    groupList->addItem(new tsl::elm::CategoryHeader("RAM Usage"));
    groupList->addItem(new tsl::elm::ListItem("Total Application: " + std::to_string(RAM_Total_application_f) + " MB"));
    groupList->addItem(new tsl::elm::ListItem("Total Applet: " + std::to_string(RAM_Total_applet_f) + " MB"));
    groupList->addItem(new tsl::elm::ListItem("Total System: " + std::to_string(RAM_Total_system_f) + " MB"));
    groupList->addItem(new tsl::elm::ListItem("Total System Unsafe: " + std::to_string(RAM_Total_systemunsafe_f) + " MB"));
    groupList->addItem(new tsl::elm::ListItem("Total All: " + std::to_string(RAM_Total_all_f) + " MB"));
    groupList->addItem(new tsl::elm::ListItem("Used Application: " + std::to_string(RAM_Used_application_f) + " MB"));
    groupList->addItem(new tsl::elm::ListItem("Used Applet: " + std::to_string(RAM_Used_applet_f) + " MB"));
    groupList->addItem(new tsl::elm::ListItem("Used System: " + std::to_string(RAM_Used_system_f) + " MB"));
    groupList->addItem(new tsl::elm::ListItem("Used System Unsafe: " + std::to_string(RAM_Used_systemunsafe_f) + " MB"));
    groupList->addItem(new tsl::elm::ListItem("Used All: " + std::to_string(RAM_Used_all_f) + " MB"));

  std::vector<std::string> sources = controller.loadSources();

  // For when the group is empty for some reason:
  if (sources.empty()) {
    frame->setContent(new tsl::elm::CategoryHeader("Group is empty"));
    return frame;
  }

  // List all of the group's sources:
  for (const std::string &source : sources) {
    auto *item = new tsl::elm::ListItem(source);

    item->setClickListener([&](u64 keys) {
      if (keys & HidNpadButton_A) {
        controller.source = source;
        tsl::changeTo<GuiMods>(); // Use the GuiMods class to navigate to the mods UI
        return true;
      }
      return false;
    });

    groupList->addItem(item);
  }

  frame->setContent(groupList);
  return frame;
}

bool GuiSources::handleInput(
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
