// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2002 Simon Goodall, University of Southampton

/*TODO
 * Allow texture unloading
 * Allow priority textures
 *
 *
 */ 


#include <unistd.h>
#include <GL/glu.h>

#include <varconf/Config.h>
#include <wfmath/quaternion.h>
#include <wfmath/vector.h>
#include <Eris/Entity.h>
#include <Eris/World.h>

#include "common/Log.h"
#include "common/Utility.h"

#include "src/Camera.h"
#include "src/Console.h"
#include "src/Exception.h"
#include "src/Frustum.h"
#include "src/Graphics.h"
#include "src/Model.h"
#include "src/ModelHandler.h"
#include "src/ObjectLoader.h"
#include "src/Sky.h"
#include "src/System.h"
#include "src/Terrain.h"
#include "src/WorldEntity.h"

#include "terrain/ROAM.h"
#include "sky/SkyBox.h"

#include "src/default_image.xpm"
#include "src/default_font.xpm"

#include "GL.h"

namespace Sear {

static float _halo_colour[4] = {1.0f, 0.0f, 1.0f, 1.0f};

inline GLuint GL::makeMask(GLint bits) {
  return (0xFF >> (8 - bits));
}


inline std::string GL::getSelectedID(unsigned int i) {
  return colour_mapped[i];
}

void GL::nextColour(const std::string &id) {
  unsigned int ic;
  
  if  (colourSetIterator != colourSet.end()) ic = *colourSetIterator++;
  else Log::writeLog("Out of colours, please increase number available", Log::LOG_ERROR);

  colour_mapped[ic] = id;
  
  GLubyte red = (ic & (redMask << redShift)) << (8 - redBits);
  GLubyte green = (ic & (greenMask << greenShift)) << (8 - greenBits);
  GLubyte blue = (ic & (blueMask << blueShift)) << (8 - blueBits);
  
  glColor3ub(red, green, blue);
}

inline void GL::resetColours(){
  colour_mapped = std::map<unsigned int, std::string>();
  colourSetIterator = colourSet.begin();
  *colourSetIterator++;
}

static GLfloat activeNameColour[] = { 1.0f, 0.75f, 0.2f, 1.0f};

static GLfloat white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static GLfloat red[] =   { 1.0f, 0.0f, 0.0f, 1.0f };
//static GLfloat green[] = { 0.0f, 1.0f, 0.0f, 1.0f };
//static GLfloat blue[] =  { 0.0f, 0.0f, 1.0f, 1.0f };
static GLfloat yellow[] =  { 0.0f, 1.0f, 1.0f, 1.0f };
//static GLfloat whiteLight[]    = { 1.0f,  1.0f, 1.0f, 1.0f };
static GLfloat blackLight[]    = { 0.0f,  0.0f, 0.0f, 1.0f };
//static GLfloat ambientLight[]  = { 0.75f, 0.75f, 0.75f, 1.0f };
//static GLfloat diffuseLight[]  = { 1.0f,  1.0f, 1.0f, 1.0f };
//static GLfloat specularLight[]  = { 1.0f,  1.0f, 1.0f, 1.0f };

void GL::buildColourSet() {
  unsigned int numPrims = 500;
  glGetIntegerv (GL_RED_BITS, &redBits);
  glGetIntegerv (GL_GREEN_BITS, &greenBits);
  glGetIntegerv (GL_BLUE_BITS, &blueBits);

  redMask = makeMask(redBits);
  greenMask = makeMask(greenBits);
  blueMask = makeMask(blueBits);
  redShift =   greenBits + blueBits;
  greenShift =  blueBits;
  blueShift =  0;
  unsigned long indx;
  colourSet = std::set<int>();
  
  for (indx = 0; indx < numPrims; indx++) {
    int ic = 0;
    ic += indx & (redMask << redShift);
    ic += indx & (greenMask << greenShift);
    ic += indx & (blueMask << blueShift);
    colourSet.insert(ic);
  }
  Log::writeLog(std::string("Number of colours: ") + string_fmt(colourSet.size()), Log::LOG_INFO);
}


GL *GL::_instance = NULL;

GL::GL() :
  _system(NULL),
  window_width(0),
  window_height(0),
  fov(RENDER_FOV),
  near_clip(RENDER_NEAR_CLIP),
  next_id(1),
  base(0),
  textureList(std::list<GLuint>()),
  terrain(NULL),
  _cur_state(NULL)
{
  _instance = this;
}

GL::GL(System *system, Graphics *graphics) :
  _system(system),
  _graphics(graphics),
  window_width(0),
  window_height(0),
  fov(RENDER_FOV),
  near_clip(RENDER_NEAR_CLIP),
  next_id(1),
  base(0),
  textureList(std::list<GLuint>()),
  terrain(NULL),
  _cur_state(NULL)
{
  _instance = this;
}

GL::~GL() {
  writeConfig();
  shutdownFont();
}

void GL::initWindow(int width, int height) {
  Log::writeLog("Render: Initilising Renderer", Log::LOG_DEFAULT);
  // TODO: put this into an info method 
  
  std::string vendor = string_fmt(glGetString(GL_VENDOR));
  std::string renderer = string_fmt(glGetString(GL_RENDERER));
  std::string version = string_fmt(glGetString(GL_VERSION));
  std::string extensions = string_fmt(glGetString(GL_EXTENSIONS));
  
  Log::writeLog(std::string("GL_VENDER: ") + vendor, Log::LOG_DEFAULT);
  Log::writeLog(std::string("GL_RENDERER: ") + renderer, Log::LOG_DEFAULT);
  Log::writeLog(std::string("GL_VERSION: ") + version, Log::LOG_DEFAULT);
  Log::writeLog(std::string("GL_EXTENSIONS: ") + extensions, Log::LOG_DEFAULT);
 
  if (vendor.empty() || renderer.empty()) {
    throw Exception("Error with OpenGL system");
  }
  
  glLineWidth(4.0f);
  //TODO: this needs to go into the set viewport method
  //Check for divide by 0
  if (height == 0) height = 1;
//  glLineWidth(2.0f); 
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Colour used to clear window
  glClearDepth(1.0); // Enables Clearing Of The Depth Buffer
  glClear(GL_DEPTH_BUFFER_BIT);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
  glDisable(GL_DITHER); 
  
  //Store window size
  window_width = width;
  window_height = height;

  setViewMode(PERSPECTIVE);
}
  
void GL::init() {
  // Most of this should be elsewhere
  readConfig();
  createDefaults();
  splash_id = requestTexture("ui", splash_texture);
  initFont();
  initLighting();
  // TODO: initialisation need to go into system
  setupStates();
#ifdef DEBUG  
  CheckError();
#endif

}

void GL::initLighting() {
  Log::writeLog("Render: initialising lighting", Log::LOG_DEFAULT);
  float gambient[4] = {0.1f, 0.1f,0.1f, 1.0f};
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT,gambient);
  // Light values and coordinates
           
  // Setup and enable light 0
  glLightfv(GL_LIGHT0, GL_AMBIENT, lights[LIGHT_CHARACTER].ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lights[LIGHT_CHARACTER].diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, lights[LIGHT_CHARACTER].specular);
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, lights[LIGHT_CHARACTER].kc);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, lights[LIGHT_CHARACTER].kl);
  glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, lights[LIGHT_CHARACTER].kq);
  glEnable(GL_LIGHT0);
  
  glLightfv(GL_LIGHT1, GL_AMBIENT, blackLight);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, blackLight);
  glLightfv(GL_LIGHT1, GL_SPECULAR, blackLight);
  
  glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, lights[LIGHT_SUN].kc);
  glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, lights[LIGHT_SUN].kl);
  glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, lights[LIGHT_SUN].kq);
  glEnable(GL_LIGHT1);
}

void GL::initFont() {
  int loop;
  float cx; // Holds Our X Character Coord
  float cy; // Holds Our Y Character Coord
  Log::writeLog("Render: Initilising Fonts", Log::LOG_DEFAULT);
  base=glGenLists(256); // Creating 256 Display Lists
  font_id = requestTexture("ui", "font");
  GLuint texture = getTextureID(font_id);
  if (!glIsTexture(texture) || font_id == -1) {
    font_id = requestTexture("default", "default_font");
    texture = getTextureID(font_id);
  }
  glBindTexture(GL_TEXTURE_2D, texture);
  for (loop=0; loop<256; loop++) {
    cx=(float)(loop%16)/16.0f; // X Position Of Current Character
    cy=(float)(loop/16)/16.0f; // Y Position Of Current Character
    glNewList(base+loop,GL_COMPILE); // Start Building A List
    glBegin(GL_QUADS); // Use A Quad For Each Character
      glTexCoord2f(cx,1-cy-0.0625f); // Texture Coord (Bottom Left)
      glVertex2i(0,0); // Vertex Coord (Bottom Left)
      glTexCoord2f(cx,1-cy); // Texture Coord (Top Left)
      glVertex2i(0,16); // Vertex Coord (Top Left)
      glTexCoord2f(cx+0.0625f,1-cy); // Texture Coord (Top Right)
      glVertex2i(16,16); // Vertex Coord (Top Right)
      glTexCoord2f(cx+0.0625f,1-cy-0.0625f); // Texture Coord (Bottom Right)
      glVertex2i(16,0);       // Vertex Coord (Bottom Right)
    glEnd(); // Done Building Our Quad (Character)
    glTranslated(10,0,0); // Move To The Right Of The Character
    glEndList(); // Done Building The Display List
  }// Loop Until All 256 Are Built
}

void GL::shutdownFont() {
  Log::writeLog("Render: Shutting down fonts", Log::LOG_DEFAULT);
  glDeleteLists(base,256); // Delete All 256 Display Lists
}

void GL::print(GLint x, GLint y, const char * string, int set) {
  if (set > 1) set = 1;
  GLuint texture = getTextureID(font_id);
//  if (!glIsTexture(texture)) {
//    static GLuint default_id = getTextureID(requestTexture("default_font"));
//    texture = default_id;
//  }
  glBindTexture(GL_TEXTURE_2D, texture);
  glMatrixMode(GL_PROJECTION); // Select The Projection Matrix
  store();
  glLoadIdentity(); // Reset The Projection Matrix
  glOrtho(0, window_width, 0 , window_height, -1, 1); // Set Up An Ortho Screen
  glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
  store();
  glLoadIdentity(); // Reset The Modelview Matrix
  glTranslated(x,y,0); // Position The Text (0,0 - Bottom Left)
  glListBase(base-32+(128*set)); // Choose The Font Set (0 or 1)
  glCallLists(strlen(string),GL_BYTE,string); // Write The Text To The Screen
  glMatrixMode(GL_PROJECTION); // Select The Projection Matrix
  glPopMatrix(); // Restore The Old Projection Matrix
  glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
  glPopMatrix(); // Restore The Old Projection Matrix
}

void GL::print3D(const char *string, int set) {
  if (set > 1) set = 1;
  GLuint texture = getTextureID(font_id);
  if (!glIsTexture(texture)) {
    static GLuint default_id = getTextureID(requestTexture("default", "default_font"));
    texture = default_id;
  }
  glBindTexture(GL_TEXTURE_2D, texture);
  store();
  glListBase(base-32+(128*set)); // Choose The Font Set (0 or 1)
  glCallLists(strlen(string),GL_BYTE,string); // Write The Text To The Screen
  glPopMatrix(); // Restore The Old Projection Matrix
}

inline void GL::newLine() {
  glTranslatef(0.0f,  ( FONT_HEIGHT) , 0.0f);
}

int GL::requestTexture(const std::string &section, const std::string &texture, bool clamp) {
  static varconf::Config *texture_config = _system->getTexture();
  std::string texture_name = std::string(texture);
  texture_config->clean(texture_name);
  SDL_Surface *tmp = NULL;
  unsigned int texture_id = 0;
  int id = texture_map[texture_name];
  if (id != 0) return id;
  glGenTextures(1, &texture_id);
  if (texture_id == 0) return -1;
  std::string file_name = texture_config->getItem(section, texture_name);
  if (file_name.empty()) return -1;
  tmp = System::loadImage(file_name);
  if (!tmp) {
    Log::writeLog("Error loading texture", Log::LOG_ERROR);
    return -1;
  }
  createTexture(tmp, texture_id, clamp);
  free (tmp);
  textureList.push_back(texture_id);
  texture_map[texture_name] = next_id;
  return next_id++;
}

int GL::requestMipMap(const std::string &section, const std::string &texture, bool clamp) {
  static varconf::Config *texture_config = _system->getTexture();
  std::string texture_name = std::string(texture);
  texture_config->clean(texture_name);
  SDL_Surface *tmp = NULL;
  unsigned int texture_id = 0;
  int id = texture_map[texture_name];
  if (id != 0) return id;
  glGenTextures(1, &texture_id);
  if (texture_id == 0) return -1;
  std::string file_name = texture_config->getItem(section, texture_name);
  if (file_name.empty()) return -1;
  tmp = System::loadImage(file_name);
  if (!tmp) return -1;
  createMipMap(tmp, texture_id, clamp);
  free (tmp);
  textureList.push_back(texture_id);
  texture_map[texture_name] = next_id;
  return next_id++;
}

int GL::requestTextureMask(const std::string &section, const std::string &texture, bool clamp) {
  static varconf::Config *texture_config = _system->getTexture();
  std::string texture_name = std::string(texture);
  texture_config->clean(texture_name);
  SDL_Surface *tmp = NULL;
  unsigned int texture_id = 0;
  int id = texture_map[texture_name + "_mask"];
  if (id != 0) return id;
  glGenTextures(1, &texture_id);
  if (texture_id == 0) return -1;
  std::string file_name = texture_config->getItem(section, texture_name);
  if (file_name.empty()) return -1;
  tmp = System::loadImage(file_name);
  if (!tmp) {
    Log::writeLog("Error loading texture", Log::LOG_ERROR);
    return -1;
  }
  createTextureMask(tmp, texture_id, clamp);
  free (tmp);
  textureList.push_back(texture_id);
  texture_map[texture_name + "_mask"] = next_id;
  return next_id++;
}

void GL::createTexture(SDL_Surface *surface, unsigned int texture, bool clamp) {
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  if (clamp) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }
  glTexImage2D(GL_TEXTURE_2D, 0, (surface->format->BytesPerPixel == 3) ? GL_RGB : GL_RGBA, surface->w, surface->h, 0, (surface->format->BytesPerPixel == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
}
 
void GL::createMipMap(SDL_Surface *surface, unsigned int texture, bool clamp)  {
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
  if (clamp) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);		  
  }
  gluBuild2DMipmaps(GL_TEXTURE_2D, 3, surface->w, surface->h, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);
}

void GL::createTextureMask(SDL_Surface *surface, unsigned int texture, bool clamp) {
  int i;
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  if (clamp) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }
  if (surface->format->BytesPerPixel == 4) {
    for (i = 0; i < surface->w * surface->h * 4; i += 4) {
      ((unsigned char *)surface->pixels)[i + 0] = (unsigned char)0xff;
      ((unsigned char *)surface->pixels)[i + 1] = (unsigned char)0xff;
      ((unsigned char *)surface->pixels)[i + 2] = (unsigned char)0xff;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, 4, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
  } else {
    for (i = 0; i < surface->w * surface->h * 3; i += 3) {
      ((unsigned char *)surface->pixels)[i + 0] = (unsigned char)0xff;
      ((unsigned char *)surface->pixels)[i + 1] = (unsigned char)0xff;
      ((unsigned char *)surface->pixels)[i + 2] = (unsigned char)0xff;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, 3, surface->w, surface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);
  }
}

inline GLuint GL::getTextureID(int texture_id) {
  int i;
  std::list<GLuint>::const_iterator I = textureList.begin();
  for (i = 1; i < texture_id; i++, I++);
  return *I;
}

void GL::stateChange(const std::string &state) {
  StateLoader *state_loader = _system->getStateLoader();
  if (state_loader) stateChange(state_loader->getStateProperties(state));
}

void GL::stateChange(StateProperties *sp) {
  if (!sp) {
    Log::writeLog("NULL State", Log::LOG_ERROR);
    return;
    // throw Exception("StateProperties is NULL");
  }
  if (_cur_state == sp) return;
  if (_cur_state) {
    std::string change = std::string(_cur_state->state) + std::string("_to_") + std::string(sp->state);
    GLuint list = _state_map[change];
    if (!glIsList(list)) {
      list = glGenLists(1);
      stateDisplayList(list, _cur_state, sp);
      _state_map[change] = list;
    }
    glCallList(list);
  } else { 
    if (sp->alpha_test) glEnable(GL_ALPHA_TEST);
    else glDisable(GL_ALPHA_TEST);
    if (sp->blend) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    if (sp->lighting && checkState(RENDER_LIGHTING)) glEnable(GL_LIGHTING);
    else glDisable(GL_LIGHTING);
    if (sp->two_sided_lighting && checkState(RENDER_LIGHTING)) glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    else glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    if (sp->textures && checkState(RENDER_TEXTURES)) glEnable(GL_TEXTURE_2D);
    else glDisable(GL_TEXTURE_2D);
    if (sp->colour_material) glEnable(GL_COLOR_MATERIAL);
    else glDisable(GL_COLOR_MATERIAL);
    if (sp->depth_test) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    if (sp->cull_face) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    if (sp->cull_face_cw) glFrontFace(GL_CW);
    else glFrontFace(GL_CCW);
    if (sp->stencil) glEnable(GL_STENCIL_TEST);
    else glDisable(GL_STENCIL_TEST);
    if (sp->fog) glEnable(GL_FOG);
    else glDisable(GL_FOG);
  }
  _cur_state = sp;
}


void GL::drawTextRect(GLint x, GLint y, GLint width, GLint height, int texture) {
  // TODO should use switchTexture
  glBindTexture(GL_TEXTURE_2D, getTextureID(texture));
  setViewMode(ORTHOGRAPHIC);
  // TODO: make into arrays?
  glBegin(GL_QUADS);
    glTexCoord2i(0, 0);
    glVertex2i(x, y);
    glTexCoord2i(0, 1);
    glVertex2i(x, y + height);
    glTexCoord2i(1, 1);
    glVertex2i(x + width, y + height);
    glTexCoord2i(1, 0);
    glVertex2i(x + width, y);
  glEnd();
  setViewMode(PERSPECTIVE);
}

void GL::procEvent(int x, int y) {
  unsigned int ic;
  std::string selected_id;
  GLubyte i[3];
  glClear(GL_COLOR_BUFFER_BIT);
  _graphics->drawScene("", true, 0);
  y = window_height - y;
  x_pos = x;
  y_pos = y;
  glReadPixels(x, y, 1, 1, GL_RGB , GL_UNSIGNED_BYTE, &i);

  GLubyte red = i[0] >> (8 - redBits);// & redMask;
  GLubyte green = i[1] >> (8 - greenBits);// & greenMask;
  GLubyte blue = i[2] >> (8 - blueBits);// & blueMask;

  ic = 0;
  ic += red;
  ic = ic << redBits;
  ic += green;
  ic = ic << greenBits;
  ic += blue;
  selected_id = getSelectedID(ic);
  if (selected_id != activeID) {
    activeID = selected_id;
    if (!activeID.empty()) Log::writeLog(std::string("ActiveID: ") + activeID, Log::LOG_DEFAULT);
  }
}

void GL::CheckError() {
  GLenum err = glGetError();
  std::string msg;
  switch (err) {
    case GL_NO_ERROR: break;
    case GL_INVALID_ENUM: msg = "GL Error: Invalid enum!"; break;
    case GL_INVALID_VALUE: msg = "GL Error: Invalid value!"; break;
    case GL_INVALID_OPERATION: msg = "GL Error: Invalid operation!"; break;
    case GL_STACK_OVERFLOW: msg = "GL Error: Stack overflow!"; break;
    case GL_STACK_UNDERFLOW: msg = "GL Error: Stack Underflow!"; break;
    case GL_OUT_OF_MEMORY: msg = "GL Error: Out of memory!"; break;
    default: msg = std::string("GL Error: Unknown Error: ") +  string_fmt(err); break; 
  }
  if (!msg.empty()) Log::writeLog(msg, Log::LOG_ERROR);
}

//TODO should be in general render class
void GL::readConfig() {
  varconf::Variable temp;
  varconf::Config *general = _system->getGeneral();
  Log::writeLog("Loading Renderer Config", Log::LOG_DEFAULT);
  if (!general) {
    Log::writeLog("GL: Error - General config object does not exist!", Log::LOG_ERROR);
    return;
  }

  // Setup character light source
  temp = general->getItem("render", KEY_character_light_kc);
  lights[LIGHT_CHARACTER].kc = (!temp.is_double()) ? (DEFAULT_character_light_kc) : ((double)(temp));
  temp = general->getItem("render", KEY_character_light_kl);
  lights[LIGHT_CHARACTER].kl = (!temp.is_double()) ? (DEFAULT_character_light_kl) : ((double)(temp));
  temp = general->getItem("render", KEY_character_light_kq);
  lights[LIGHT_CHARACTER].kq = (!temp.is_double()) ? (DEFAULT_character_light_kq) : ((double)(temp));
  
  temp = general->getItem("render", KEY_character_light_ambient_red);
  lights[LIGHT_CHARACTER].ambient[0] = (!temp.is_double()) ? (DEFAULT_character_light_ambient_red) : ((double)(temp));
  temp = general->getItem("render", KEY_character_light_ambient_green);
  lights[LIGHT_CHARACTER].ambient[1] = (!temp.is_double()) ? (DEFAULT_character_light_ambient_green) : ((double)(temp));
  temp = general->getItem("render", KEY_character_light_ambient_blue);
  lights[LIGHT_CHARACTER].ambient[2] = (!temp.is_double()) ? (DEFAULT_character_light_ambient_blue) : ((double)(temp));
  temp = general->getItem("render", KEY_character_light_ambient_alpha);
  lights[LIGHT_CHARACTER].ambient[3] = (!temp.is_double()) ? (DEFAULT_character_light_ambient_alpha) : ((double)(temp));

  temp = general->getItem("render", KEY_character_light_diffuse_red);
  lights[LIGHT_CHARACTER].diffuse[0] = (!temp.is_double()) ? (DEFAULT_character_light_diffuse_red) : ((double)(temp));
  temp = general->getItem("render", KEY_character_light_diffuse_green);
  lights[LIGHT_CHARACTER].diffuse[1] = (!temp.is_double()) ? (DEFAULT_character_light_diffuse_green) : ((double)(temp));
  temp = general->getItem("render", KEY_character_light_diffuse_blue);
  lights[LIGHT_CHARACTER].diffuse[2] = (!temp.is_double()) ? (DEFAULT_character_light_diffuse_blue) : ((double)(temp));
  temp = general->getItem("render", KEY_character_light_diffuse_alpha);
  lights[LIGHT_CHARACTER].diffuse[3] = (!temp.is_double()) ? (DEFAULT_character_light_diffuse_alpha) : ((double)(temp));

  temp = general->getItem("render", KEY_character_light_specular_red);
  lights[LIGHT_CHARACTER].specular[0] = (!temp.is_double()) ? (DEFAULT_character_light_specular_red) : ((double)(temp));
  temp = general->getItem("render", KEY_character_light_specular_green);
  lights[LIGHT_CHARACTER].specular[1] = (!temp.is_double()) ? (DEFAULT_character_light_specular_green) : ((double)(temp));
  temp = general->getItem("render", KEY_character_light_specular_blue);
  lights[LIGHT_CHARACTER].specular[2] = (!temp.is_double()) ? (DEFAULT_character_light_specular_blue) : ((double)(temp));
  temp = general->getItem("render", KEY_character_light_specular_alpha);
  lights[LIGHT_CHARACTER].specular[3] = (!temp.is_double()) ? (DEFAULT_character_light_specular_alpha) : ((double)(temp));
  //Setup Sun light source
  temp = general->getItem("render", KEY_sun_light_kc);
  lights[LIGHT_SUN].kc = (!temp.is_double()) ? (DEFAULT_sun_light_kc) : ((double)(temp));
  temp = general->getItem("render", KEY_sun_light_kl);
  lights[LIGHT_SUN].kl = (!temp.is_double()) ? (DEFAULT_sun_light_kl) : ((double)(temp));
  temp = general->getItem("render", KEY_sun_light_kq);
  lights[LIGHT_SUN].kq = (!temp.is_double()) ? (DEFAULT_sun_light_kq) : ((double)(temp));

  // Setup render states
  temp = general->getItem("render", KEY_use_textures);
  setState(RENDER_TEXTURES, ((!temp.is_bool()) ? (DEFAULT_use_textures) : ((bool)(temp))));
  temp = general->getItem("render", KEY_use_lighting);
  setState(RENDER_LIGHTING, ((!temp.is_bool()) ? (DEFAULT_use_lighting) : ((bool)(temp))));
  temp = general->getItem("render", KEY_show_fps);
  setState(RENDER_FPS, ((!temp.is_bool()) ? (DEFAULT_show_fps) : ((bool)(temp))));
  temp = general->getItem("render", KEY_use_stencil);
  setState(RENDER_STENCIL, ((!temp.is_bool()) ? (DEFAULT_use_stencil) : ((bool)(temp))));
  
  // Setup the speech offsets
  temp = general->getItem("render", KEY_speech_offset_x);
  _speech_offset_x = (!temp.is_double()) ? (DEFAULT_speech_offset_x) : ((double)(temp));
  temp = general->getItem("render", KEY_speech_offset_y);
  _speech_offset_y = (!temp.is_double()) ? (DEFAULT_speech_offset_y) : ((double)(temp));
  temp = general->getItem("render", KEY_speech_offset_z);
  _speech_offset_z = (!temp.is_double()) ? (DEFAULT_speech_offset_y) : ((double)(temp));

  temp = general->getItem("render", KEY_fog_start);
  _fog_start = (!temp.is_double()) ? (DEFAULT_fog_start) : ((double)(temp));
  temp = general->getItem("render", KEY_fog_end);
  _fog_end = (!temp.is_double()) ? (DEFAULT_fog_end) : ((double)(temp));

  temp = general->getItem("render", KEY_far_clip_dist);
  _far_clip_dist = (!temp.is_double()) ? (DEFAULT_far_clip_dist) : ((double)(temp));

}  

void GL::writeConfig() {
  varconf::Config *general = _system->getGeneral();
  if (!general) {
    Log::writeLog("GL: Error - General config object does not exist!", Log::LOG_ERROR);
    return;
  }
  
  // Save character light source
  general->setItem("render", KEY_character_light_kc, lights[LIGHT_CHARACTER].kc);
  general->setItem("render", KEY_character_light_kl, lights[LIGHT_CHARACTER].kl);
  general->setItem("render", KEY_character_light_kq, lights[LIGHT_CHARACTER].kq);

  general->setItem("render", KEY_character_light_ambient_red, lights[LIGHT_CHARACTER].ambient[0]);
  general->setItem("render", KEY_character_light_ambient_green, lights[LIGHT_CHARACTER].ambient[1]);
  general->setItem("render", KEY_character_light_ambient_blue, lights[LIGHT_CHARACTER].ambient[2]);
  general->setItem("render", KEY_character_light_ambient_alpha, lights[LIGHT_CHARACTER].ambient[3]);

  general->setItem("render", KEY_character_light_diffuse_red, lights[LIGHT_CHARACTER].diffuse[0]);
  general->setItem("render", KEY_character_light_diffuse_green, lights[LIGHT_CHARACTER].diffuse[1]);
  general->setItem("render", KEY_character_light_diffuse_blue, lights[LIGHT_CHARACTER].diffuse[2]);
  general->setItem("render", KEY_character_light_diffuse_alpha, lights[LIGHT_CHARACTER].diffuse[3]);

  general->setItem("render", KEY_character_light_specular_red, lights[LIGHT_CHARACTER].specular[0]);
  general->setItem("render", KEY_character_light_specular_green, lights[LIGHT_CHARACTER].specular[1]);
  general->setItem("render", KEY_character_light_specular_blue, lights[LIGHT_CHARACTER].specular[2]);
  general->setItem("render", KEY_character_light_specular_alpha, lights[LIGHT_CHARACTER].specular[3]);
  
  // Save Sun light source
  general->setItem("render", KEY_sun_light_kc, lights[LIGHT_SUN].kc);
  general->setItem("render", KEY_sun_light_kl, lights[LIGHT_SUN].kl);
  general->setItem("render", KEY_sun_light_kq, lights[LIGHT_SUN].kq);

  // Save render states
  general->setItem("render", KEY_use_textures, checkState(RENDER_TEXTURES));
  general->setItem("render", KEY_use_lighting, checkState(RENDER_LIGHTING));
  general->setItem("render", KEY_show_fps, checkState(RENDER_FPS));
  general->setItem("render", KEY_use_stencil, checkState(RENDER_STENCIL));
  
  // Save the speech offsets
  general->setItem("render", KEY_speech_offset_x, _speech_offset_x);
  general->setItem("render", KEY_speech_offset_y, _speech_offset_y);
  general->setItem("render", KEY_speech_offset_z, _speech_offset_z);

  general->setItem("render", KEY_fog_start, _fog_start);
  general->setItem("render", KEY_fog_end, _fog_end);
  general->setItem("render", KEY_far_clip_dist, _far_clip_dist);
}  


// THIS BUILDS A STATE TRANSITION LIST
void GL::stateDisplayList(GLuint &list, StateProperties *previous_state, StateProperties *next_state) {
  // TODO: if textures are disabled then disable them here too
  if (!previous_state || !next_state) return;
  glNewList(list, GL_COMPILE);
  if (previous_state->alpha_test != next_state->alpha_test) {
    if (next_state->alpha_test) glEnable(GL_ALPHA_TEST);
    else glDisable(GL_ALPHA_TEST);
  }
  if (previous_state->blend != next_state->blend) {
    if (next_state->blend) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
  }
  if (previous_state->lighting != next_state->lighting) {
    if (next_state->lighting && checkState(RENDER_LIGHTING)) glEnable(GL_LIGHTING);
    else glDisable(GL_LIGHTING);
  }
  if (previous_state->lighting != next_state->lighting) {
    if (next_state->lighting && checkState(RENDER_LIGHTING)) glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    else glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
  }
  if (previous_state->textures != next_state->textures) {
    if (next_state->textures && checkState(RENDER_TEXTURES)) glEnable(GL_TEXTURE_2D);
    else glDisable(GL_TEXTURE_2D);
  }
  if (previous_state->colour_material != next_state->colour_material) {
    if (next_state->colour_material) glEnable(GL_COLOR_MATERIAL);
    else glDisable(GL_COLOR_MATERIAL);
  }
  if (previous_state->depth_test != next_state->depth_test) {
    if (next_state->depth_test) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
  }
  if (previous_state->cull_face != next_state->cull_face) {
    if (next_state->cull_face) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    if (next_state->cull_face_cw) glFrontFace(GL_CW);
    else glFrontFace(GL_CCW);
  }
  if (previous_state->stencil != next_state->stencil) {
    if (next_state->stencil) glEnable(GL_STENCIL_TEST);
    else glDisable(GL_STENCIL_TEST);
  }
  if (previous_state->fog != next_state->fog) {
    if (next_state->fog) glEnable(GL_FOG);
    else glDisable(GL_FOG);
  }
  glEndList();
}


void GL::setupStates() {
  // TODO: should this be in the init?
  glAlphaFunc(GL_GREATER, 0.1f);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glFogi(GL_FOG_MODE, GL_LINEAR);
  GLfloat fog_colour[] = {0.50f, 0.50f, 0.50f, 0.50f};
  glFogfv(GL_FOG_COLOR, fog_colour);
  glFogf(GL_FOG_START, _fog_start);
  glFogf(GL_FOG_END, _fog_end);
}

inline void GL::translateObject(float x, float y, float z) {
  glTranslatef(x, y, z);
}

void GL::rotateObject(WorldEntity *we, int type) {
  if (!we) return; // THROW ERROR;
  switch (type) {
    case Graphics::ROS_NONE: return; break;
    case Graphics::ROS_POSITION: {
       WFMath::Point<3> pos = we->getPosition();
       glRotatef(pos.x() + pos.y() + pos.z(), 0.0f, 0.0f, 1.0f);
       break;
    }       
    case Graphics::ROS_NORMAL: {
      applyQuaternion(we->getAbsOrient());
      break;
    }
    case Graphics::ROS_BILLBOARD: // Same as HALO, but does not rotate with camera elevation
    case Graphics::ROS_HALO: {
      float rotation_matrix[4][4];
      WFMath::Quaternion  orient2 = WFMath::Quaternion(1.0f, 0.0f, 0.0f, 0.0f); // Initial Camera rotation
      orient2 /= _graphics->getCameraOrientation();
      QuatToMatrix(orient2, rotation_matrix); //Get the rotation matrix for base rotation
      glMultMatrixf(&rotation_matrix[0][0]); //Apply rotation matrix
      break;
    }
  }
}

void GL::scaleObject(float scale) {
  glScalef(scale, scale, scale);
}

void GL::setViewMode(int type) {
//  Perspective
  glViewport(0, 0, window_width, window_height);
  switch (type) {
    case PERSPECTIVE: {
      if (window_height == 0) window_height = 1;
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity(); // Reset The Projection Matrix
  
      // Calculate The Aspect Ratio Of The Window
      gluPerspective(fov,(GLfloat)window_width/(GLfloat)window_height, near_clip, _far_clip_dist);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      break;
    }
    case ORTHOGRAPHIC: {
      glMatrixMode(GL_PROJECTION); // Select The Projection Matrix
      glLoadIdentity(); // Reset The Projection Matrix
      glOrtho(0, window_width, 0 , window_height, -1, 1); // Set Up An Ortho Screen
      glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
      glLoadIdentity();
      break;
    }			    
    case ISOMETRIC: {
      break;
    }			    
  }	
}

void GL::setMaterial(float *ambient, float *diffuse, float *specular, float shininess, float *emissive) {
  // TODO: set up missing values
  if (ambient)           glMaterialfv (GL_FRONT, GL_AMBIENT,   ambient);
  if (diffuse)           glMaterialfv (GL_FRONT, GL_DIFFUSE,   diffuse);
  if (specular)          glMaterialfv (GL_FRONT, GL_SPECULAR,  specular);
  if (shininess >= 0.0f) glMaterialf  (GL_FRONT, GL_SHININESS, shininess);
  if (emissive)          glMaterialfv (GL_FRONT, GL_EMISSION,  emissive);
  else                   glMaterialfv (GL_FRONT, GL_EMISSION,  black);
}

void GL::renderArrays(unsigned int type, unsigned int offset, unsigned int number_of_points, float *vertex_data, float *texture_data, float *normal_data) {
  // TODO: Reduce ClientState switches
  bool textures = checkState(RENDER_TEXTURES);
  bool lighting = checkState(RENDER_LIGHTING);
 
  if (!vertex_data) {
    Log::writeLog("No Vertex Data", Log::LOG_ERROR);
    return; //throw Exception(""); 
  }
  glVertexPointer(3, GL_FLOAT, 0, vertex_data);
  glEnableClientState(GL_VERTEX_ARRAY);
  if (textures && texture_data) {
    glTexCoordPointer(2, GL_FLOAT, 0, texture_data);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  }
  if (lighting && normal_data) {
    glNormalPointer(GL_FLOAT, 0, normal_data);
    glEnableClientState(GL_NORMAL_ARRAY);
  }

  switch (type) {
    case (Graphics::RES_INVALID): Log::writeLog("Trying to render INVALID type", Log::LOG_ERROR); break;
    case (Graphics::RES_POINT): glDrawArrays(GL_POINT, offset, number_of_points); break;
    case (Graphics::RES_LINES): glDrawArrays(GL_LINES, offset, number_of_points); break;
    case (Graphics::RES_TRIANGLES): glDrawArrays(GL_TRIANGLES, offset, number_of_points); break;
    case (Graphics::RES_QUADS): glDrawArrays(GL_QUADS, offset, number_of_points); break;
    case (Graphics::RES_TRIANGLE_FAN): glDrawArrays(GL_TRIANGLE_FAN, offset, number_of_points); break;
    case (Graphics::RES_TRIANGLE_STRIP): glDrawArrays(GL_TRIANGLE_STRIP, offset, number_of_points); break;
    case (Graphics::RES_QUAD_STRIP): glDrawArrays(GL_QUAD_STRIP, offset, number_of_points); break;
    default: Log::writeLog("Unknown type", Log::LOG_ERROR); break;
  }
 
  glDisableClientState(GL_VERTEX_ARRAY);
  if (lighting && normal_data) glDisableClientState(GL_NORMAL_ARRAY);
  if (textures && texture_data) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void GL::renderElements(unsigned int type, unsigned int number_of_points, int *faces_data, float *vertex_data, float *texture_data, float *normal_data) {
  // TODO: Reduce ClientState switches
  bool textures = checkState(RENDER_TEXTURES);
  bool lighting = checkState(RENDER_LIGHTING);
 
  if (!vertex_data) return; //throw Exception(""); 
  glVertexPointer(3, GL_FLOAT, 0, vertex_data);
  glEnableClientState(GL_VERTEX_ARRAY);
  if (textures && texture_data) {
    glTexCoordPointer(2, GL_FLOAT, 0, texture_data);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  }
  if (lighting && normal_data) {
    glNormalPointer(GL_FLOAT, 0, normal_data);
    glEnableClientState(GL_NORMAL_ARRAY);
  }

  switch (type) {
    case (Graphics::RES_INVALID): Log::writeLog("Trying to render INVALID type", Log::LOG_ERROR); break;
    case (Graphics::RES_POINT): glDrawElements(GL_POINT, number_of_points, GL_UNSIGNED_INT, faces_data); break;
    case (Graphics::RES_LINES): glDrawElements(GL_LINES, number_of_points, GL_UNSIGNED_INT, faces_data); break;
    case (Graphics::RES_TRIANGLES): glDrawElements(GL_TRIANGLES, number_of_points, GL_UNSIGNED_INT, faces_data); break;
    case (Graphics::RES_QUADS): glDrawElements(GL_QUADS, number_of_points, GL_UNSIGNED_INT, faces_data); break;
    case (Graphics::RES_TRIANGLE_FAN): glDrawElements(GL_TRIANGLE_FAN, number_of_points, GL_UNSIGNED_INT, faces_data); break;
    case (Graphics::RES_TRIANGLE_STRIP): glDrawElements(GL_TRIANGLE_STRIP, number_of_points, GL_UNSIGNED_INT, faces_data); break;
    case (Graphics::RES_QUAD_STRIP): glDrawElements(GL_QUAD_STRIP, number_of_points, GL_UNSIGNED_INT, faces_data); break;
    default: Log::writeLog("Unknown type", Log::LOG_ERROR); break;
  }
 
  glDisableClientState(GL_VERTEX_ARRAY);
  if (lighting && normal_data) glDisableClientState(GL_NORMAL_ARRAY);
  if (textures && texture_data) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

unsigned int GL::createTexture(unsigned int width, unsigned int height, unsigned int depth, unsigned char *data, bool clamp) {
  unsigned int texture = 0;
 glGenTextures(1, &texture);
  // TODO: Check for valid texture generation and return error
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  if (clamp) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  } else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }
  glTexImage2D(GL_TEXTURE_2D, 0, (depth == 3) ? GL_RGB : GL_RGBA, width, height, 0, (depth == 3) ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
  return texture;
}

void GL::drawQueue(std::map<std::string, Queue> queue, bool select_mode, float time_elapsed) {
  static StateLoader *state_loader = System::instance()->getStateLoader();
  static ModelHandler *model_handler = _system->getModelHandler();
  for (std::map<std::string, Queue>::const_iterator I = queue.begin(); I != queue.end(); I++) {
    // Change state for this queue
    stateChange(state_loader->getStateProperties(I->first));
    for (Queue::const_iterator J = I->second.begin(); J != I->second.end(); J++) {

      WorldEntity *we = (WorldEntity *)*J;
      // Get model
      Model *model = model_handler->getModel(this, we);
      if (!model) {  // ERROR GETTING MODEL
	Log::writeLog("Trying to render NULL model", Log::LOG_ERROR);
        continue;
      }
      ObjectProperties *op = we->getObjectProperties(); 
      glPushMatrix();

      // Translate Model
      WFMath::Point<3> pos = we->getAbsPos();
      translateObject(pos.x(), pos.y(), pos.z() + terrain->getHeight(pos.x(), pos.y()));
     
      // Rotate Model
      if (model->rotationStyle()) rotateObject(we, model->rotationStyle());

      // Scale Object
      float scale = op->scale;
      if (scale != 0.0f && scale != 1.0f) glScalef(scale, scale, scale);

      // Update Model
      model->update(time_elapsed);
      
      // Draw Model
      if (select_mode) {
        nextColour(we->getID());
	model->render(true);
      } else {
        if (we->getID() == activeID) {
          active_name = we->getName();
	  drawOutline(model, checkState(RENDER_STENCIL) && op->outline);
	}
	else model->render(false);
      }
      
      glPopMatrix();
    }
  }
}

void GL::drawMessageQueue(std::map<std::string, Queue> queue) {
  glColor4fv(yellow);
  stateChange("font");
  for (std::map<std::string, Queue>::const_iterator I = queue.begin(); I != queue.end(); I++) {
    for (Queue::const_iterator J = I->second.begin(); J != I->second.end(); J++) {
      WorldEntity *we = (WorldEntity*)*J;
      glPushMatrix();
      WFMath::Point<3> pos = we->getAbsPos();
      glTranslatef(pos.x(), pos.y(), pos.z() + terrain->getHeight(pos.x(), pos.y()));
      WFMath::Quaternion  orient2 = WFMath::Quaternion(1.0f, 0.0f, 0.0f, 0.0f); // Initial Camera rotation
      orient2 /= _graphics->getCameraOrientation(); 
      applyQuaternion(orient2);
      glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
      glScalef(0.025f, 0.025f, 0.025f);
      glTranslatef(_speech_offset_x, _speech_offset_y, _speech_offset_z);
      we->renderMessages();
      glPopMatrix();
    }
  }
}
 
inline float GL::distFromNear(float x, float y, float z) {
  return Frustum::distFromNear(frustum, x, y, z);
}
	
inline int GL::patchInFrustum(WFMath::AxisBox<3> bbox) {
  return Frustum::patchInFrustum(frustum, bbox);
}

void GL::drawOutline(Model *model, bool use_stencil) {
  StateProperties *sp = _cur_state; // Store current state
  if (checkState(RENDER_STENCIL) && use_stencil) { // Using Stencil Buffer
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, -1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glPushMatrix();
    model->render(false);
    glPopMatrix();
    stateChange("halo");
    glStencilFunc(GL_NOTEQUAL, -1, 1);
    glColor4fv(_halo_colour);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    model->render(true);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_STENCIL_TEST);
    glColor4fv(white);
  } else { // Just use solid colour on object 
    stateChange(model->getSelectState());
    glColor4fv(_halo_colour);  
    model->render(true);
    glColor4fv(white);
  }
  stateChange(sp); // Restore state
}

void GL::createDefaults() {
  //Create Default Texture
  Log::writeLog("Building Default Texture", Log::LOG_INFO);
  unsigned int texture_id = 0;
  unsigned int width, height;
  glGenTextures(1, &texture_id);

  if (texture_id == 0) {
    Log::writeLog("Error creating default texture", Log::LOG_ERROR);
    return;
  }

  unsigned char *data = xpm_to_image((const char**)default_image_xpm, width, height);
  
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

  free (data);
  textureList.push_back(texture_id);
  texture_map["default"] = next_id++;
  
  //Create Default Font
  Log::writeLog("Building Default Font Texture", Log::LOG_INFO);
  texture_id = 0;
  glGenTextures(1, &texture_id);

  if (texture_id == 0) {
    Log::writeLog("Error creating default font", Log::LOG_ERROR);
    return;
  }

  //data = xpm_to_image(default_font, default_font_width, default_font_height);
  data = xpm_to_image((const char**)default_font_xpm, width, height);
  
  glBindTexture(GL_TEXTURE_2D, texture_id);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

  free (data);
  textureList.push_back(texture_id);
  texture_map["default_font"] = next_id++;
}
 
inline void GL::switchTexture(int texture) {
  switchTextureID(getTextureID(texture));
}

inline void GL::switchTextureID(unsigned int texture) {
  if (!glIsTexture(texture)) {
    static GLuint default_id = getTextureID(requestTexture("default", "default"));
    texture = default_id;
  }
  glBindTexture(GL_TEXTURE_2D, texture);
}

inline void GL::store() { glPushMatrix(); }
inline void GL::restore() { glPopMatrix(); }

inline void GL::beginFrame() {
  terrain = _graphics->getTerrain();
  active_name = "";
  if (checkState(RENDER_STENCIL)) {
    glClearStencil(1);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // Clear The Screen And The Depth Buffer
    } else {
      glClear(GL_DEPTH_BUFFER_BIT);
    }
  glLoadIdentity(); // Reset The View
  //Rotate Coordinate System so Z points upwards and Y points into the screen. 
  glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
}

inline void GL::endFrame(bool select_mode) {
  glFlush();
  if (!select_mode) SDL_GL_SwapBuffers();
#ifdef DEBUG
  CheckError();
#endif
}
  
inline void GL::drawFPS(float fps) {
  std::string frame_rate_string = string_fmt(fps).substr(0, 4);
  stateChange("font");
  glColor4fv(red);
  print(10, 100, frame_rate_string.c_str(), 0);
}
  
void GL::drawSplashScreen() {
  stateChange("splash");
  #ifndef _WIN32
    // TODO Need to find a win32 version
    usleep(sleep_time);
  #endif
  setViewMode(ORTHOGRAPHIC);
  
  glColor4fv(white);
  switchTexture(requestTexture("ui", "splash"));
  glBegin(GL_QUADS); 
    glTexCoord2i(0, 0); glVertex2f(0.0f, 0.0f);
    glTexCoord2i(0, 1); glVertex2f(0.0f, window_height);
    glTexCoord2i(1, 1); glVertex2f(window_width, window_height);
    glTexCoord2i(1, 0); glVertex2f(window_width, 0.0f);
  glEnd(); 
  setViewMode(PERSPECTIVE);
}
  
inline void GL::applyQuaternion(WFMath::Quaternion quaternion) {
  float rotation_matrix[4][4];
  QuatToMatrix(quaternion, rotation_matrix); //Get the rotation matrix for base rotation
  glMultMatrixf(&rotation_matrix[0][0]); //Apply rotation matrix
}
  
void GL::applyCharacterLighting(float x, float y, float z) {
  float ps[] = {x, y, z, 1.0f};
  glLightfv(GL_LIGHT0,GL_POSITION, ps);
}


void GL::applyLighting() {
  float tim = _system->getTimeOfDay();
  float dawn_time = _system->getDawnTime();
  float day_time = _system->getDayTime();
  float dusk_time = _system->getDuskTime();
  float night_time = _system->getNightTime();
     
  static GLfloat fog_colour[4];// = {0.50f, 0.50f, 0.50f, 0.50f};
  switch (_system->getTimeArea()) {
    case System::DAWN: {
      _light_level = (tim - dawn_time) / (day_time - dawn_time);
      float pos_mod = (tim - dawn_time) / (night_time - dawn_time);
      lights[LIGHT_SUN].x_pos = -200.0f * (pos_mod - 0.5f);
      break;
    }
    case System::DAY: {
      _light_level = 1.0f;
      float pos_mod = (tim - dawn_time) / (night_time - dawn_time);
      lights[LIGHT_SUN].x_pos = -200.0f * (pos_mod - 0.5f);
      break;
    }
    case System::DUSK: {
      _light_level = 1.0f - ((tim - dusk_time) / (night_time - dusk_time));
      float pos_mod = (tim - dawn_time) / (night_time - dawn_time);
      lights[LIGHT_SUN].x_pos = -200.0f * (pos_mod - 0.5f);
      break;
    }
    case System::NIGHT: {
      _light_level = 0.0f;
      break;
    }
  }
   
  fog_colour[0] = fog_colour[1] = fog_colour[2] = fog_colour[3] = 0.5f * _light_level;
  glFogfv(GL_FOG_COLOR, fog_colour);
  float sun_pos[] = {lights[LIGHT_SUN].x_pos, 0.0f, 100.0f, 1.0f};
  lights[LIGHT_SUN].ambient[0] = lights[LIGHT_SUN].ambient[1] = lights[LIGHT_SUN].ambient[2] = _light_level * 0.5f;
  lights[LIGHT_SUN].diffuse[0] = lights[LIGHT_SUN].diffuse[1] = lights[LIGHT_SUN].diffuse[2] = _light_level;
  glLightfv(GL_LIGHT1,GL_POSITION,sun_pos);
  glLightfv(GL_LIGHT1, GL_AMBIENT, lights[LIGHT_SUN].ambient);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, lights[LIGHT_SUN].diffuse);
}

inline void GL::resetSelection() {
  resetColours();
}

inline void GL::renderActiveName() {
  stateChange("font");
  glColor4fv(activeNameColour);
  print(x_pos, y_pos, active_name.c_str(), 1);
}

inline void GL::getFrustum(float frust[6][4]) {
  float  proj[16];
  float  modl[16];
  /* Get the current PROJECTION matrix from OpenGraphics */
  glGetFloatv(GL_PROJECTION_MATRIX, proj );
  /* Get the current MODELVIEW matrix from OpenGraphics */
  glGetFloatv(GL_MODELVIEW_MATRIX, modl );
  Frustum::getFrustum(frust, proj, modl);
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 4; j++) {
      frustum[i][j] = frust[i][j];
    }
  }
}
  
} /* namespace Sear */
