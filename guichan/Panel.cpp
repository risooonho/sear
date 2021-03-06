// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2005 Alistair Riddoch

#include "guichan/Panel.h"

#include "guichan/ActionListenerSigC.h"
#include "guichan/ConsoleWindow.h"
#include "guichan/OptionsWindow.h"
#include "guichan/Inventory.h"
#include "guichan/RootWidget.h"
#include "guichan/box.hpp"

#include "renderers/Render.h"
#include "renderers/RenderSystem.h"

#include "src/System.h"
#include "src/Console.h"

#include <guichan.hpp>

#include <sigc++/bind.h>
#include <sigc++/hide.h>
#include <sigc++/object_slot.h>

#include <iostream>

namespace Sear {

static std::string PANEL_OPEN = "panel_open";
static std::string PANEL_CLOSE = "panel_close";
static std::string PANEL_TOGGLE = "panel_toggle";

Panel::Panel(RootWidget * top) : gcn::Window(""), m_top(top)
{
  gcn::Color base = getBaseColor();
  base.a = 128;
  setBaseColor(base);

  // This used to be 0, however setting to 1 to workaround a Guichan bug.
  setTitleBarHeight(1);
  setMovable(false);

  // setOpaque(true);

  m_hbox = new gcn::HBox(6);

  m_buttonListener = new ActionListenerSigC;
  m_buttonListener->Action.connect(sigc::mem_fun(*this, &Panel::actionPressed));

  add(m_hbox);

  Render * render = RenderSystem::getInstance().getRenderer();
  int height = render->getWindowHeight();

  m_console = new ConsoleWindow;
  addWindow(m_console);
  m_top->setWindowCoords(m_console, std::make_pair(2, height - m_console->getHeight() - 2));

  addWindow(new Inventory);
  addWindow(new OptionsWindow(m_top));
}

Panel::~Panel()
{
  delete m_hbox;

  // m_console is used as a SPtr from addWindow function
  // delete m_console;
  delete m_buttonListener;
}

void Panel::registerCommands(Console * console)
{
  console->registerCommand(PANEL_OPEN, this);
  console->registerCommand(PANEL_CLOSE, this);
  console->registerCommand(PANEL_TOGGLE, this);
}

void Panel::runCommand(const std::string & command, const std::string & args)
{
  if (command == PANEL_CLOSE) {
    std::cout << "Got the panel close command" << std::endl << std::flush;
    WindowDict::const_iterator I = m_windows.find(args);
    if (I != m_windows.end()) {
      SPtr<gcn::Window> win = I->second;
      assert(win != 0);
      if (win->getParent() != 0) {
        m_top->closeWindow(win.get());
      }
    }
  }
  else if (command == PANEL_OPEN) {
    WindowDict::const_iterator I = m_windows.find(args);
    if (I != m_windows.end()) {
      SPtr<gcn::Window> win = I->second;
      assert(win != 0);
      if (win->getParent() == 0) {
        m_top->openWindow(win.get());
      }
    }
  }
  else if (command == PANEL_TOGGLE) {
    WindowDict::const_iterator I = m_windows.find(args);
    if (I != m_windows.end()) {
      SPtr<gcn::Window> win = I->second;
      assert(win != 0);
      if (win->getParent() == 0) {
        m_top->openWindow(win.get());
      } else {
        m_top->closeWindow(win.get());
      }
    }
  }
}

void Panel::actionPressed(std::string event)
{
  WindowDict::const_iterator I = m_windows.find(event);
  if (I != m_windows.end()) {
    SPtr<gcn::Window> win = I->second;
    assert(win != 0);
    if (win->getParent() == 0) {
      m_top->openWindow(win.get());
    } else {
      m_top->closeWindow(win.get());
    }
  } else {
    std::cout << "Say what?" << std::endl << std::flush;
  }
}

void Panel::addWindow(gcn::Window * window)
{
  gcn::Button * button = new gcn::Button(window->getCaption());
  button->setFocusable(false);
  button->setActionEventId(window->getCaption());
  button->addActionListener(m_buttonListener);
  m_hbox->pack(button);

  m_buttons[window->getCaption()] = SPtr<gcn::Button>(button);
  m_windows[window->getCaption()] = SPtr<gcn::Window>(window);

  resizeToContent();
}

bool Panel::requestConsole()
{
  if (m_console->getParent() != 0) {
//    std::cout << "Goo" << std::endl << std::flush;
    return m_console->requestConsoleFocus();
  } else {
    m_top->openWindow(m_console);
    return m_console->requestConsoleFocus();
  }
  return false;
}

bool Panel::dismissConsole()
{
  if (m_console->getParent() != 0) {
    if (!m_console->dismissConsoleFocus()) {
      m_top->closeWindow(m_console);
    }
    return true;
  }
  return false;
}

} // namespace Sear
