// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2005 Simon Goodall, University of Southampton

// $Id: NPlane.h,v 1.9 2005-06-29 21:19:41 simon Exp $

#ifndef SEAR_NPLANE_H
#define SEAR_NPLANE_H 1

#include <string>

#include "common/types.h"

#include "Model.h"
#include "renderers/Graphics.h"
#include "renderers/RenderSystem.h"

namespace Sear {
	
class NPlane : public Model {
public:
  NPlane(Render*);
  ~NPlane();
  
  int init(const std::string &, unsigned int num_planes, float width, float height);
  int shutdown();
 
  void invalidate();
 
  void render(bool); 

  Graphics::RotationStyle rotationStyle() { return Graphics::ROS_POSITION; }
  
private:
  unsigned int m_num_planes;
  std::string m_texture_name;
  TextureID m_texture_id, m_texture_mask_id;
  Vertex_3 *m_vertex_data;
  Normal *m_normal_data;
  Texel *m_texture_data;
  bool m_initialised;

  int m_disp, m_select_disp;
};

} /* namespace Sear */ 
#endif /* SEAR_NPLANE_H */
