// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2004 Alistair Riddoch

#include "gui/Workspace.h"

#include "gui/RootWindow.h"

Workspace::Workspace() : m_rootWindow(new RootWindow()) {
}

Workspace::~Workspace() {
}