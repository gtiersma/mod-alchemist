#include <switch.h>

#include "overlay.h"
#include "ui/ui_main.h"

void ModAlchemist::initServices() {
  pmdmntInitialize();
  pminfoInitialize();

  tsl::hlp::doWithSmSession([this]{
    if (hosversionAtLeast(8, 0, 0)) {
      clkrstInitialize();
    } else {
      pcvInitialize();
    }
  });
  
  envIsSyscallHinted(0x6F);
}
void ModAlchemist::exitServices() {
	clkrstExit();
	pcvExit();
  pminfoExit();
  pmdmntExit();
}

void ModAlchemist::onHide() {}
void ModAlchemist::onShow() {}

std::unique_ptr<tsl::Gui> ModAlchemist::loadInitialGui() {
  return initially<GuiMain>();
}