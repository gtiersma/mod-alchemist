#include "ui/ui_rando_mods.h"

GuiRandoMods::GuiRandoMods(
    Controller& controller_,
    const std::string& source_,
    const std::string& group_,
    const std::vector<Mod>& mods_
) : controller(controller_), source(source_), group(group_), mods(mods_) { }

tsl::elm::Element* GuiRandoMods::createUI() {
  auto frame = new tsl::elm::OverlayFrame("The Mod Alchemist", this->source + " - Randomization Ratings");

  auto list = new tsl::elm::List();

  auto defaultLabel = new tsl::elm::CategoryHeader("Default " + this->source);
  this->defaultSlider = new tsl::elm::TrackBar("\u0020");
  this->defaultSlider->setProgress(this->controller.getModlessRating(this->group, this->source));
  list->addItem(defaultLabel);
  list->addItem(this->defaultSlider);

  for (const Mod& mod : this->mods) {
    auto label = new tsl::elm::CategoryHeader(mod.name);
    auto slider = new tsl::elm::TrackBar("\u0020");
    slider->setProgress(mod.rating);
    this->sliders.push_back(slider);
    list->addItem(label);
    list->addItem(slider);
    this->sliders.push_back(slider);
  }

  frame->setContent(list);
  return frame;
}

bool GuiRandoMods::handleInput(
  u64 keysDown,
  u64 keysHeld,
  const HidTouchState &touchPos,
  HidAnalogStickState joyStickPosLeft,
  HidAnalogStickState joyStickPosRight
) {
  if (keysDown & HidNpadButton_B) {
    for (std::size_t i = 0; i < this->sliders.size(); ++i) {
      this->mods[i].rating = sliders[i]->getProgress();
    }

    this->controller.saveRatings(this->mods, this->source, this->group);
    this->controller.saveModlessRating(this->defaultSlider->getProgress(), this->source, this->group);

    tsl::goBack();
    return true;
  }
  return false;
}