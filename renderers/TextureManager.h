// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2003 Simon Goodall, University of Southampton

// $Id: TextureManager.h,v 1.3 2003-03-11 23:33:46 simon Exp $

#ifndef SEAR_RENDER_TEXTUREMANAGER_H
#define SEAR_RENDER_TEXTUREMANAGER_H 1

#include <string>
#include <map>
#include <vector>

#include "SDL.h"

#include <sigc++/object_slot.h>

#include <varconf/Config.h>

/*
 TODO

 Add console commands
 - Flush textures
 - determine max_texture_units from gl query
 - read/write config values
 - add ability to resample image to 2^N by 2^M
 - allow more formats thanb just RGB and RGBA
 - work in image loaders
 - create default textures

  */ 
namespace Sear {

  typedef unsigned int TextureObject; ///< OpenGL reference to texture
  typedef int TextureID; ///< TextureManager reference to texture
/**
 * This class handles everything to do with textures. 
 * Expected usuage is to call requestTexture to initially load the texture
 * and store the returned value.
 */ 
	
class TextureManager : public SigC::Object {

public:
  
  typedef std::map<std::string, TextureID> TextureMap;
  typedef std::vector<TextureObject> TextureVector;
 
  /**
   * Default constructor
   */   
  TextureManager();

  /**
   * Destructor
   */ 
  ~TextureManager() {
    // Call shutdown if it hasn't already been
    if (_initialised) shutdown();
  }

  /**
   * Initialise a TextureManager object
   */ 
  void init();

  /**
   * Clean up the TextureManager object
   */ 
  void shutdown();

  /**
   * Reads the texture manager config from the specified config object
   * @param config Config object containing config data
   */ 
  void readConfig(varconf::Config &config);

  /**
   * Writes the texture manager config to the specified config object
   * @param config Config object to write config data to
   */ 
  void writeConfig(varconf::Config &config);
  
  /**
   * This reads in texture configuration data from the specified file
   * @param filename Filename to texture configuration file
   */ 
  void readTextureConfig(const std::string &filename);

  /** 
   * Loads the requested texture. Parameters for the textures are taken from thr
   * config values accessed by texture_name. The return value should be used 
   * with getTextureObejct to get the actual ID value.
   * @param texture_name Name of texture to load
   * @return ID for texture.
   */ 
  TextureID loadTexture(const std::string &texture_name);

  /**
   * Unloads the specified texture from the OpenGL system
   * @param texture_name Name of texture to unload
   */ 
  void unloadTexture(const std::string &texture_name);

  /**
   * Unloads the specified texture from the OpenGL system
   * @param texture_object TextureObject to unload
   */ 
  void unloadTexture(TextureObject texture_object);

  /**
   * Returns the textureID associated with a given texture
   * @param texture_name Name of texture
   * @return TextureID of texture. 0 if not loaded, -1 if error during loading
   */ 
  TextureID getTextureID(const std::string &texture_name) {
    return _texture_map[texture_name];
  }

  /**
   * Returns TextureObject for a given TextureID
   * @param texture TextureID of texture
   * @return TextureObject of texture
   */
  TextureObject getTextureObject(TextureID texture) {
    return _textures[texture];
  }

  /**
   * This is the standard function to switch textures. Will only
   * change textures if the requested texture is different from the 
   * currently loaded one.
   * @param texture_id TextureID of the texture to load
   */ 
  void switchTexture(TextureID texture_id);

  /**
   * This is the function to switch multiple textures. Will only
   * change textures if the requested textures are different from the 
   * currently loaded ones.
   * @param texture_ids Pointer to array of TextureIDs to load in
   * @param num_textures Number of textures in array
   */ 
  void switchMultiTexture(TextureID *texture_ids, unsigned int num_textures);
  
  void switchTexture(unsigned int texture_unit, TextureID texture_ids);

  void setScale(float scale);
  void setScale(float scale_x, float scale_y);
  void setScale(unsigned int texture_unit, float scale);
  void setScale(unsigned int texture_unit, float scale_x, float scale_y);

  unsigned int getNumTextureUnits() const { return _texture_units; }
  
private:
  bool _initialised; ///< Flag indicating whether object has had init called
  varconf::Config _texture_config; ///< Config object for all texture
  TextureMap _texture_map; ///< Mapping between texture name and its TextureID
  TextureVector _textures; ///< Used to translate a TextureID to a TextureObject
  unsigned int _texture_counter; ///< Keeps track of last allocated TextureID
  TextureID *_last_textures;
  int _texture_units;

  /**
   * This function is used to setup the required OpenGL texture extensions
   */ 
  void setupGLExtensions();
  TextureObject loadTexture(const std::string &texture_name, SDL_Surface *surface);
  void varconf_callback(const std::string &section, const std::string &key, varconf::Config &config);	  
  void varconf_error_callback(const char *message);

  TextureID createDefaultTexture();
  TextureID createDefaultFont();

  
  /**
   * Returns the OpenGL filter from the name
   * @param filter_name String name of the filter
   * @return The filter, or 0 is a wrong name was specified
   */ 
  static unsigned int getFilter(const std::string &filter_name);
};
  
} /* namespace Sear */

#endif /* SEAR_RENDER_TEXTUREMANAGER_H */