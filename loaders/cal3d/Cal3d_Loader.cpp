// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2007 Simon Goodall

// $Id: Cal3d_Loader.cpp,v 1.23 2007-04-01 19:00:21 simon Exp $

#include <varconf/Config.h>

#include "src/System.h"
#include "src/FileHandler.h"

#include "ModelHandler.h"
#include "ModelRecord.h"
#include "ObjectRecord.h"
#include "ModelSystem.h"

#include "Cal3d_Loader.h"
#include "Cal3dModel.h"
#include "CoreModelHandler.h"

#ifdef DEBUG
  static const bool debug = true;
#else
  static const bool debug = false;
#endif

namespace Sear {
	
const std::string Cal3d_Loader::CAL3D = "cal3d";
	
Cal3d_Loader::Cal3d_Loader() {
  m_core_model_handler = SPtrShutdown<CoreModelHandler>(new CoreModelHandler());
  m_core_model_handler->init();
}

Cal3d_Loader::~Cal3d_Loader() {
  m_core_model_handler.release();
}

SPtr<ModelRecord> Cal3d_Loader::loadModel(WorldEntity *we, const std::string &model_id, varconf::Config &model_config) {

  SPtr<ModelRecord> model_record = ModelLoader::loadModel(we, model_id, model_config);

  assert(model_record);

  std::string file_name = model_record->data_file_path;
  // Expand variable in string
  System::instance()->getFileHandler()->getFilePath(file_name);
 
  try {
    Cal3dModel *model = m_core_model_handler->instantiateModel(file_name);
  
    if (!model) {
      std::cerr << "Unable to load model" << std::endl;	  
      return SPtr<ModelRecord>();
    }

    model_record->model = SPtrShutdown<Model>(model);
  } catch (...) {
    std::cerr << "Cal3d_Loader: Unknown Exception" << std::endl;
    return SPtr<ModelRecord>();
  }

  return model_record;
}

} /* namespace Sear */

