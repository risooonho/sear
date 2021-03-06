// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2005 Alistair Riddoch

#ifndef SEAR_GUICHAN_FOO_OPTIONS_H
#define SEAR_GUICHAN_FOO_OPTIONS_H

#include <guichan/widgets/window.hpp>
#include <guichan/actionlistener.hpp>

#include <sigc++/object.h>

namespace Sear {

class RootWidget;

class OptionsTemplate : public gcn::Window, public gcn::ActionListener {
protected:
  RootWidget * m_top;
public:
  explicit OptionsTemplate(RootWidget * top);
  virtual ~OptionsTemplate();

  void action(const gcn::ActionEvent&);
};

} // namespace Sear

#endif // SEAR_GUICHAN_FOO_OPTIONS_H
