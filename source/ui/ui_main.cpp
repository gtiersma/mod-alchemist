#include "ui/ui_main.h"
#include "ui/ui_groups.h"

#include "controller.h"

/**
 * Debugging code transplanted from Masagrator's Status Monitor Overlay to help monitor RAM usage
 * 
 * https://github.com/masagrator/Status-Monitor-Overlay
 * 
 * Unfortunately, I could not get it to work, and it's not worth any more time,
 * so I'm abandoning these changes for a simpler solution.
 */

Thread t;
std::pair<u32, u32> RAM_dimensions;
size_t fontsize = 15;
uint32_t margin = 8;
size_t text_width = 0;
bool Initialized = false;
bool threadexit = false;
bool usingClk = false;

Mutex mutex_Misc = {0};
//Stuff that doesn't need multithreading
void Misc(void *) {
  while (!threadexit) {
    mutexLock(&mutex_Misc);
    ClkrstSession clkSession;
    if (R_SUCCEEDED(clkrstOpenSession(&clkSession, PcvModuleId_EMC, 3))) {
      usingClk = true;
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

		mutexUnlock(&mutex_Misc);
		svcSleepThread(3 < 10 ? (1'000'000'000 / 3) : 100'000'000);
  }
}

void StartThreads() {
  threadCreate(&t, Misc, NULL, NULL, 0x1000, 0x3F, -2);
  threadStart(&t);
}

//End reading all stats
void CloseThreads() {
  threadexit = true;
  threadWaitForExit(&t);
  threadClose(&t);
  threadexit = false;
}

GuiMain::GuiMain() {
  controller.init();
	mutexInit(&mutex_Misc);
  StartThreads();
}

GuiMain::~GuiMain() {
  CloseThreads();
}

tsl::elm::Element* GuiMain::createUI() {
    // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
    auto frame = new tsl::elm::OverlayFrame("The Mod Alchemist", "v0.1.0");

    auto list = new tsl::elm::List();

    auto Status = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, u16 x, u16 y, u16 w, u16 h) {

			if (!Initialized) {
				if (usingClk) {
					RAM_dimensions = renderer->drawString("RAM 4.4/44.4GB△4444.4", false, 0, fontsize, fontsize, renderer->a(0x0000));
				} else {
          RAM_dimensions = renderer->drawString("RAM 100.0%△4444.4", false, 0, fontsize, fontsize, renderer->a(0x0000));
        }
				auto spacesize = renderer->drawString(" ", false, 0, fontsize, fontsize, renderer->a(0x0000));
				margin = spacesize.first;
				text_width = 0;
				int8_t entry_count = -1;
				text_width += RAM_dimensions.first;
				entry_count += 1;
				text_width += (margin * entry_count);
				Initialized = true;
				tsl::hlp::requestForeground(false);
			}

			u32 base_y = tsl::cfg::FramebufferHeight - (fontsize + (fontsize / 4));

			renderer->drawRect(0, base_y, tsl::cfg::FramebufferWidth, fontsize + (fontsize / 4), a(0x1117));

			uint32_t offset = (tsl::cfg::FramebufferWidth - text_width) / 2;
			auto dimensions_s = renderer->drawString("RAM", false, offset, base_y+fontsize, fontsize, renderer->a(0xFFFF));
			uint32_t offset_s = offset + dimensions_s.first + margin;
			renderer->drawString(controller.RAM_var_compressed_c, false, offset_s, base_y+fontsize, fontsize, renderer->a(0xFFFF));
			offset += RAM_dimensions.first + margin;
		});

    list->addItem(Status);

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

void GuiMain::update() {
  mutexLock(&mutex_Misc);

  snprintf(
    controller.RAM_Hz_c,
    sizeof controller.RAM_Hz_c,
    "Frequency: %.1f MHz",
    (float)controller.RAM_Hz / 1000000
  );

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

  snprintf(controller.RAM_all_c, sizeof controller.RAM_all_c, "Total:");
  snprintf(controller.RAM_application_c, sizeof controller.RAM_application_c, "Application:");
  snprintf(controller.RAM_applet_c, sizeof controller.RAM_applet_c, "Applet:");
  snprintf(controller.RAM_system_c, sizeof controller.RAM_system_c, "System:");
  snprintf(controller.RAM_systemunsafe_c, sizeof controller.RAM_systemunsafe_c, "System Unsafe:");
  snprintf(
    controller.RAM_all_c,
    sizeof controller.RAM_all_c,
    "%4.2f / %4.2f MB",
    RAM_Used_all_f,
    RAM_Total_all_f
  );
  snprintf(
    controller.RAM_application_c,
    sizeof controller.RAM_application_c,
    "%4.2f / %4.2f MB",
    RAM_Used_application_f,
    RAM_Total_application_f
  );
  snprintf(
    controller.RAM_applet_c,
    sizeof controller.RAM_applet_c,
    "%4.2f / %4.2f MB",
    RAM_Used_applet_f,
    RAM_Total_applet_f
  );
  snprintf(
    controller.RAM_system_c,
    sizeof controller.RAM_system_c,
    "%4.2f / %4.2f MB",
    RAM_Used_system_f,
    RAM_Total_system_f
  );
  snprintf(
    controller.RAM_systemunsafe_c,
    sizeof controller.RAM_systemunsafe_c,
    "%4.2f / %4.2f MB",
    RAM_Used_systemunsafe_f,
    RAM_Total_systemunsafe_f
  );
  snprintf(
    controller.RAM_var_compressed_c,
    sizeof controller.RAM_var_compressed_c,
    "%s\n%s\n%s\n%s\n%s",
    controller.RAM_all_c,
    controller.RAM_application_c,
    controller.RAM_applet_c,
    controller.RAM_system_c,
    controller.RAM_systemunsafe_c
  );

	mutexUnlock(&mutex_Misc);
}
