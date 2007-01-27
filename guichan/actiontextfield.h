/*
 *  The Mana World
 *  Copyright 2004 The Mana World Development Team
 *  Copyright (C) 2005 Alistair Riddoch
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Modified by members of the WorldForge project to work without
 *  The Mana World's enhanced widget rendering.
 *
 */

#ifndef SEAR_GUICHAN_ACTIONTEXTFIELD_H
#define SEAR_GUICHAN_ACTIONTEXTFIELD_H

#include <guichan/widgets/textfield.hpp>

namespace Sear {

/**
 * A password field.
 *
 * \ingroup GUI
 */
class ActionTextField : public gcn::TextField {
    public:
        /**
         * Constructor, initializes the password field with the given string.
         */
        ActionTextField(const std::string& text = "",  gcn::ActionListener *a = 0, const std::string &event = "");

         virtual void keyPress(const gcn::Key& key);
  private:
    gcn::ActionListener *m_action;
    std::string m_event;

};

}

#endif // SEAR_GUICHAN_ACTIONTEXTFIELD_H
