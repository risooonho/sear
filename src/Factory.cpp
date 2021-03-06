// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2006 Simon Goodall, University of Southampton

// $Id: Factory.cpp,v 1.10 2006-05-17 23:15:35 alriddoch Exp $

#include "Factory.h"

#include <Eris/View.h>
#include <Eris/Connection.h>

#include <Atlas/Objects/RootEntity.h>

#include "System.h"
#include "renderers/Graphics.h"
#include "environment/Environment.h"
#include "TerrainEntity.h"

namespace Sear {

   Eris::EntityPtr Factory::instantiate(const Atlas::Objects::Entity::RootEntity & ge, Eris::TypeInfo *type, Eris::View *view) {
    if (type->isA(terrainType)) {
      return new TerrainEntity(ge->getId(), type, view);
    } else {
      WorldEntity *we = new WorldEntity(ge->getId(), type, view);
      if (type->getName() == "weather") {
        Environment::getInstance().setWeatherEntity(we);
      }
      return we;
    }
  }

} /* namespace Sear */

