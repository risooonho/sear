// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2002 Simon Goodall

// $Id: Slice_Loader.cpp,v 1.5 2002-09-08 16:15:01 simon Exp $

#include "src/System.h"
#include <string>

#include "src/Render.h"
#include "src/ModelHandler.h"

#include "Slice_Loader.h"
#include "Slice.h"

namespace Sear {

Slice_Loader::Slice_Loader(ModelHandler *mh) {
  mh->registerModelLoader("slice", this);
}

Slice_Loader::~Slice_Loader() {
  // TODO: Add ability to unregister loader.
}

Model *Slice_Loader::loadModel(Render *render, ModelStruct &ms) {
  Slice *model = new Slice(render);

  std::string type = ms.type;
  int id = render->requestMipMap("slice", type + "_0_0", true);
  if (id == -1 && ms.parent) {
    type = ms.parent;
    id = render->requestMipMap("slice", type + "_0_0", true);
  }
  if (id == -1) {
    // TODO: what happens if we still cannot find a texture?

  }
  Model *trunk_model = System::instance()->getModelHandler()->getModel(System::instance()->getGraphics()->getRender(), type + "_trunk", System::instance()->getObjectLoader()->getObjectProperties(type + "_trunk"));
  if (!model->init(type, ms.width, ms.height, trunk_model, ms.num_slicings, ms.slices_per_slicing)) {
    model->shutdown();
    delete model;
    return NULL;
  }
  return model;
}

} /* namespace Sear */

