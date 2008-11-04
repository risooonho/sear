// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2008 Simon Goodall

#include "TerrainModHandler.h"

#include <Eris/Entity.h>
#include <Eris/View.h>

#include <Mercator/TerrainMod.h>

#include "TerrainMod.h"

namespace Eris {

bool TerrainModHandler::init() {

  assert(m_initialised == false);

  m_initialised = true;

  return true;
}

void TerrainModHandler::shutdown() {

  assert(m_initialised == true);

  while (!m_modMap.empty()) {
    TerrainModMap::iterator I = m_modMap.begin();
    delete I->second;
    m_modMap.erase(I);
  }

  m_initialised = false;
}

void TerrainModHandler::setView(View *view) {
  // TODO: Entiy Created does not appear to be fired
  // Perhaps setView is called too late in the start up process
  // Use of appearance and disappearance should also work, but lead to an
  // increased number of add/remove calls

  // view->EntityCreated.connect(sigc::mem_fun(this, &TerrainModHandler::onEntityCreated));
  //view->EntityDeleted.connect(sigc::mem_fun(this, &TerrainModHandler::onEntityDeleted));

  view->Appearance.connect(sigc::mem_fun(this, &TerrainModHandler::onEntityCreated));
  view->Disappearance.connect(sigc::mem_fun(this, &TerrainModHandler::onEntityDeleted));
}

void TerrainModHandler::onEntityCreated(Entity *e) {

  TerrainMod *tm = new TerrainMod(e, this);

  tm->EventModChanged.connect(sigc::mem_fun(*this, &TerrainModHandler::onEventModChanged));
  tm->EventModDeleted.connect(sigc::mem_fun(*this, &TerrainModHandler::onEventModChanged));

  tm->init();

  m_modMap[e->getId()] = tm;
}

void TerrainModHandler::onEntityDeleted(Entity *e) {

  // clean up
  TerrainModMap::iterator I = m_modMap.find(e->getId());
  if (I != m_modMap.end()) {
    delete I->second;
    m_modMap.erase(I);
  }
}

void TerrainModHandler::onEventModChanged(TerrainMod *tm) {

  // Perhaps we need an extra EventModAdded signal?1
//  TerrainModAdded.emit(tm->getEntity(), tm->getMod());
  TerrainModChanged.emit(tm->getEntity(), tm->getMod());
}

void TerrainModHandler::onEventModDeleted(TerrainMod *tm) {

  TerrainModDeleted.emit(tm->getEntity(), tm->getMod());
} 

} // namespace Eris