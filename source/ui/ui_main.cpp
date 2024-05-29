#include "ui/ui_main.h"
#include "ui/ui_groups.h"

#include "controller.h"

/**
 * Debugging code transplanted from Masagrator's Status Monitor Overlay to help monitor RAM usage
 * 
 * https://github.com/masagrator/Status-Monitor-Overlay
 */

GuiMain::GuiMain() {}

tsl::elm::Element* GuiMain::createUI() {
    controller.init();

    ClkrstSession clkSession;
    if (R_SUCCEEDED(clkrstOpenSession(&clkSession, PcvModuleId_EMC, 3))) {
      clkrstGetClockRate(&clkSession, &controller.RAM_Hz);
      clkrstCloseSession(&clkSession);
    } else {
      pcvGetClockRate(PcvModule_EMC, &controller.RAM_Hz);
    }

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

    // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
    auto frame = new tsl::elm::OverlayFrame("The Mod Alchemist", "v0.1.0");

    auto list = new tsl::elm::List();

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

    // Option for switching between mods
    auto *mod_item = new tsl::elm::ListItem("Set Mods");
    mod_item->setClickListener([](u64 keys) {
        if (keys & HidNpadButton_A) {
            tsl::changeTo<GuiGroups>();
            return true;
        }
        return false;
    });
    list->addItem(mod_item);

    // Add the list to the frame for it to be drawn
    frame->setContent(list);

    // Return the frame to have it become the top level element of this Gui
    return frame;
}

// Called once every frame to handle inputs not handled by other UI elements
bool GuiMain::handleInput(
  u64 keysDown,
  u64 keysHeld,
  const HidTouchState &touchPos,
  HidAnalogStickState joyStickPosLeft,
  HidAnalogStickState joyStickPosRight
) {
    return false;   // Return true here to signal the inputs have been consumed
}
