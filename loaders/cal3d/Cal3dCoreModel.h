// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2003 Simon Goodall

// $Id: Cal3dCoreModel.h,v 1.3 2003-03-06 12:52:08 simon Exp $

#ifndef SEAR_LOADERS_CAL3d_CAL3DCOREMODEL_H
#define SEAR_LOADERS_CAL3d_CAL3DCOREMODEL_H 1

#include <sigc++/object_slot.h>

#include <string>
#include <map>
#include <list>

#include <varconf/Config.h>
#include <cal3d/cal3d.h>

namespace varconf {
  class Config;
}

namespace Sear {
class Cal3dModel;
	
class Cal3dCoreModel : public SigC::Object {
public:
  friend class Cal3dModel;
	
  typedef std::map <std::string, unsigned int> MeshMap;
  typedef std::map <std::string, unsigned int> AnimationMap;
  typedef std::list <std::string> MaterialList;
  typedef std::map<std::string, unsigned int> MaterialMap;
  typedef std::map<std::string, MaterialMap> MaterialsMap;
  typedef std::map<std::string, unsigned int> PartMap;
  typedef std::map<std::string, unsigned int> SetMap;
 
  Cal3dCoreModel();
  ~Cal3dCoreModel();

  void init(const std::string &filename);
  void shutdown();

  CalCoreModel *getCalCoreModel() const { return _core_model; }
  Cal3dModel *instantiate(float height, varconf::Config &config);
 
  //Accessors
  float getScale() const { return _scale; }
  AnimationMap getAnimationMap() const { return _animations; }
 

 
private:
  void readConfig(const std::string &filename);
  void varconf_callback(const std::string &section, const std::string &key, varconf::Config &config);
  void varconf_error_callback(const char *message);

  unsigned int loadTexture(const std::string &strFilename);
  
  bool _initialised;
  CalCoreModel *_core_model;

  float _scale;
  MeshMap _meshes;
  AnimationMap _animations;
  MaterialList _material_list;
  MaterialsMap _materials;
  PartMap _parts;
  SetMap _sets;
  
};
} /* namespace Sear */

#endif /* SEAR__H */