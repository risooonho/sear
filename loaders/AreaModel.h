// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2001 - 2005 Simon Goodall

#ifndef SEAR_AREAMODEL_H
#define SEAR_AREAMODEL_H

#include "Model.h"

namespace Mercator
{
    class Area;
}

namespace Sear
{

class ObjectRecord;

class AreaModel : public Model
{
public:
    AreaModel(Render*, ObjectRecord* orec);
    /**
    initialise the model. Result indicates success (true) or failure
    */
    bool init();    
    virtual ~AreaModel();
    
    virtual int shutdown();
    virtual void invalidate();    
    
    int getLayer() const;
private:
    ObjectRecord* m_object;
    Mercator::Area* m_area;
};

}

#endif // of SEAR_AREAMODEL_H
