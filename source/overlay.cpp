#include <switch.h>

#include "overlay.h"
#include "ui/ui_groups.h"

void ModAlchemist::initServices() {
  pmdmntInitialize();
  pminfoInitialize();
}
void ModAlchemist::exitServices() {
  pminfoExit();
  pmdmntExit();
}

void ModAlchemist::onHide() {}
void ModAlchemist::onShow() {}

std::unique_ptr<tsl::Gui> ModAlchemist::loadInitialGui() {
  return initially<GuiGroups>();
}