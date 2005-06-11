// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2005 Alistair Riddoch

#include "guichan/Workarea.h"

#include "guichan/RootWidget.h"
#include "guichan/LoginWindow.h"
#include "guichan/ConsoleWindow.h"
#include "guichan/ActionListenerSigC.h"

#include "guichan/box.hpp"

#include "renderers/Render.h"
#include "renderers/RenderSystem.h"

#include <guichan/sdl.hpp>
#include <guichan/opengl.hpp>

#include <iostream>

namespace Sear {

Gui::Gui()
{
}

Gui::~Gui()
{
}

Workarea::Workarea(System * s) : m_system(s), m_input(0)
{
  m_imageLoader = new gcn::OpenGLImageLoader();
  m_hostImageLoader = new gcn::SDLImageLoader();

  // Set the loader that the OpenGLImageLoader should use to load images from
  // disk, as it can't do it itself, and then install the image loader into
  // guichan.
  m_imageLoader->setHostImageLoader(m_hostImageLoader);
  gcn::Image::setImageLoader(m_imageLoader);

  // Create the handler for OpenGL graphics.
  m_graphics = new gcn::OpenGLGraphics();

  // Tell it the size of our screen.
  Render * render = RenderSystem::getInstance().getRenderer();
  int width = render->getWindowWidth(),
      height = render->getWindowHeight();
  m_graphics->setTargetPlane(width, height);

  m_input = new gcn::SDLInput();

  m_top = new RootWidget();
  m_top->setDimension(gcn::Rectangle(0, 0, width, height));
  m_top->setOpaque(false);

  m_gui = new Gui();
  m_gui->setGraphics(m_graphics);
  m_gui->setInput(m_input);
  m_gui->setTop(m_top);

  try {
    gcn::ImageFont * font = new gcn::ImageFont("/tmp/fixedfont.bmp", " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    // gcn::ImageFont * font = new gcn::ImageFont("/tmp/Font-Utopia.bmp", " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{!}~");
    gcn::Widget::setGlobalFont(font);
  } catch (...) {
    std::cout << "Failed to load font" << std::endl << std::flush;
  }

  // gcn::Image * image = new gcn::Image("/tmp/gui-chan.bmp");
  // gcn::Icon * icon = new gcn::Icon(image);

  // m_top->add(icon, 10, 30);

  LoginWindow * lw = new LoginWindow();
  // m_top->add(lw, width / 2 - lw->getWidth() / 2, height / 2 - lw->getHeight () / 2);
  ConsoleWindow * cw = new ConsoleWindow();
  // m_top->add(cw, 4, height - cw->getHeight() - 4);
}

Workarea::~Workarea()
{
  delete m_input;
  delete m_graphics;
  delete m_imageLoader;
}

void Workarea::resize(int x, int y)
{
  Render * render = RenderSystem::getInstance().getRenderer();
  int width = render->getWindowWidth(),
      height = render->getWindowHeight();
  m_graphics->setTargetPlane(width, height);
  m_top->setDimension(gcn::Rectangle(0, 0, width, height));
}

bool Workarea::handleEvent(const SDL_Event & event)
{
  gcn::FocusHandler * fh = m_gui->getFocusHandler();
  assert(fh != 0);

  gcn::Widget * focus = fh->getFocused();
  if (focus == 0) {
    std::cout << "Nothing is focused" << std::endl << std::flush;
  } else {
    std::cout << "A Widget is focused" << std::endl << std::flush;
  }

  bool gui_has_mouse = m_top->childHasMouse();

  bool clear_focus = false;
  bool event_eaten = false;

  switch (event.type) {
    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
      // FIXME This should depend on whether the gui is visible.
      event_eaten = gui_has_mouse;
      break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      clear_focus = (event.key.keysym.sym == SDLK_RETURN);
      event_eaten = (focus != 0);
      break;
    default:
      event_eaten = false;
      break;
  }

  m_input->pushInput(event);

  if (clear_focus) {
    fh->focusNone();
  }

  if (!event_eaten) {
      std::cout << "But not eaten" << std::endl << std::flush;
  } else {
      std::cout << "Eaten" << std::endl << std::flush;
  }
  return event_eaten;
}

void Workarea::draw()
{
  m_gui->logic();
  m_gui->draw();
}

} // namespace Sear
