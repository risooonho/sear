// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2005 Simon Goodall, University of Southampton

// $Id: Graphics.cpp,v 1.3 2005-02-21 14:16:46 simon Exp $

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sage/sage.h>

#include <varconf/Config.h>
#include <Eris/Entity.h>
#include <Eris/View.h>
#include <wfmath/quaternion.h>
#include <wfmath/vector.h>

#include "common/Log.h"
#include "common/Utility.h"
#include "environment/Environment.h"
#include "GL.h"
#include "renderers/Sprite.h"
#include "Camera.h"
#include "src/Character.h"
#include "src/Console.h"
#include "src/Exception.h"
#include "Frustum.h"
#include "Light.h"
#include "LightManager.h"
#include "loaders/Model.h"
#include "loaders/ModelRecord.h"
#include "loaders/ObjectRecord.h"
#include "loaders/ObjectHandler.h"
#include "Render.h"
#include "src/System.h"
#include "src/WorldEntity.h"
#include "src/client.h"

#include "renderers/RenderSystem.h"
#include "gui/Compass.h"
#include "gui/Workspace.h"

#ifdef USE_MMGR
  #include "common/mmgr.h"
#endif

#ifdef DEBUG
  static const bool debug = true;
#else
  static const bool debug = false;
#endif

namespace Sear {

 extern void renderDome(float, int, int);
static const std::string GRAPHICS = "graphics";
	
static const std::string DEFAULT = "default";
static const std::string FONT = "font";
static const std::string STATE = "state";
static const std::string SELECT = "select_state";
	  // Consts
  static const int sleep_time = 5000;
  
  // Config key strings
  static const std::string KEY_use_textures = "render_use_textures";
  static const std::string KEY_use_lighting = "render_use_lighting";
  static const std::string KEY_show_fps = "render_show_fps";
  static const std::string KEY_use_stencil = "render_use_stencil";

  static const std::string KEY_fire_ac = "fire_attenuation_constant";
  static const std::string KEY_fire_al = "fire_attenuation_linear";
  static const std::string KEY_fire_aq = "fire_attenuation_quadratic";

  static const std::string KEY_fire_amb_red = "fire_ambient_red";
  static const std::string KEY_fire_amb_green = "fire_ambient_green";
  static const std::string KEY_fire_amb_blue = "fire_ambient_blue";
  static const std::string KEY_fire_amb_alpha = "fire_ambient_alpha";
  static const std::string KEY_fire_diff_red = "fire_diffuse_red";
  static const std::string KEY_fire_diff_green = "fire_diffuse_green";
  static const std::string KEY_fire_diff_blue = "fire_diffuse_blue";
  static const std::string KEY_fire_diff_alpha = "fire_diffuse_alpha";
  static const std::string KEY_fire_spec_red = "fire_specular_red";
  static const std::string KEY_fire_spec_green = "fire_specular_green";
  static const std::string KEY_fire_spec_blue = "fire_specular_blue";
  static const std::string KEY_fire_spec_alpha = "fire_specular_alpha";
 
  // Default config values
  static const float DEFAULT_use_textures = true;
  static const float DEFAULT_use_lighting = true;
  static const float DEFAULT_show_fps = true;
  static const float DEFAULT_use_stencil = true;

  static const float DEFAULT_fire_ac = 0.7f;
  static const float DEFAULT_fire_al = 0.2f;
  static const float DEFAULT_fire_aq = 0.15f;

  static const float DEFAULT_fire_amb_red = 0.0f;
  static const float DEFAULT_fire_amb_green = 0.0f;
  static const float DEFAULT_fire_amb_blue = 0.0f;
  static const float DEFAULT_fire_amb_alpha = 0.0f;
  static const float DEFAULT_fire_diff_red = 1.0f;
  static const float DEFAULT_fire_diff_green = 1.0f;
  static const float DEFAULT_fire_diff_blue = 0.9f;
  static const float DEFAULT_fire_diff_alpha = 0.0f;
  static const float DEFAULT_fire_spec_red = 0.0f;
  static const float DEFAULT_fire_spec_green = 0.0f;
  static const float DEFAULT_fire_spec_blue = 0.0f;
  static const float DEFAULT_fire_spec_alpha = 0.0f;
 
Graphics::Graphics(System *system) :
  m_system(system),
  m_renderer(NULL),
  m_character(NULL),
  m_camera(NULL),
  m_num_frames(0),
  m_frame_time(0),
  m_initialised(false),
  m_compass(NULL)
{
}

Graphics::~Graphics() {
  if (m_initialised) shutdown();
}

void Graphics::init() {
  if (m_initialised) shutdown();
  // Read  Graphics config options
  readConfig();
  // Add callbeck to detect updated options
  m_system->getGeneral().sigsv.connect(SigC::slot(*this, &Graphics::varconf_callback));
  // Register console commands

  // Create the default camera
  m_camera = new Camera();
  m_camera->init();
  m_camera->registerCommands(m_system->getConsole());
  
  // Create the compass
  m_compass = new Compass(580.f, 50.f);
  m_compass->setup();

  // Create the LightManager    
  m_lm = new LightManager();

  m_initialised = true;
}

void Graphics::shutdown() {
  assert(m_initialised);
  writeConfig();

  if (m_camera) {
    m_camera->shutdown();
    delete m_camera;
    m_camera = NULL;
  }
 
  if (m_compass) {
    delete m_compass;
    m_compass = NULL;
  } 

  if (m_lm) {  
    m_lm->shutdown();
    delete m_lm;
    m_lm = NULL;
  }

  m_initialised = false;
}

void Graphics::drawScene(bool select_mode, float time_elapsed) {
  if (!m_renderer) {
    std::cerr << "No Render object to render with!" << std::endl;
    return;
  }
  if (select_mode) m_renderer->resetSelection();
  if (m_camera) m_camera->updateCameraPos(time_elapsed);

  m_renderer->beginFrame();
  drawWorld(select_mode, time_elapsed);

  if (!select_mode) {
/* Removed for release
 
    Workspace *ws = m_system->getWorkspace();
    if (ws) ws->draw();
    else throw Exception("Error no Workspace object");
*/
    Console *con = m_system->getConsole();
    if (con) con->draw();
    else throw Exception("Error no Console object");
  }

  if (!select_mode) {
    // Only update on a viewable frame
    m_frame_time += time_elapsed;
    m_frame_rate = (float)m_num_frames++ / m_frame_time;
    if (m_frame_time > 1.0f) {
      std::string fr = string_fmt(m_frame_rate);
      SDL_WM_SetCaption(fr.c_str(), fr.c_str());
      m_num_frames = 0;
      m_frame_time = 0.0f;
    }
  }

  if (!select_mode) m_renderer->renderActiveName();

  RenderSystem::getInstance().switchState(RenderSystem::getInstance().requestState("cursor"));
  int mouse_x, mouse_y;
  SDL_GetMouseState(&mouse_x, &mouse_y);
  mouse_y = m_renderer->getWindowHeight() - mouse_y - 32;
  glColor3f(1.0f, 1.0f, 1.0f);
  m_renderer->drawTextRect(mouse_x, mouse_y, 32, 32, RenderSystem::getInstance().getMouseCursor());

  m_renderer->endFrame(select_mode);
}

void Graphics::drawWorld(bool select_mode, float time_elapsed) {
  static WFMath::Vector<3> y_vector = WFMath::Vector<3>(0.0f, 1.0f, 0.0f);
  static WFMath::Vector<3> z_vector = WFMath::Vector<3>(0.0f, 0.0f, 1.0f);
/*
Camera coords
//Should be stored in camera object an updated as required
x = cos elevation * cos rotation * distance * distance;
y = cos elevation * din rotation * distance * distance;
z = sin elevation * distance;

((CAMERA + CHAR_POS) - ENTITY_POS)^2 = D^2

Compare D^2 to choose what detail level to use

*/

	
  m_lm->reset();
  WFMath::Point<3> pos(0,0,0); // Initial camera position
  Eris::Avatar *avatar = m_system->getClient()->getAvatar();
  if (m_system->checkState(SYS_IN_WORLD) && avatar) {
    if (!m_character) m_character = m_system->getCharacter();
    //WorldEntity *focus = dynamic_cast<WorldEntity *>(view->getTopLevel()); //Get the player character entity
    WorldEntity *focus = dynamic_cast<WorldEntity *>(avatar->getEntity()); //Get the player character entity
    if (focus) {
      std::string id = focus->getId();
      static WFMath::Quaternion quaternion_by_90 = WFMath::Quaternion(z_vector, WFMath::Pi / 2.0f);
      m_orient = WFMath::Quaternion(1.0f, 0.0f, 0.0f, 0.0f); // Initial Camera rotation
      m_orient /= quaternion_by_90; // Rotate by 90 degrees as WF 0 degrees is East
      pos = focus->getAbsPos();
      float x = pos.x(),
            y = pos.y(),
            z = pos.z();
      
      // Apply camera rotations
      m_orient /= WFMath::Quaternion(y_vector, m_camera->getElevation());
      m_orient /= WFMath::Quaternion(z_vector, m_camera->getRotation());
      
      if (m_character) m_orient /= WFMath::Quaternion(z_vector,  m_character->getAngle());

      // Draw Sky box, requires the rotation to be done before any translation to keep the camera centered
      if (!select_mode) {
	m_renderer->store();
        m_renderer->applyQuaternion(m_orient);
        Environment::getInstance().renderSky();
	m_renderer->restore();
      }
      // Translate camera getDist() units away from the character. Allows closups or large views
      m_renderer->translateObject(0.0f, m_camera->getDistance(), -1.0f);
      m_renderer->applyCharacterLighting(0.5, 0, 0.5);
      m_renderer->applyQuaternion(m_orient);
      
      float height = (focus->hasBBox()) ? (focus->getBBox().highCorner().z() - focus->getBBox().lowCorner().z()) : (1.0f);
      m_renderer->translateObject(-x, -y, -z - height); //Translate to accumulated position - Also adjust so origin is nearer head level
    
      // m_renderer->applyCharacterLighting(x, y, z + height);

      m_renderer->getFrustum(m_frustum);
    }

    m_renderer->applyLighting();

    if (!select_mode ) {
      m_renderer->store();
      RenderSystem::getInstance().switchState(RenderSystem::getInstance().requestState("terrain"));
      glEnableClientState(GL_VERTEX_ARRAY);
      Environment::getInstance().renderTerrain(pos);
      glDisableClientState(GL_VERTEX_ARRAY);
      m_renderer->restore();
    }
#if(1)
    WorldEntity *root = NULL; 
    Eris::View *view = avatar->getView();
    if ((root = dynamic_cast<WorldEntity *>(view->getTopLevel()))) {
      if (m_character) m_character->updateLocals(false);
      m_render_queue = Render::QueueMap();
      m_message_list = Render::MessageList();

      buildQueues(root, 0, select_mode, m_render_queue, m_message_list);

      m_renderer->drawQueue(m_render_queue, select_mode, time_elapsed);
      if (!select_mode) m_renderer->drawMessageQueue(m_message_list);
    }
#endif
      m_renderer->store();
  RenderSystem::getInstance().switchState(RenderSystem::getInstance().requestState("terrain"));
  glEnableClientState(GL_VERTEX_ARRAY);
  Environment::getInstance().renderSea();
  glDisableClientState(GL_VERTEX_ARRAY);
      m_renderer->restore();
    
    m_compass->update(m_camera->getRotation());
    m_compass->draw(m_renderer, select_mode);
  } else {
    m_renderer->drawSplashScreen();
  }
}


void Graphics::buildQueues(WorldEntity *we, int depth, bool select_mode, Render::QueueMap &render_queue, Render::MessageList &message_list) {
  we->checkActions(); // See if model animations need to be updated
  if (depth == 0 || we->isVisible()) {
    if (we->getType() != NULL) {
      // TODO there must be a better way of doing this - after the first time round for each object, it should only require one call
      ObjectHandler *object_handler = m_system->getObjectHandler();
      ObjectRecord *object_record = object_handler->getObjectRecord(we->getId());
      if (object_record && object_record->type.empty()) object_record->type = we->getId();
      if (!object_record) {
        std::string type = we->type();
        varconf::Config::inst()->clean(type);
        object_record = object_handler->getObjectRecord(type);
	object_handler->copyObjectRecord(we->getId(), object_record);
        object_record = object_handler->getObjectRecord(we->getId());
        if (object_record) object_record->type = we->type();
      }
      if (!object_record) {
        std::string parent = we->parent();
        varconf::Config::inst()->clean(parent);
        object_record = object_handler->getObjectRecord(parent);
	object_handler->copyObjectRecord(we->getId(), object_record);
        object_record = object_handler->getObjectRecord(we->getId());
        if (object_record) object_record->type = we->parent();
      }
      if (!object_record) {
        object_record = object_handler->getObjectRecord(DEFAULT);
	object_handler->copyObjectRecord(we->getId(), object_record);
        object_record = object_handler->getObjectRecord(we->getId());
        if (object_record) object_record->type = DEFAULT;
      }
      if (!object_record) {
        std::cout << "No Record found" << std::endl;	      
        return;   
      }
      object_record->name = we->getName();
      object_record->id = we->getId();
      object_record->entity = we;

      if (we->hasBBox()) {
        object_record->bbox = we->getBBox();
      } else {
//        WFMath::Point<3> lc = WFMath::Point<3>(0.0f, 0.0f, 0.0f);
//        WFMath::Point<3> hc = WFMath::Point<3>(1.0f, 1.0f, 1.0f);
//        object_record->bbox = WFMath::AxisBox<3>(lc, hc);
      }

      // Hmm, might be better to explicity link to object.
      // calls only required if Pos, or orientation changes - how can we tell?
      object_record->position = we->getAbsPos();
      object_record->orient = we->getAbsOrient();
      // TODO determine what model queue to use.
      // TODO if queue is empty switch to another
//      if (object_record->low_quality.begin() == object_record->low_quality.end()) std::cout << "Error, no models!" << std::endl;

      // Setup lights as we go
      if (we->type() == "fire") {
        // Turn on light source
        m_fire.enabled = true;

        // Set position to entity posotion
        m_fire.position = we->getAbsPos();
        m_fire.position.z() += 0.5f; // Raise position off the ground a bit

        // Add light to gl system
        m_lm->applyLight(m_fire);
        
        // Disable as we don't need it again for now 
        m_fire.enabled = false;
      }


      // Loop through all models in list
      if (object_record->draw_self) {
        // Check if we're visible
        // Change method here
        if (Frustum::sphereInFrustum(m_frustum, object_record->bbox, object_record->position)) {
        //if (Frustum::orientBBoxInFrustum(m_frustum, we->getOrientBBox(), object_record->position)) {
          for (ObjectRecord::ModelList::const_iterator I = object_record->low_quality.begin(); I != object_record->low_quality.end(); ++I) {
            if (!select_mode) {
               // Add to queue by state, then model record
               render_queue[m_system->getModelRecords().getItem(*I, "state_num")].push_back(Render::QueueItem(object_record, *I));
              if (we->hasMessages()) message_list.push_back(we);
            }
            else render_queue[m_system->getModelRecords().getItem(*I, "select_state_num")].push_back(Render::QueueItem(object_record, *I));
          }
        }
      }
      if (object_record->draw_members) {
        for (unsigned int i = 0; i < we->numContained(); ++i) {
          buildQueues((WorldEntity*)we->getContained(i), depth + 1, select_mode, render_queue, message_list);
        }
      }
    }
  }
}

void Graphics::readConfig() {
  varconf::Variable temp;
  varconf::Config &general = m_system->getGeneral();
 
  // Read Fire properties 
  if (general.findItem(GRAPHICS, KEY_fire_ac)) {
    temp = general.getItem(GRAPHICS, KEY_fire_ac);
    m_fire.attenuation_constant = (!temp.is_double()) ? (DEFAULT_fire_ac) : ((double)(temp));
  } else {
    m_fire.attenuation_constant = DEFAULT_fire_ac;
  }
  if (general.findItem(GRAPHICS, KEY_fire_al)) {
    temp = general.getItem(GRAPHICS, KEY_fire_al);
    m_fire.attenuation_linear = (!temp.is_double()) ? (DEFAULT_fire_al) : ((double)(temp));
  } else {
    m_fire.attenuation_linear = DEFAULT_fire_al;
  }
  if (general.findItem(GRAPHICS, KEY_fire_aq)) {
    temp = general.getItem(GRAPHICS, KEY_fire_aq);
    m_fire.attenuation_quadratic = (!temp.is_double()) ? (DEFAULT_fire_aq) : ((double)(temp));
  } else {
    m_fire.attenuation_quadratic = DEFAULT_fire_aq;
  }

  if (general.findItem(GRAPHICS, KEY_fire_amb_red)) {
    temp = general.getItem(GRAPHICS, KEY_fire_amb_red);
    m_fire.ambient[0] = (!temp.is_double()) ? (DEFAULT_fire_amb_red) : ((double)(temp));
  } else {
    m_fire.ambient[0] = DEFAULT_fire_amb_red;
  }
  if (general.findItem(GRAPHICS, KEY_fire_amb_green)) {
    temp = general.getItem(GRAPHICS, KEY_fire_amb_green);
    m_fire.ambient[1] = (!temp.is_double()) ? (DEFAULT_fire_amb_green) : ((double)(temp));
  } else {
    m_fire.ambient[1] = DEFAULT_fire_amb_green;
  }
  if (general.findItem(GRAPHICS, KEY_fire_amb_blue)) {
    temp = general.getItem(GRAPHICS, KEY_fire_amb_blue);
    m_fire.ambient[2] = (!temp.is_double()) ? (DEFAULT_fire_amb_blue) : ((double)(temp));
  } else {
    m_fire.ambient[3] = DEFAULT_fire_amb_blue;
  }
  if (general.findItem(GRAPHICS, KEY_fire_amb_alpha)) {
    temp = general.getItem(GRAPHICS, KEY_fire_amb_alpha);
    m_fire.ambient[3] = (!temp.is_double()) ? (DEFAULT_fire_amb_alpha) : ((double)(temp));
  } else {
    m_fire.ambient[4] = DEFAULT_fire_amb_alpha;
  }


  if (general.findItem(GRAPHICS, KEY_fire_diff_red)) {
    temp = general.getItem(GRAPHICS, KEY_fire_diff_red);
    m_fire.diffuse[0] = (!temp.is_double()) ? (DEFAULT_fire_diff_red) : ((double)(temp));
  } else {
    m_fire.diffuse[0] = DEFAULT_fire_diff_red;
  }
  if (general.findItem(GRAPHICS, KEY_fire_diff_green)) {
    temp = general.getItem(GRAPHICS, KEY_fire_diff_green);
    m_fire.diffuse[1] = (!temp.is_double()) ? (DEFAULT_fire_diff_green) : ((double)(temp));
  } else {
    m_fire.diffuse[1] = DEFAULT_fire_diff_green;
  }
  if (general.findItem(GRAPHICS, KEY_fire_diff_blue)) {
    temp = general.getItem(GRAPHICS, KEY_fire_diff_blue);
    m_fire.diffuse[2] = (!temp.is_double()) ? (DEFAULT_fire_diff_blue) : ((double)(temp));
  } else {
    m_fire.diffuse[2] =  DEFAULT_fire_diff_blue;
  }
  if (general.findItem(GRAPHICS, KEY_fire_diff_alpha)) {
    temp = general.getItem(GRAPHICS, KEY_fire_diff_alpha);
    m_fire.diffuse[3] = (!temp.is_double()) ? (DEFAULT_fire_diff_alpha) : ((double)(temp));
  } else {
    m_fire.diffuse[3] = DEFAULT_fire_diff_alpha;
  }



  if (general.findItem(GRAPHICS, KEY_fire_spec_red)) {
    temp = general.getItem(GRAPHICS, KEY_fire_spec_red);
    m_fire.specular[0] = (!temp.is_double()) ? (DEFAULT_fire_spec_red) : ((double)(temp));
  } else {
    m_fire.specular[0] = DEFAULT_fire_spec_red;
  }
  if (general.findItem(GRAPHICS, KEY_fire_spec_green)) {
    temp = general.getItem(GRAPHICS, KEY_fire_spec_green);
    m_fire.specular[1] = (!temp.is_double()) ? (DEFAULT_fire_spec_green) : ((double)(temp));
  } else {
    m_fire.specular[1] = DEFAULT_fire_spec_green;
  }
  if (general.findItem(GRAPHICS, KEY_fire_spec_blue)) {
    temp = general.getItem(GRAPHICS, KEY_fire_spec_blue);
    m_fire.specular[2] = (!temp.is_double()) ? (DEFAULT_fire_spec_blue) : ((double)(temp));
  } else {
    m_fire.specular[2] = DEFAULT_fire_spec_blue;
  }
  if (general.findItem(GRAPHICS, KEY_fire_spec_alpha)) {
    temp = general.getItem(GRAPHICS, KEY_fire_spec_alpha);
    m_fire.specular[3] = (!temp.is_double()) ? (DEFAULT_fire_spec_alpha) : ((double)(temp));
  } else {
    m_fire.specular[3] = DEFAULT_fire_spec_alpha;
  }

}  

void Graphics::writeConfig() {
  varconf::Config &general = m_system->getGeneral();
  
  // Save frame rate detail boundaries
  general.setItem(GRAPHICS, KEY_fire_ac, m_fire.attenuation_constant);
  general.setItem(GRAPHICS, KEY_fire_al, m_fire.attenuation_linear);
  general.setItem(GRAPHICS, KEY_fire_aq, m_fire.attenuation_quadratic);
  
  general.setItem(GRAPHICS, KEY_fire_amb_red, m_fire.ambient[Light::RED]);
  general.setItem(GRAPHICS, KEY_fire_amb_green, m_fire.ambient[Light::GREEN]);
  general.setItem(GRAPHICS, KEY_fire_amb_blue, m_fire.ambient[Light::BLUE]);
  general.setItem(GRAPHICS, KEY_fire_amb_alpha, m_fire.ambient[Light::ALPHA]);

  general.setItem(GRAPHICS, KEY_fire_diff_red, m_fire.diffuse[Light::RED]);
  general.setItem(GRAPHICS, KEY_fire_diff_green, m_fire.diffuse[Light::GREEN]);
  general.setItem(GRAPHICS, KEY_fire_diff_blue, m_fire.diffuse[Light::BLUE]);
  general.setItem(GRAPHICS, KEY_fire_diff_alpha, m_fire.diffuse[Light::ALPHA]);

  general.setItem(GRAPHICS, KEY_fire_spec_red, m_fire.specular[Light::RED]);
  general.setItem(GRAPHICS, KEY_fire_spec_green, m_fire.specular[Light::GREEN]);
  general.setItem(GRAPHICS, KEY_fire_spec_blue, m_fire.specular[Light::BLUE]);
  general.setItem(GRAPHICS, KEY_fire_spec_alpha, m_fire.specular[Light::ALPHA]);

}  

void Graphics::readComponentConfig() {
  if (m_renderer) m_renderer->readConfig();
  if (m_camera) m_camera->readConfig();
  if (m_camera) m_camera->writeConfig();
}

void Graphics::writeComponentConfig() {
  if (m_renderer) m_renderer->writeConfig();
}

void Graphics::registerCommands(Console * console) {
  console->registerCommand("invalidate", this);
}

void Graphics::runCommand(const std::string &command, const std::string &args) {
  if (command == "invalidate") {
    RenderSystem::getInstance().invalidate();
    Environment::getInstance().invalidate();
  }
}

void Graphics::varconf_callback(const std::string &section, const std::string &key, varconf::Config &config) {
  varconf::Variable temp;
}

} /* namespace Sear */
