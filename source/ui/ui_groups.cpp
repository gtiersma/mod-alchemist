#include "ui/ui_groups.h"
#include "ui/ui_sources.h"
#include "controller.h"
#include "constants.h"

/**
 * Debugging code transplanted from Masagrator's Status Monitor Overlay to help monitor RAM usage
 * 
 * https://github.com/masagrator/Status-Monitor-Overlay
 */

GuiGroups::GuiGroups() { }

tsl::elm::Element* GuiGroups::createUI() {
  auto frame = new tsl::elm::OverlayFrame("The Mod Alchemist", "Mod Groups");

  auto list = new tsl::elm::List();

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

    list->addItem(new tsl::elm::CategoryHeader("RAM Usage"));
    list->addItem(new tsl::elm::ListItem("Total Application: " + std::to_string(RAM_Total_application_f) + " MB"));
    list->addItem(new tsl::elm::ListItem("Total Applet: " + std::to_string(RAM_Total_applet_f) + " MB"));
    list->addItem(new tsl::elm::ListItem("Total System: " + std::to_string(RAM_Total_system_f) + " MB"));
    list->addItem(new tsl::elm::ListItem("Total System Unsafe: " + std::to_string(RAM_Total_systemunsafe_f) + " MB"));
    list->addItem(new tsl::elm::ListItem("Total All: " + std::to_string(RAM_Total_all_f) + " MB"));
    list->addItem(new tsl::elm::ListItem("Used Application: " + std::to_string(RAM_Used_application_f) + " MB"));
    list->addItem(new tsl::elm::ListItem("Used Applet: " + std::to_string(RAM_Used_applet_f) + " MB"));
    list->addItem(new tsl::elm::ListItem("Used System: " + std::to_string(RAM_Used_system_f) + " MB"));
    list->addItem(new tsl::elm::ListItem("Used System Unsafe: " + std::to_string(RAM_Used_systemunsafe_f) + " MB"));
    list->addItem(new tsl::elm::ListItem("Used All: " + std::to_string(RAM_Used_all_f) + " MB"));

  if (!controller.doesGameHaveFolder()) {
    list->addItem(new tsl::elm::ListItem("The running game has no folder."));
    list->addItem(new tsl::elm::ListItem("It should be named \"" + std::to_string(controller.titleId) + "\""));
    list->addItem(new tsl::elm::ListItem("And located in the \"" + ALCHEMIST_PATH + "\" directory."));
    frame->setContent(list);
    return frame;
  }

  std::vector<std::string> groups = controller.loadGroups();

  // When there are no groups for some odd reason:
  if (groups.empty()) {
    frame->setContent(new tsl::elm::CategoryHeader("No groups found"));
    return frame;
  }

  for (const std::string &group : groups) {
    auto *item = new tsl::elm::ListItem(group);

    item->setClickListener([&](u64 keys) {
      if (keys & HidNpadButton_A) {
        controller.group = group;
        tsl::changeTo<GuiSources>();
        return true;
      }
      return false;
    });

    list->addItem(item);
  }
  
  frame->setContent(list);
  return frame;
}

bool GuiGroups::handleInput(
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
