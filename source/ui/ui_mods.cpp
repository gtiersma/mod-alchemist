#include <tesla.hpp>    // The Tesla Header

#include "ui_mods.h"

/**
 * List of toggable mods for a specific source 
 */
GuiMods::GuiMods(Controller& controller_, std::string& source_, std::string& group_)
  : controller(controller_), source(source_), group(group_) {
    this->toggles = std::vector<tsl::elm::ToggleListItem*>();
  }

tsl::elm::Element* GuiMods::createUI() override {
  auto frame = new tsl::elm::OverlayFrame("The Mod Alchemist", this->source);

  try {
    std::vector<std::string> mods = controller.loadMods(this->source, this->group);
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
    for (const std::string mod : mods) {
      auto item = new tsl::elm::ToggleListItem(mod, mod == activeMod);

      item->setStateChangedListener([this, mod](bool state) {
        if (state) {
          controller.deactivateMod(this->source, this->group);
          controller.activateMod(this->source, this->group, mod);
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
  } catch (std::filesystem::filesystem_error &e) {
    tsl::changeTo<GuiError>(std::string("Filesystem error: ") + e.what());
  } catch (std::exception &e) {
    tsl::changeTo<GuiError>(e.what());
  } catch (...) {
    tsl::changeTo<GuiError>("Unknown error");
  }
}

void GuiMods::update() override { }

bool GuiMods::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
  if (keysDown & HidNpadButton_B) {
    tsl::goBack();
    return true;
  }
  return false;
}