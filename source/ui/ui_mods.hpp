#include <tesla.hpp>    // The Tesla Header

#include "ui_error.hpp"
#include "../controller.hpp"

class GuiMods : public tsl::Gui {
  private:
    Controller controller;
    std::string source;
    std::string group;
    std::vector<tsl::elm::ToggleListItem*> toggles;

  public:
    GuiMods(Controller& controller_, std::string& source_, std::string& group_)
      : controller(controller_), source(source_), group(group_) {
        this->toggles = std::vector<tsl::elm::ToggleListItem*>();
      }

    // Called when this Gui gets loaded to create the UI
    // Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
    virtual tsl::elm::Element* createUI() override {
        // A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
        // If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
        auto frame = new tsl::elm::OverlayFrame("The Mod Alchemist", this->source);

        try {
          std::vector<std::string> mods = controller.loadMods(this->source, this->group);
          std::string activeMod = controller.getActiveMod(this->source, this->group);

          auto list = new tsl::elm::List();

          auto defaultToggle = new tsl::elm::ToggleListItem("Default " + this->source, activeMod == "");
          defaultToggle->setStateChangedListener([this](bool state) {
            if (state) {
              controller.deactivateMod(this->source, this->group);
            }
          });

          this->toggles.push_back(defaultToggle);
          list->addItem(this->toggles[0]);

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
        }
    }

    // Called once every frame to update values
    virtual void update() override {

    }

    // Called once every frame to handle inputs not handled by other UI elements
    virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
        if (keysDown & HidNpadButton_B) {
            tsl::goBack();
            return true;
        }
        return false;
    }
};

