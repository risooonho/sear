// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2002 Simon Goodall, University of Southampton

#ifndef _BOUNDBOX_H
#define _BOUNDBOX_H_ 1

#include "Models.h"
#include <wfmath/axisbox.h>

namespace Sear {

class BoundBox : public Models {
public:
  BoundBox(WFMath::AxisBox<3>, bool);
  ~BoundBox();
  
  bool init();
  void shutdown();
  
  const int getNumPoints() const { return _num_points; }  
  const float *getVertexData() const { return &_vertex_data[0][0]; }
  const float *getTextureData() const { return &_texture_data[0][0]; }
  const float *getNormalData() const { return &_normal_data[0][0]; }

  const bool hasVertexData() const { return true; }
  const bool hasTextureData() const { return true; }
  const bool hasNormalData() const { return true; } 
 
  const Type getType() const { return QUADS; }
  
private:
  static const int _num_points = 24;
  WFMath::AxisBox<3> _bbox;
  bool _wrap;
  float _vertex_data[_num_points][3];
  float _texture_data[_num_points][2];
  float _normal_data[_num_points][3];
};

} /* namespace Sear */
#endif /* _BOUNDBOX_H_ */
