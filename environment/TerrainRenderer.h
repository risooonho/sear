// This file may be redistributed and modified only under the terms of
// the GNU General Public License (See COPYING for details).
// Copyright (C) 2000 - 2003 Alistair Riddoch
// Copyright (C) 2004 - 2009 Simon Goodall

#ifndef SEAR_TERRAIN_RENDERER_H
#define SEAR_TERRAIN_RENDERER_H

#include <vector>

#include <sage/sage.h>
#include <sage/GL.h>

#include <Mercator/Terrain.h>
#include <Mercator/Shader.h>
#include <wfmath/point.h>

#include "renderers/RenderTypes.h"

namespace Eris {
  class TerrainModHandler;
}

namespace Sear {

class Character;
class Environment;
typedef WFMath::Point<3> PosType;


class TerrainRenderer 
{
  public:
    class DataSeg {
public:
      DataSeg() :
        vb_narray(0),
        vb_harray(0),
        disp(0),
        harray(NULL),
        narray(NULL),
        m_context_no(-1),
        m_list_set(false)
      {
      }
      
      ~DataSeg() {}
      
      std::map<int, GLuint> m_alphaTextures;
      
      GLuint vb_narray;
      GLuint vb_harray;
      GLuint disp;
      float *harray;
      float *narray;
      int m_context_no;
      bool m_list_set;

      void contextCreated();
      void contextDestroyed(bool check);

    };
    typedef std::map<int, DataSeg> DisplayListColumn;
    typedef std::map<int, DisplayListColumn> DisplayListStore;

    Mercator::Terrain m_terrain;

    class ShaderEntry
    {
    public:
        ShaderEntry(Mercator::Shader* s, const std::string& tnm) :
            shader(s),
            texId(0),
            texName(tnm)
        {}
        
        Mercator::Shader* shader;
        GLint texId;
        std::string texName; // for invalidation
    };

    std::vector<ShaderEntry> m_shaders;
    
    void contextCreated();
    void contextDestroyed(bool check);

    void reset();
  protected:
    DisplayListStore m_displayLists;
    int m_numLineIndeces;
    unsigned short * const m_lineIndeces;
    GLuint m_lineIndeces_vbo;
   
    TextureID m_seaTexture;
    TextureID m_shadowTexture;
    GLuint m_landscapeList;
    bool m_haveTerrain;
    Eris::TerrainModHandler *m_tmh;

    void enableRendererState();
    void disableRendererState();

    void generateAlphaTextures(Mercator::Segment *, DataSeg &);
    void drawRegion(Mercator::Segment *, DataSeg&, bool select_mode);
    void drawMap(Mercator::Terrain &, const PosType & camPos, bool select_mode);
    void drawSea( Mercator::Terrain &);
    void drawShadow(const WFMath::Point<2> & pos, float radius = 1.f);
    void setSurface(const std::string &name, const std::string &pattern, const Mercator::Shader::Parameters &params);
  void onActiveCharacterChanged(Character *c);
  public:
    TerrainRenderer();
    virtual ~TerrainRenderer();
    
    virtual void render( const PosType & camPos, bool select_mode);
    virtual void renderSea() { drawSea(m_terrain); }
    friend class Environment;
    
    void registerShader(Mercator::Shader*, const std::string& texId);
    void deregisterShader(Mercator::Shader* s);


  int m_context_no;
};
}
#endif // SEAR_TERRAIN_RENDERER_H
