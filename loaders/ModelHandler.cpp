// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2008 Simon Goodall, University of Southampton

#include <string.h>
#include <inttypes.h>

#include <sigc++/object_slot.h>

#include <Atlas/Message/Element.h>

#include <varconf/config.h>

#include <Eris/Timeout.h>

#include "common/Utility.h"

#include "renderers/RenderSystem.h"
#include "renderers/TextureManager.h"
#include "src/Console.h"
#include "ModelRecord.h"
#include "ObjectRecord.h"
#include "ModelLoader.h"
#include "src/FileHandler.h"
#include "src/System.h"
#include "src/WorldEntity.h"

#include "ModelHandler.h"
#include "ModelLoader.h"
#include "Model.h"
#include "ModelSystem.h"
#include "NullModel.h"

#include "SearObjectTypes.h"

#ifdef DEBUG
  static const bool debug = true;
#else
  static const bool debug = false;
#endif

namespace Sear {

static const std::string CMD_LOAD_MODEL_RECORDS = "load_model_records";
static const std::string CMD_dump_object = "dump_object";
static const std::string CMD_reload_config_models = "reload_config_models";
static const std::string CMD_unload_models = "unload_models";

static const std::string ATTR_GUISE= "guise";
static const std::string ATTR_MODE = "mode";

static const std::string KEY_STATE = "state";
static const std::string KEY_STATE_NUM = "state_num";
static const std::string KEY_SELECT_STATE = "select_state";
static const std::string KEY_SELECT_STATE_NUM = "select_state_num";

ModelHandler::ModelHandler() :
  m_initialised(false),
  m_timeout(NULL)
{}

ModelHandler::~ModelHandler() {
  if (m_initialised) shutdown();
}

void ModelHandler::init() {
  assert (m_initialised == false);

  m_model_records.sigsv.connect(sigc::mem_fun(this, &ModelHandler::varconf_callback));
  m_model_records.sige.connect(sigc::mem_fun(this, &ModelHandler::varconf_error_callback));

  m_timeout = new Eris::Timeout(60000);
  m_timeout->Expired.connect(sigc::mem_fun(this, &ModelHandler::TimeoutExpired));

  // Add default record
  m_model_records.setItem("default", ModelRecord::MODEL_LOADER, "wireframe");
  m_model_records.setItem("default", ModelRecord::STATE, "default");
  m_model_records.setItem("default", ModelRecord::SELECT_STATE, "select");
  m_model_records.setItem("default", ModelRecord::OUTLINE, false);

  m_initialised = true;
}

void ModelHandler::shutdown() {
  assert (m_initialised == true);

  delete m_timeout;

  // Clean up model loaders
  m_model_loaders.clear();
  // Delete all unique records
  m_object_map.clear();
  // Delete all remaining records
  m_model_records_map.clear();

  // Clear all model config filenames
  m_model_configs.clear();

  // Disconnect the sigc callbacks
  notify_callbacks();

  m_initialised = false;
}
  
SPtr<ModelRecord> ModelHandler::getModel(const std::string &model_id, WorldEntity *we) {
  assert (m_initialised == true);

  assert(we);
  // Model loaded for this object?

  // Composite object_id + model id (model record name)
  const std::string &id = we->getViewId() + model_id;

  // Look in per entity map
  ObjectRecordMap::const_iterator I = m_object_map.find(id);
  if (I != m_object_map.end()) {
    return I->second;
  }

  // Look in type map
  ModelRecordMap::const_iterator J = m_model_records_map.find(model_id);
  if (J != m_model_records_map.end()) {
    m_object_map[id] = J->second;
    return J->second;
  }

  // Need to create a new model
  std::string model_loader = (std::string)m_model_records.getItem(model_id, ModelRecord::MODEL_LOADER);

  // We are assuming that the boundbox loader is always available

  if (model_loader.empty()) {
    fprintf(stderr, "Model Loader not defined for %s. Using BoundBox.\n", model_id.c_str());
    model_loader = "boundbox";
  }

  ModelLoaderMap::iterator K = m_model_loaders.find(model_loader);
  ModelLoaderMap::const_iterator Kend = m_model_loaders.end();
  if (K == Kend) {
    fprintf(stderr, "Unknown Model Loader for %s. Using BoundBox.\n", model_id.c_str());
    model_loader = "boundbox";
    K = m_model_loaders.find(model_loader);
  }

  SPtr<ModelRecord> model;
  if (K != Kend) {
    model = K->second->loadModel(we, model_id, m_model_records);
  } else {
    fprintf(stderr, "No loader found (%s) for %s\n ", model_loader.c_str(), model_id.c_str());
//    return model;
  }
  
  // Check model was loaded, and fall back to a NullModel on error
  if (!model) {
    fprintf(stderr, "Error loading model of type %s for %s\n", model_loader.c_str(), model_id.c_str());
    model = SPtr<ModelRecord>(new ModelRecord);
    model->model = SPtr<Model>(new NullModel());
  }

  // Set initial animation
  if (we->hasAttr(ATTR_MODE)) {
    model->model->animate(we->valueOfAttr(ATTR_MODE).asString());
  }

  // If model is a generic one, add it to the generic list
  if (model->model_by_type) m_model_records_map[model_id] = model;

  // Store per entity model
  m_object_map[id] = model;

  return model; 
}

void ModelHandler::registerModelLoader(SPtr<ModelLoader> model_loader) {
  assert (m_initialised == true);
  const std::string &model_type = model_loader->getType();

  // Throw error if we already have a loader for this type
  assert(m_model_loaders.find(model_type) == m_model_loaders.end());

  // If all is well, assign loader
  m_model_loaders[model_type] = model_loader;
}

void ModelHandler::unregisterModelLoader(const std::string &model_type) {
  assert (m_initialised == true);
  // Only unregister a model laoder if it is properly registered
  ModelLoaderMap::iterator I = m_model_loaders.find(model_type);
  if (I != m_model_loaders.end()) {
    m_model_loaders.erase(I);
  }
}

void ModelHandler::checkModelTimeouts(bool forceUnload) {
  assert (m_initialised == true);
  // This function checks to see when the last time a model record was rendered.
  // If the time has been longer than a threshold, we unload the model record 
  // and associated models.

  if (debug) printf("[ModelHandler] Checking Timeouts\n");

  // Do the same again for the object map
  ModelRecordMap::iterator Jend = m_object_map.end();
  ObjectRecordMap::iterator J = m_object_map.begin();
  while (J != Jend) {
    SPtr<ModelRecord> record = J->second;
    SPtr<Model> model = record->model;
    bool unload = forceUnload;
    if (!unload && model) {
      if (System::instance()->getTimef() - model->getLastTime() > 60.0f) {
        unload = true;
      }
    }
    if (unload) m_object_map.erase(J++);
    else ++J;
  }

  // Loop through and find all objects that have expired and add to set object
  ModelRecordMap::iterator Iend = m_model_records_map.end();
  ModelRecordMap::iterator I = m_model_records_map.begin();
  while (I != Iend) {
    SPtr<ModelRecord> record = I->second;
    SPtr<Model> model = record->model;
    bool unload = forceUnload;
    if (!unload && model) {
      if (System::instance()->getTimef() - model->getLastTime() > 60.0f) {
        unload = true;
      }
    }
    if (unload) m_model_records_map.erase(I++);
    else ++I;
  }
}

void ModelHandler::TimeoutExpired() {
  assert (m_initialised == true);
  checkModelTimeouts(false);
  m_timeout->reset(60000);
}

void ModelHandler::loadModelRecords(const std::string &filename) {
  assert (m_initialised == true);
  m_model_records.readFromFile(filename);
}

void ModelHandler::varconf_callback(const std::string &section, const std::string &key, varconf::Config &config) {
  // Convert textual state name to the state number
  if (key == KEY_STATE) {
    int stn = RenderSystem::getInstance().requestState(config.getItem(section, key));
    assert(stn);
    config.setItem(section, KEY_STATE_NUM, stn);
  }
  else if (key == KEY_SELECT_STATE) {
    int stn = RenderSystem::getInstance().requestState(config.getItem(section, key));
    assert(stn);
    config.setItem(section, KEY_SELECT_STATE_NUM, stn);
  }
}

void ModelHandler::varconf_error_callback(const char *message) {
  fprintf(stderr, "[ModelHandler] %s\n", message);
}

void ModelHandler::registerCommands(Console *console) {
  assert(m_initialised == true);
  assert(console != NULL);
  
  console->registerCommand(CMD_LOAD_MODEL_RECORDS, this);
  console->registerCommand(CMD_dump_object, this);
  console->registerCommand(CMD_reload_config_models, this);
  console->registerCommand(CMD_unload_models, this);
}

void ModelHandler::runCommand(const std::string &command, const std::string &args) {
  assert (m_initialised == true);
  if (command == CMD_LOAD_MODEL_RECORDS) {
    std::string args_cpy = args;
    m_model_configs.push_back(args);
    System::instance()->getFileHandler()->getFilePath(args_cpy);
    loadModelRecords(args_cpy);
  } else if (command == CMD_dump_object) {
    // Quick hack to save some SearObject files.
    // It should really have its own function somewhere, and even
    // be in another utility.
    Tokeniser tok;
    tok.initTokens(args);
    const std::string &id = tok.nextToken();
    const std::string &filename = tok.remainingTokens();

    ModelRecordMap::const_iterator I = m_model_records_map.find(id);
    if (I == m_model_records_map.end()) return;

    SPtr<Model> model = I->second->model;
    if (model->hasStaticObjects() == false) return;
    const StaticObjectList &sol = model->getStaticObjects();
    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp) {
      fprintf(stderr, "[ModelHandler] Error opening %s for writing\n", filename.c_str());
      return;
    }
    SearObjectHeader soh;

    strncpy(soh.magic, "SEARSTAT", 8);
    soh.byte_order = 0xFF00;
    soh.version = 1;
    soh.num_meshes = sol.size();
    fwrite(&soh, sizeof(SearObjectHeader), 1, fp);

    StaticObjectList::const_iterator J = sol.begin();
    StaticObjectList::const_iterator Jend = sol.end();
    SearObjectMesh som;
    TextureManager *tm = RenderSystem::getInstance().getTextureManager();
    assert (tm != 0);
    for (; J != Jend; ++J) {
      StaticObject* so = *J;
      assert(so);
      so->getMatrix().getMatrix(som.mesh_transform);
      so->getTexMatrix().getMatrix(som.texture_transform);

      int t_id, tm_id;
      so->getTexture(0, t_id, tm_id);
      std::string tex_name = tm->getTextureName(t_id);

      memset(som.texture_map, '\0', 256); 
      strncpy(som.texture_map, tex_name.c_str(), tex_name.size());
      som.num_vertices = so->getNumPoints();
      som.num_faces = so->getNumFaces();

      so->getAmbient(som.ambient);
      so->getDiffuse(som.diffuse);
      so->getSpecular(som.specular);
      so->getEmission(som.emissive);

      som.shininess = so->getShininess();

      fwrite(&som, sizeof(SearObjectMesh), 1, fp);

      float *fptr = so->getVertexDataPtr();
      fwrite(fptr, sizeof(float), so->getNumPoints() * 3, fp);

      fptr = so->getNormalDataPtr();
      fwrite(fptr, sizeof(float), so->getNumPoints() * 3, fp);

      fptr = so->getTextureDataPtr();
      fwrite(fptr, sizeof(float), so->getNumPoints() * 2, fp);

      int *iptr = so->getIndicesPtr();
      fwrite(iptr, sizeof(uint32_t), so->getNumFaces() * 3, fp);
    }

    fclose(fp);
  }
  else
  if (command == CMD_reload_config_models) {
    // Force model unloading
    checkModelTimeouts(true);
    // Force a context cleanup
    contextDestroyed(true);
    // We need to completely re-load this information
    m_model_records = varconf::Config();

    m_model_records.sige.connect(sigc::mem_fun(this, &ModelHandler::varconf_error_callback));
    m_model_records.sigsv.connect(sigc::mem_fun(this, &ModelHandler::varconf_callback));

    // Add default record
    m_model_records.setItem("default", ModelRecord::MODEL_LOADER, "wireframe");
    m_model_records.setItem("default", ModelRecord::STATE, "default");
    m_model_records.setItem("default", ModelRecord::SELECT_STATE, "select");
    m_model_records.setItem("default", ModelRecord::OUTLINE, false);

    std::list<std::string>::const_iterator I = m_model_configs.begin();
    std::list<std::string>::const_iterator Iend = m_model_configs.end();
    while (I != Iend) {
      std::string args_cpy = *I++;
      System::instance()->getFileHandler()->getFilePath(args_cpy);
      loadModelRecords(args_cpy);
    }
    contextCreated();
  }
  else
  if (command == CMD_unload_models) {
    checkModelTimeouts(false);
  }
}
void ModelHandler::contextCreated() {
  assert (m_initialised == true);

  for (ObjectRecordMap::iterator I = m_object_map.begin(); I != m_object_map.end(); ++I) {
    SPtr<ModelRecord> record = I->second;

    SPtr<Model> model = record->model;
    if (model) {
      model->contextCreated();
    }
  }
}

void ModelHandler::contextDestroyed(bool check) {
  assert (m_initialised == true);

  for (ObjectRecordMap::iterator I = m_object_map.begin(); I != m_object_map.end(); ++I) {
    SPtr<ModelRecord> record = I->second;

    SPtr<Model> model = record->model;
    if (model) {
      model->contextDestroyed(check);
    }
  }
}

void ModelHandler::reset() {
  assert (m_initialised == true);
  checkModelTimeouts(true);
}


PosAndOrient Model::getPositionForSubmodel(const std::string& submodelName) const {
  PosAndOrient po;
  po.pos = WFMath::Vector<3>(0.0f,0.0f,0.0f);
  po.orient = WFMath::Quaternion(1.0f, 0.0f,0.0f,0.0f);
  return po;
}


} /* namespace Sear */
