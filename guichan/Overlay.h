// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2005 Alistair Riddoch

#ifndef SEAR_GUICHAN_OVERLAY_H
#define SEAR_GUICHAN_OVERLAY_H

#include <guichan.hpp>

#include <Atlas/Objects/RootOperation.h>

#include <sigc++/object.h>

namespace gcn {
}

namespace Eris {
  class Entity;
}

namespace Sear {

class RootWidget;
class SpeechBubble;

class Overlay : virtual public SigC::Object {
public:
  typedef std::map<Eris::Entity *, SpeechBubble *> BubbleMap;
protected:
  Overlay();
  ~Overlay();

  RootWidget * m_top;
  std::map<Eris::Entity *, SpeechBubble *> m_bubbles;

  static Overlay * m_instance;
public:
  static Overlay * instance() {
    if (m_instance == 0) {
      m_instance = new Overlay();
    }
    return m_instance;
  }

  void heard(Eris::Entity *, const Atlas::Objects::Operation::RootOperation &);

  void logic(RootWidget *);
};

} // namespace Sear

#endif // SEAR_GUICHAN_OVERLAY_H