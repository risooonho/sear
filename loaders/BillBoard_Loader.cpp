// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2002 Simon Goodall

#include "src/System.h"
#include <string>

#include "src/ModelHandler.h"
#include "src/Render.h"

#include "BillBoard_Loader.h"
#include "BillBoard.h"

namespace Sear {

BillBoard_Loader::BillBoard_Loader(ModelHandler *mh) {
  mh->registerModelLoader("billboard", this);
}

BillBoard_Loader::~BillBoard_Loader() {
  // TODO: Add ability to unregister loader.
}

Model *BillBoard_Loader::loadModel(Render *render, ModelStruct &ms) {
  if (!render) return NULL;
  BillBoard *model = new BillBoard(render);

  std::string type = ms.type;
  int id = render->requestTexture("billboard", type);
  if (id == -1) {
    type = ms.parent;
    id = render->requestTexture("billboard", type);
  }
  if (id == -1) {
    // TODO: what happens if we still cannot find a texture?

  }
  model->init(type, ms.width, ms.height);
  return model;
}

} /* namespace Sear */

