// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2005 Alistair Riddoch
// Copyright (C) 2007 - 2009 Simon Goodall

#include <sigc++/object_slot.h>

#include "guichan/Workarea.h"

#include "guichan/RootWidget.h"
#include "guichan/ConnectWindow.h"
#include "guichan/LoginWindow.h"
#include "guichan/CharacterWindow.h"
#include "guichan/Compass.h"
#include "guichan/Panel.h"
#include "guichan/StatusWindow.h"
#include "guichan/ActionListenerSigC.h"
#include "guichan/SpeechBubble.h"
#include "guichan/Alert.h"
#include "guichan/WFUTWindow.h"

#include "guichan/box.hpp"

#include "renderers/Render.h"
#include "renderers/RenderSystem.h"

#include "src/ActionHandler.h"
#include "src/FileHandler.h"
#include "src/Console.h"
#include "src/System.h"

#include <guichan/sdl.hpp>
#include <guichan/opengl.hpp>
#include <guichan/opengl/openglimage.hpp>
#include <guichan/opengl/openglsdlimageloader.hpp>

#include <sage/GL.h>

#include <iostream>
#include "imagefontxpm.h"
#include "rpgfont.h"

namespace Sear {

static const bool debug = false;

static const std::string WORKSPACE = "workspace";

static const std::string WORKAREA_TOGGLE = "workarea_toggle";
static const std::string WORKAREA_OPEN = "workarea_open";
static const std::string WORKAREA_CLOSE = "workarea_close";
static const std::string WORKAREA_ALERT = "workarea_alert";

static const std::string WORKAREA = "workarea";

static const std::string KEY_fixed_font = "fixed_font";
static const std::string KEY_fixed_font_characters = "fixed_font_characters";

// static const std::string DEFAULT_fixed_font = "${SEAR_INSTALL}/data/fixedfont.bmp";
// static const std::string DEFAULT_fixed_font_characters = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static const std::string DEFAULT_fixed_font = "${SEAR_INSTALL}/data/rpgfont.png";
static const std::string DEFAULT_fixed_font_characters = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.,!?-+/():;%&`'*#=[]\"<>{}^~|_@$\\";

Gui::Gui()
{
}

Gui::~Gui()
{
}

Workarea::Workarea(System * s) : 
  m_system(s), 
  m_input(0),
  m_graphics(0),
  m_imageLoader(0),
  m_gui(0),
  m_panel(0),
  m_connectWindow(0),
  m_top(0)
{
}

void Workarea::init()
{
  m_imageLoader = new gcn::OpenGLSDLImageLoader();

  // Set the loader that the OpenGLImageLoader should use to load images from
  // disk, as it can't do it itself, and then install the image loader into
  // guichan.
  gcn::Image::setImageLoader(m_imageLoader);

  // Create the handler for OpenGL graphics.
  m_graphics = new gcn::OpenGLGraphics();

  // Tell it the size of our screen.
  Render * render = RenderSystem::getInstance().getRenderer();
  m_width = render->getWindowWidth();
  m_height = render->getWindowHeight();
  m_graphics->setTargetPlane(m_width, m_height);

  m_input = new gcn::SDLInput();

  m_top = new RootWidget();
  m_top->setDimension(gcn::Rectangle(0, 0, m_width, m_height));
  m_top->setOpaque(false);
  m_top->setFocusable(true);
  m_top->setTabInEnabled(false);

  m_gui = new Gui();
  m_gui->setGraphics(m_graphics);
  m_gui->setInput(m_input);
  m_gui->setTop(m_top);

  try {
    std::string font_path = m_fixed_font;
    m_system->getFileHandler()->getFilePath(font_path);

    gcn::ImageFontXPM * font = new gcn::ImageFontXPM("default_font", rpgfont_xpm, m_fixed_font_characters);
    
    gcn::Widget::setGlobalFont(font);
  } catch (...) {
    std::cerr << "Failed to load font " << m_fixed_font << std::endl << std::flush;
  }

  gcn::Window* con_w = new ConnectWindow();
  m_top->add(con_w, m_width / 2 - con_w->getWidth() / 2, m_height / 2 - con_w->getHeight () / 2);

  m_panel = new Panel(m_top);
  // m_top->add(m_panel, 0, 0);
  m_top->setWindowCoords(m_panel, std::make_pair(0,0));
  m_windows["panel"] = m_panel;

  m_windows["connect"] = con_w;
  m_windows["login"] = new LoginWindow();
  m_windows["character"] = new CharacterWindow();
  m_windows["update"] = new WFUTWindow();
  m_windows["compass"] = new Compass();

  m_system->getActionHandler()->addHandler("connected", "/workarea_close connect");
  m_system->getActionHandler()->addHandler("connected", "/workarea_open login");
  m_system->getActionHandler()->addHandler("disconnected", "/workarea_alert Connection to server failed");
  m_system->getActionHandler()->addHandler("disconnected", "/workarea_open connect");

  m_system->getActionHandler()->addHandler("logged_in", "/workarea_close login");
  m_system->getActionHandler()->addHandler("logged_in", "/workarea_open character");

  m_system->getActionHandler()->addHandler("world_entered", "/workarea_close character");
  m_system->getActionHandler()->addHandler("world_entered", "/workarea_open panel");
  m_system->getActionHandler()->addHandler("avatar_failed", "/workarea_alert Unable to get character");

  m_system->getActionHandler()->addHandler("inventory_open", "/panel_toggle Inventory");

  RenderSystem::getInstance().ContextCreated.connect(sigc::mem_fun(*this, &Workarea::contextCreated));
  RenderSystem::getInstance().ContextDestroyed.connect(sigc::mem_fun(*this, &Workarea::contextDestroyed));
}

Workarea::~Workarea()
{
  delete m_gui;
  delete m_input;
  delete m_graphics;
  delete m_imageLoader;
  delete m_top;
}

void Workarea::registerCommands(Console * console)
{
  if (m_panel != 0) {
    dynamic_cast<Panel*>(m_panel)->registerCommands(console);
  }

  console->registerCommand(WORKAREA_TOGGLE, this);
  console->registerCommand(WORKAREA_OPEN, this);
  console->registerCommand(WORKAREA_CLOSE, this);
  console->registerCommand(WORKAREA_ALERT, this);
}

void Workarea::runCommand(const std::string & command, const std::string & args)
{
  if (command == WORKAREA_CLOSE) {
    if (debug) std::cout << "Got the workarea close command" << std::endl << std::flush;
    WindowDict::const_iterator I = m_windows.find(args);
    if (I != m_windows.end()) {
      gcn::Window* win = I->second;
      assert(win != 0);
      if (win->getParent() != 0) {
        m_top->closeWindow(win);
      }
    } else {
      std::cerr << "Asked to close unknown window " << args
                << std::endl << std::flush;
    }
  }
  else if (command == WORKAREA_OPEN) {
    WindowDict::const_iterator I = m_windows.find(args);
    if (I != m_windows.end()) {
      gcn::Window* win = I->second;
      assert(win != 0);
      if (win->getParent() == 0) {
        m_top->openWindow(win);
      }
    } else {
      std::cerr << "Asked to open unknown window " << args
                << std::endl << std::flush;
    }
  }
  else if (command == WORKAREA_TOGGLE) {
    WindowDict::const_iterator I = m_windows.find(args);
    if (I != m_windows.end()) {
      gcn::Window* win = I->second;
      assert(win != 0);
      if (win->getParent() != 0) {
        m_top->closeWindow(win);
      } else {
        m_top->openWindow(win);
      }
    } else {
      std::cerr << "Asked to open unknown window " << args
                << std::endl << std::flush;
    }
  }
  else if (command == WORKAREA_ALERT) {
    std::cerr << "Asked to open ALERT" << args
              << std::endl << std::flush;
    Alert * al = new Alert(m_top, args);
    // This should be deleted in removeLaters if all goes well...
    m_widgets.push_back(al);
    // m_top->openWindow(al);
  }
}

void Workarea::readConfig(varconf::Config & config)
{
  varconf::Variable temp;

  if (config.findItem(WORKAREA, KEY_fixed_font)) {
    temp = config.getItem(WORKAREA, KEY_fixed_font);
    m_fixed_font = (!temp.is_string()) ? (DEFAULT_fixed_font) : temp.as_string();
  } else {
    m_fixed_font = DEFAULT_fixed_font;
  }
  if (config.findItem(WORKAREA, KEY_fixed_font_characters)) {
    temp = config.getItem(WORKAREA, KEY_fixed_font_characters);
    m_fixed_font_characters = (!temp.is_string()) ? (DEFAULT_fixed_font_characters) : temp.as_string();
  } else {
    m_fixed_font_characters = DEFAULT_fixed_font_characters;
  }
}

void Workarea::writeConfig(varconf::Config & config)
{
  config.setItem(WORKAREA, KEY_fixed_font, m_fixed_font);
  config.setItem(WORKAREA, KEY_fixed_font_characters, m_fixed_font_characters);
}

void Workarea::resize()
{
  Render *render = RenderSystem::getInstance().getRenderer();
  int width = render->getWindowWidth(),
      height = render->getWindowHeight();
  m_graphics->setTargetPlane(width, height);
  m_top->resize(width, height, m_width, m_height);
  // m_top->setDimension(gcn::Rectangle(0, 0, width, height));
  m_width = width;
  m_height = height;
}

bool Workarea::handleEvent(const SDL_Event & event)
{
  gcn::FocusHandler *fh = m_gui->getFocusHandler();
  assert(fh != 0);

  gcn::Widget *focus = fh->getFocused();

  bool clear_focus = false;
  bool event_eaten = false;
  bool suppress = false;
  Panel *panel = dynamic_cast<Panel*>(m_panel);
  switch (event.type) {
    case SDL_MOUSEMOTION:
      // FIXME This should depend on whether the gui is visible.
      event_eaten = m_gui->getWidgetAt(event.motion.x, event.motion.y) != m_top;
      break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      // FIXME This should depend on whether the gui is visible.
      event_eaten = m_gui->getWidgetAt(event.button.x, event.button.y) != m_top;
      break;
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_RETURN) {
        if (panel != 0 && System::instance()->checkState(SYS_IN_WORLD) &&
            (focus == 0 || focus == m_top)) {
          suppress = panel->requestConsole();
        }
        event_eaten = true;
      } else if (event.key.keysym.sym == SDLK_SLASH) {
        if (panel != 0 && System::instance()->checkState(SYS_IN_WORLD) &&
            (focus == 0 || focus == m_top)) {
          panel->requestConsole();
        }
        event_eaten = true;
      } else if (event.key.keysym.sym == SDLK_ESCAPE) {
        if (panel != 0) {
          suppress = panel->dismissConsole();
        }
        event_eaten = true;
      } else {
        event_eaten = ((focus != 0) && (focus != m_top));
      }
    case SDL_KEYUP:
      event_eaten = ((focus != 0) && (focus != m_top));
      break;
    default:
      event_eaten = false;
      break;
  }

  if (!suppress) { m_input->pushInput(event); }

  if (clear_focus) {
    fh->focusNone();
  }

  focus = fh->getFocused();

  if ((focus != 0) && (focus != m_top)) {
    if (m_system->isMouselookEnabled()) {
      m_system->toggleMouselook();
    }
  }

  return event_eaten;
}

void Workarea::draw()
{

  RenderSystem::getInstance().switchState(RenderSystem::getInstance().requestState(WORKSPACE));
  glLineWidth(1.f);
  m_gui->logic();
  removeLaters();
  m_gui->draw();

  glLineWidth(4.f);
}

void Workarea::contextCreated() {
  try {
    std::string font_path = m_fixed_font;
    // Expand variables
    m_system->getFileHandler()->getFilePath(font_path);

    // Create ImageFont from data sources
    gcn::ImageFontXPM * font = new gcn::ImageFontXPM("default_font", rpgfont_xpm, m_fixed_font_characters);

    // Set default font
    gcn::Widget::setGlobalFont(font);
  } catch (...) {
    std::cerr << "Failed to load font " << m_fixed_font << std::endl << std::flush;
  }
  m_top->contextCreated();
}

void Workarea::contextDestroyed(bool check) {
  m_top->contextDestroyed(check);
  /*
  // Clean up global font
  m_top->setFont(NULL);
  gcn::Font * f = m_top->getFont();
  if (f) {
    gcn::Widget::setGlobalFont(NULL);
    delete f;
  }
  */
}

void Workarea::removeLaters() {
  std::list<gcn::Widget*>::iterator I = m_remove_widgets.begin();
  while (I != m_remove_widgets.end()) {
    gcn::Widget *w = *I;
    gcn::Widget *p = w->getParent();

    if (p) {
      gcn::Container *p2 = dynamic_cast<gcn::Container*>(p);
      if (p2) {
        p2->remove(w);
      }
    }

    m_remove_widgets.erase(I); 
    I = m_remove_widgets.begin();
  }
}

} // namespace Sear
