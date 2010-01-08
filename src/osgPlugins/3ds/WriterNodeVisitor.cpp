// -*-c++-*-

/*
 * 3DS reader/writer for Open Scene Graph
 *
 * Copyright (C) ???
 *
 * Writing support added 2007 by Sukender (Benoit Neil), http://sukender.free.fr,
 * strongly inspired by the OBJ writer object by Stephan Huber
 *
 * The Open Scene Graph (OSG) is a cross platform C++/OpenGL library for
 * real-time rendering of large 3D photo-realistic models.
 * The OSG homepage is http://www.openscenegraph.org/
 */

#include <osg/io_utils>
#include <osg/CullFace>
#include <osgDB/WriteFile>

#include "WriterNodeVisitor.h"
#include <assert.h>
#include <string.h>


void copyOsgMatrixToLib3dsMatrix(Lib3dsMatrix lib3ds_matrix, const osg::Matrix& osg_matrix)
{
    for(int row=0; row<4; ++row) {
        lib3ds_matrix[row][0] = osg_matrix.ptr()[row*4+0];
        lib3ds_matrix[row][1] = osg_matrix.ptr()[row*4+1];
        lib3ds_matrix[row][2] = osg_matrix.ptr()[row*4+2];
        lib3ds_matrix[row][3] = osg_matrix.ptr()[row*4+3];
    }
}

inline void copyOsgVectorToLib3dsVector(Lib3dsVector lib3ds_vector, const osg::Vec3f& osg_vector) {
    lib3ds_vector[0] = osg_vector[0];
    lib3ds_vector[1] = osg_vector[1];
    lib3ds_vector[2] = osg_vector[2];
}
inline void copyOsgVectorToLib3dsVector(Lib3dsVector lib3ds_vector, const osg::Vec3d& osg_vector) {
    lib3ds_vector[0] = osg_vector[0];
    lib3ds_vector[1] = osg_vector[1];
    lib3ds_vector[2] = osg_vector[2];
}

inline void copyOsgColorToLib3dsColor(Lib3dsVector lib3ds_vector, const osg::Vec4f& osg_vector) {
    lib3ds_vector[0] = osg_vector[0];
    lib3ds_vector[1] = osg_vector[1];
    lib3ds_vector[2] = osg_vector[2];
}
inline void copyOsgColorToLib3dsColor(Lib3dsVector lib3ds_vector, const osg::Vec4d& osg_vector) {
    lib3ds_vector[0] = osg_vector[0];
    lib3ds_vector[1] = osg_vector[1];
    lib3ds_vector[2] = osg_vector[2];
}

inline void copyOsgQuatToLib3dsQuat(float lib3ds_vector[4], const osg::Quat& osg_quat) {
    //lib3ds_vector[0] = osg_quat[3];        // Not sure
    //lib3ds_vector[1] = osg_quat[0];
    //lib3ds_vector[2] = osg_quat[1];
    //lib3ds_vector[3] = osg_quat[2];
    // 3DS seems to store (angle in radians, axis_x, axis_y, axis_z), but it works with (axis_x, axis_y, axis_z, -angle in radians)!
    osg::Quat::value_type angle, x, y, z;
    osg_quat.getRotate(angle, x, y, z);
    lib3ds_vector[0] = static_cast<float>(x);
    lib3ds_vector[1] = static_cast<float>(y);
    lib3ds_vector[2] = static_cast<float>(z);
    lib3ds_vector[3] = static_cast<float>(-angle);
}

std::string getFileName(const std::string & path) {
    unsigned int slashPos = path.find_last_of("/\\");
    if (slashPos == std::string::npos) return path;
    return path.substr(slashPos+1);
}


/// Checks if a filename (\b not path) is 8.3 (an empty name is never 8.3, and a path is never 8.3).
bool is83(const std::string & s) {
    // 012345678901
    // ABCDEFGH.ABC
    if (s.find_first_of("/\\") != std::string::npos) return false;            // It should not be a path, but a filename
    unsigned int len = s.length();
    if (len > 12 || len == 0) return false;
    unsigned int pointPos = s.rfind('.');
    if (pointPos == std::string::npos) return len <= 8;        // Without point
    // With point
    if (pointPos > 8) return false;
    if (len-1 - pointPos > 3) return false;
    return true;
}

/// Tests if the given string is a path supported by 3DS format (8.3, 63 chars max).
bool is3DSpath(const std::string & s) {
    unsigned int len = s.length();
    if (len >= 64 || len == 0) return false;

    unsigned int tokenBegin = 0;
    for (unsigned int tokenEnd=0; tokenEnd != std::string::npos; tokenBegin = tokenEnd+1) {
        tokenEnd = s.find_first_of("/\\", tokenBegin);
        if ( !is83(s.substr(tokenBegin, tokenEnd-tokenBegin-1)) ) return false;
    }
    return true;
}



/** writes all primitives of a primitive-set out to a stream, decomposes quads to triangles, line-strips to lines etc */
class PrimitiveIndexWriter : public osg::PrimitiveIndexFunctor {
public:
      PrimitiveIndexWriter(osg::Geometry  *    geo, 
                           ListTriangle  &    listTriangles,
                           unsigned int        drawable_n,
                           unsigned int        material) : 
          osg::PrimitiveIndexFunctor(),
          _drawable_n(drawable_n),
          _listTriangles(listTriangles),
          _hasNormalCoords(geo->getNormalArray() != NULL),
          _hasTexCoords(geo->getTexCoordArray(0) != NULL),
          _geo(geo),
          _lastFaceIndex(0),
          _material(material)
      {
      }

      unsigned int getNextFaceIndex() { return _lastFaceIndex; }

      virtual void setVertexArray(unsigned int,const osg::Vec2*) {}

      virtual void setVertexArray(unsigned int count,const osg::Vec3* vecs) {}

      virtual void setVertexArray(unsigned int,const osg::Vec4* ) {}

      virtual void setVertexArray(unsigned int,const osg::Vec2d*) {}

      virtual void setVertexArray(unsigned int ,const osg::Vec3d* ) {}
      virtual void setVertexArray(unsigned int,const osg::Vec4d* ) {}


      // operator for triangles
      void writeTriangle(unsigned int i1, unsigned int i2, unsigned int i3)
      {
          Triangle triangle;
          triangle.t1 = i1;
          triangle.t2 = i2;
          triangle.t3 = i3;
          triangle.material = _material;
          _listTriangles.push_back(std::make_pair(triangle, _drawable_n));
      }
      virtual void begin(GLenum mode)
      {
          _modeCache = mode;
          _indexCache.clear();
      }

      virtual void vertex(unsigned int vert)
      {
          _indexCache.push_back(vert);
      }

      virtual void end()
      {
          if (!_indexCache.empty())
          {
              drawElements(_modeCache,_indexCache.size(),&_indexCache.front());
          }
      }

      virtual void drawArrays(GLenum mode,GLint first,GLsizei count);

      virtual void drawElements(GLenum mode,GLsizei count,const GLubyte* indices)
      {
          drawElementsImplementation<GLubyte>(mode, count, indices);
      }
      virtual void drawElements(GLenum mode,GLsizei count,const GLushort* indices)
      {
          drawElementsImplementation<GLushort>(mode, count, indices);
      }

      virtual void drawElements(GLenum mode,GLsizei count,const GLuint* indices)
      {
          drawElementsImplementation<GLuint>(mode, count, indices);
      }

protected:

    template<typename T>void drawElementsImplementation(GLenum mode, GLsizei count, const T* indices)
    {
        if (indices==0 || count==0) return;

        typedef const T* IndexPointer;

        switch(mode)
        {
        case(GL_TRIANGLES):
            {
                //lib3ds_mesh_resize_faces(_mesh, _lastFaceIndex + count / 3);
                IndexPointer ilast = &indices[count];
                for(IndexPointer  iptr=indices;iptr<ilast;iptr+=3)
                    writeTriangle(*iptr,*(iptr+1),*(iptr+2));

                break;
            }
        case(GL_TRIANGLE_STRIP):
            {
                //lib3ds_mesh_resize_faces(_mesh, _lastFaceIndex + count -2);
                IndexPointer iptr = indices;
                for(GLsizei i=2;i<count;++i,++iptr)
                {
                    if ((i%2)) writeTriangle(*(iptr),*(iptr+2),*(iptr+1));
                    else       writeTriangle(*(iptr),*(iptr+1),*(iptr+2));
                }
                break;
            }
        case(GL_QUADS):
            {
                //lib3ds_mesh_resize_faces(_mesh, _lastFaceIndex + count /2);        // count/4*2
                IndexPointer iptr = indices;
                for(GLsizei i=3;i<count;i+=4,iptr+=4)
                {
                    writeTriangle(*(iptr),*(iptr+1),*(iptr+2));
                    writeTriangle(*(iptr),*(iptr+2),*(iptr+3));
                }
                break;
            }
        case(GL_QUAD_STRIP):
            {
                //lib3ds_mesh_resize_faces(_mesh, _lastFaceIndex + (count / 2 -1)*2);
                IndexPointer iptr = indices;
                for(GLsizei i=3;i<count;i+=2,iptr+=2)
                {
                    writeTriangle(*(iptr),*(iptr+1),*(iptr+2));
                    writeTriangle(*(iptr+1),*(iptr+3),*(iptr+2));
                }
                break;
            }
        case(GL_POLYGON): // treat polygons as GL_TRIANGLE_FAN
        case(GL_TRIANGLE_FAN):
            {
                //lib3ds_mesh_resize_faces(_mesh, _lastFaceIndex + count -2);
                IndexPointer iptr = indices;
                unsigned int first = *iptr;
                ++iptr;
                for(GLsizei i=2;i<count;++i,++iptr)
                {
                    writeTriangle(first,*(iptr),*(iptr+1));
                }
                break;
            }
        case(GL_POINTS):
        case(GL_LINES):
        case(GL_LINE_STRIP):
        case(GL_LINE_LOOP):
            // Not handled
            break;

        default:
            // uhm should never come to this point :)
            break;
        }
    }

private:

    PrimitiveIndexWriter& operator = (const PrimitiveIndexWriter&) { return *this; }

    unsigned int         _drawable_n;
    ListTriangle    &     _listTriangles;
    GLenum               _modeCache;
    std::vector<GLuint>  _indexCache;
    bool                 _hasNormalCoords, _hasTexCoords;
    osg::Geometry*       _geo;
    unsigned int         _lastFaceIndex;
    unsigned int         _material;
};


void PrimitiveIndexWriter::drawArrays(GLenum mode,GLint first,GLsizei count)
{
    switch(mode)
    {
    case(GL_TRIANGLES):
        {
            unsigned int pos=first;
            for(GLsizei i=2;i<count;i+=3,pos+=3)
            {
                writeTriangle(pos,pos+1,pos+2);
            }
            break;
        }
    case(GL_TRIANGLE_STRIP):
        {
            unsigned int pos=first;
            for(GLsizei i=2;i<count;++i,++pos)
            {
                if ((i%2)) writeTriangle(pos,pos+2,pos+1);
                else       writeTriangle(pos,pos+1,pos+2);
            }
            break;
        }
    case(GL_QUADS):
        {
            unsigned int pos=first;
            for(GLsizei i=3;i<count;i+=4,pos+=4)
            {
                writeTriangle(pos,pos+1,pos+2);
                writeTriangle(pos,pos+2,pos+3);
            }
            break;
        }
    case(GL_QUAD_STRIP):
        {
            unsigned int pos=first;
            for(GLsizei i=3;i<count;i+=2,pos+=2)
            {
                writeTriangle(pos,pos+1,pos+2);
                writeTriangle(pos+1,pos+3,pos+2);
            }
            break;
        }
    case(GL_POLYGON): // treat polygons as GL_TRIANGLE_FAN
    case(GL_TRIANGLE_FAN):
        {
            unsigned int pos=first+1;
            for(GLsizei i=2;i<count;++i,++pos)
            {
                writeTriangle(first,pos,pos+1);
            }
            break;
        }
    case(GL_POINTS):
    case(GL_LINES):
    case(GL_LINE_STRIP):
    case(GL_LINE_LOOP):
        //break;
    default:
        osg::notify(osg::WARN) << "WriterNodeVisitor :: can't handle mode " << mode << std::endl;
        break;
    }
}



WriterNodeVisitor::Material::Material(WriterNodeVisitor & writerNodeVisitor, osg::StateSet * stateset, osg::Material* mat, osg::Texture* tex, int index) :
    index(index),
    diffuse(1,1,1,1),
    ambient(0.2,0.2,0.2,1),
    specular(0,0,0,1),
    shininess(0),
    transparency(0),
    double_sided(false),
    image(NULL),
    texture_transparency(false),
    texture_no_tile(false)
{
    //static unsigned int s_objmaterial_id = 0;
    //++s_objmaterial_id;
    if (mat) {
        assert(stateset);
        diffuse = mat->getDiffuse(osg::Material::FRONT);
        ambient = mat->getAmbient(osg::Material::FRONT);
        specular = mat->getSpecular(osg::Material::FRONT);
        shininess = mat->getShininess(osg::Material::FRONT);
        transparency = 1-diffuse.w();
        name = writerNodeVisitor.getUniqueName(mat->getName(),"mat");
        osg::StateAttribute * attribute = stateset->getAttribute(osg::StateAttribute::CULLFACE);
        if (!attribute) {
            double_sided = true;
        } else {
            assert(dynamic_cast<osg::CullFace *>(attribute));
            osg::CullFace::Mode mode = static_cast<osg::CullFace *>(attribute)->getMode();
            if (mode == osg::CullFace::BACK) double_sided = false;
            else if (mode == osg::CullFace::FRONT) {
                osg::notify(osg::WARN) << "3DS Writer: Reversed face (culled FRONT) not supported yet." << std::endl;
                double_sided = false;
            }
            else {
                assert(mode == osg::CullFace::FRONT_AND_BACK);
                osg::notify(osg::WARN) << "3DS Writer: Invisible face (culled FRONT_AND_BACK) not supported yet." << std::endl;
                double_sided = false;
            }
        }
    }
    if (tex) {
        osg::Image* img = tex->getImage(0);
        if(img)
        {
            texture_transparency = (stateset->getMode(GL_BLEND) == osg::StateAttribute::ON);
            texture_no_tile = (tex->getWrap(osg::Texture2D::WRAP_S) == osg::Texture2D::CLAMP);
            image = img;
        }
    }

    if (name.empty()) {
        std::stringstream ss;
        ss << "m" << index;
        name = ss.str();
    }
}


std::string
getPathRelative(const std::string & srcBad,
                const std::string & dstBad)
{
    if(srcBad.empty())
        return osgDB::getSimpleFileName(dstBad);
    const std::string & src = osgDB::convertFileNameToNativeStyle(srcBad);
    const std::string & dst = osgDB::convertFileNameToNativeStyle(dstBad);
    std::string::const_iterator itDst = dst.begin();
    std::string::const_iterator itSrc = src.begin();

    std::string result = "";

    while(itDst != dst.end())
    {
        if (itSrc != src.end() && *itDst == *itSrc)
            ++itSrc;
        else if (!result.empty() || *itDst != '\\')  
            result += *itDst;
        ++itDst;
    }
    if (itSrc != src.end())
        result = osgDB::getSimpleFileName(dst);
    return result;
}

void WriterNodeVisitor::writeMaterials()
{
    unsigned int nbMat = _materialMap.size();
    lib3ds_file_reserve_materials(file3ds, nbMat, 1);
    // Ugly thing: it seems lib3ds_file_insert_material() doesn't support insertion in a random order (else materials are not assigned the right way)
    for (unsigned int iMat=0; iMat<nbMat; ++iMat)
    {
        bool found = false;
        for(MaterialMap::iterator itr = _materialMap.begin(); itr != _materialMap.end(); ++itr)
        {
            const Material & mat = itr->second;
            if (mat.index != static_cast<int>(iMat)) continue;        // Ugly thing (2)
            found = true;

            assert(mat.index>=0 && mat.index < static_cast<int>(_materialMap.size()));
            Lib3dsMaterial * mat3ds = lib3ds_material_new(getFileName(mat.name).c_str());
            copyOsgColorToLib3dsColor(mat3ds->ambient,  mat.ambient);
            copyOsgColorToLib3dsColor(mat3ds->diffuse,  mat.diffuse);
            copyOsgColorToLib3dsColor(mat3ds->specular, mat.specular);
            mat3ds->shininess = mat.shininess;
            mat3ds->transparency = mat.transparency;
            mat3ds->two_sided = mat.double_sided ? 1 : 0;
            if (mat.image)
            {
                Lib3dsTextureMap & tex = mat3ds->texture1_map;
                std::string path;
                if(mat.image->getFileName().empty())
                {
                    std::ostringstream oss;
                    oss << "Image_" << _imageCount++ << ".rgb";
                    path = oss.str();
                }
                else {
                    path = getPathRelative(_srcDirectory, mat.image->getFileName());
                }
                if(!is3DSpath(path)) {
                    path = getUniqueName(path, "", true);
                    //path = osgDB::getSimpleFileName(path);
                }

                strcpy(tex.name, path.c_str());
                path = osgDB::concatPaths(_directory, path);
                osgDB::makeDirectoryForFile(path);

                //if (mat.image->valid()) osgDB::writeImageFile(*(mat.image), path);
                osgDB::writeImageFile(*(mat.image), path);
                if (mat.texture_transparency) tex.flags |= LIB3DS_TEXTURE_ALPHA_SOURCE;
                if (mat.texture_no_tile) tex.flags |= LIB3DS_TEXTURE_NO_TILE;
            }
            if (!suceedLastApply())
                return;
            lib3ds_file_insert_material(file3ds, mat3ds, itr->second.index);
            break;        // Ugly thing (3)
        }
        if (!found) throw "Implementation error";                // Ugly thing (4)
    }
}


std::string WriterNodeVisitor::getUniqueName(const std::string& _defaultValue, const std::string & _defaultPrefix, bool nameIsPath) {
    if (_defaultPrefix.length()>=4) throw "Default prefix is too long";            // Arbitrarily defined to 3 chars. You can modify this, but you may have to change the code so that finding a number is okay, even when changing the default prefix length.

    // Tests if default name is valid and unique
    bool defaultIs83 = is83(_defaultValue);
    bool defaultIsValid = nameIsPath ? is3DSpath(_defaultValue) : defaultIs83;
    if (defaultIsValid && _nameMap.find(_defaultValue) == _nameMap.end()) {
        _nameMap.insert(_defaultValue);
        return _defaultValue;
    }

    // Handling of paths is not well done yet. Defaulting to something very simple.
    // We should actually ensure each component is 8 chars long, and final filename is 8.3, and total is <64 chars.
    std::string defaultValue(nameIsPath ? osgDB::getSimpleFileName(_defaultValue) : _defaultValue);
    std::string ext(nameIsPath ? osgDB::getFileExtensionIncludingDot(_defaultValue).substr(0, std::min<unsigned int>(_defaultValue.size(), 4)) : "");        // 4 chars = dot + 3 chars

    std::string defaultPrefix(_defaultPrefix.empty() ? "_" : _defaultPrefix);

    unsigned int max_val = 0;
    std::string truncDefaultValue = "";
    for (unsigned int i = 0; i < std::min<unsigned int>(defaultValue.size(), 4); ++i)
    {
        if (defaultValue[i] == '.')
        {
            truncDefaultValue = defaultValue.substr(0, i);
            break;
        }
    }
    if (truncDefaultValue.empty())
        truncDefaultValue = defaultValue.substr(0, std::min<unsigned int>(defaultValue.size(), 4));
    std::map<std::string, unsigned int>::iterator pairPrefix;
    defaultIs83 = is83(truncDefaultValue);
    if (defaultIs83)
    {
        max_val = static_cast<unsigned int>(pow(10., 8. - truncDefaultValue.length() - 1)) -1;        // defaultPrefix.length()-1 because we add an underscore ("_")
        pairPrefix = _mapPrefix.find(truncDefaultValue);
    }  

    if (defaultIs83 && (_mapPrefix.end() == pairPrefix || pairPrefix->second <= max_val))
    {
        defaultPrefix = truncDefaultValue;
    }
    else
    {
        max_val = static_cast<unsigned int>(pow(10., 8. - defaultPrefix.length() - 1)) -  1;        // defaultPrefix.length()-1 because we add an underscore ("_")
        pairPrefix = _mapPrefix.find(defaultPrefix);
    }

    unsigned int searchStart = 0;
    if (pairPrefix != _mapPrefix.end())
        searchStart = pairPrefix->second;

    for(unsigned int i = searchStart; i <= max_val; ++i) {
        std::stringstream ss;
        ss << defaultPrefix << "_" << i;
        const std::string & res = ss.str();
        if (_nameMap.find(res) == _nameMap.end()) {
            if (pairPrefix != _mapPrefix.end())
            {
                pairPrefix->second = i + 1;
            }
            else
            {
                _mapPrefix.insert(std::make_pair(defaultPrefix, i + 1));
            }
            _nameMap.insert(res);
            return res + ext;
        }
    }
    if (defaultPrefix == "_") _lastGeneratedNumberedName = max_val;
    throw "No more names available! Is default prefix too long?";
}

int WriterNodeVisitor::processStateSet(osg::StateSet* ss)
{
    MaterialMap::const_iterator itr = _materialMap.find(ss);
    if (itr != _materialMap.end()) {
        assert(itr->second.index>=0);
        return itr->second.index;
    }

    osg::Material* mat = dynamic_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
    osg::Texture* tex = dynamic_cast<osg::Texture*>(ss->getTextureAttribute(0, osg::StateAttribute::TEXTURE));

    if (mat || tex)
    {
        int matNum = _lastMaterialIndex;
        _materialMap.insert(std::make_pair(osg::ref_ptr<osg::StateSet>(ss), Material(*this, ss, mat, tex, matNum) ));
        ++_lastMaterialIndex;
        return matNum;
    }
    return -1;
}

/** 
*  Add a vertice to the index and link him with the Triangle index and the drawable.
*  \param index_vert is the map where the vertice are stored.
*  \param index is the indice of the vertice's position in the vec3.
*  \param drawable_n is the number of the drawable.
*  \return the position of the vertice in the final mesh.
*/
unsigned int
WriterNodeVisitor::getMeshIndexForGeometryIndex(MapIndices & index_vert, 
                                                unsigned int index,
                                                unsigned int drawable_n)
{
    MapIndices::iterator itIndex = index_vert.find(std::make_pair(index, drawable_n));
    if (itIndex == index_vert.end()) {
        unsigned int indexMesh = index_vert.size();
        index_vert.insert(std::make_pair(std::make_pair(index, drawable_n), indexMesh));
        return indexMesh;
    }
    return itIndex->second;
}


void 
WriterNodeVisitor::buildMesh(osg::Geode                  &    geo,
                             MapIndices                  &    index_vert,
                             bool                        texcoords,
                             Lib3dsMesh                  *    mesh)
{
    osg::notify(osg::DEBUG_INFO) << "Building Mesh" << std::endl;

    if (!mesh) throw "Allocation error";        // TODO

    lib3ds_mesh_resize_vertices(mesh, index_vert.size(), texcoords ? 1 : 0, 0);
    // Write points

    for(MapIndices::iterator it = index_vert.begin(); it != index_vert.end();++it)
    {
        osg::Geometry *g = geo.getDrawable( it->first.second )->asGeometry();
        assert(g->getVertexArray());
        if (g->getVertexArray()->getType() != osg::Array::Vec3ArrayType)
            throw "Vertex array is not Vec3. Not implemented";        // TODO
        const osg::Vec3Array & vecs= *static_cast<osg::Vec3Array *>(g->getVertexArray());
        copyOsgVectorToLib3dsVector(mesh->vertices[it->second], vecs[it->first.first]);
    }

    // Write texture coords (Texture 0 only)
    if (texcoords)
    {
        for(MapIndices::iterator it = index_vert.begin(); it != index_vert.end(); ++it)
        {
            osg::Geometry *g = geo.getDrawable( it->first.second )->asGeometry();
            osg::Array * array = g->getTexCoordArray(0);
            if(array)
            {
                if (g->getTexCoordArray(0)->getType() != osg::Array::Vec2ArrayType)
                    throw "Texture coords array is not Vec2. Not implemented";        // TODO
                const osg::Vec2Array & vecs= *static_cast<osg::Vec2Array *>(array);
                mesh->texcos[it->second][0] = vecs[it->first.first][0];
                mesh->texcos[it->second][1] = vecs[it->first.first][1];
            }
        }
    }
    lib3ds_file_insert_mesh(file3ds, mesh, _lastMeshIndex);
    ++_lastMeshIndex;

    Lib3dsMeshInstanceNode * node3ds = lib3ds_node_new_mesh_instance(mesh, mesh->name, NULL, NULL, NULL);
    lib3ds_file_append_node(file3ds, reinterpret_cast<Lib3dsNode*>(node3ds), reinterpret_cast<Lib3dsNode*>(_cur3dsNode));
}

unsigned int 
WriterNodeVisitor::calcVertices(osg::Geode & geo)
{
    unsigned int numVertice = 0;
    for (unsigned int i = 0; i < geo.getNumDrawables(); ++i)
    {
        osg::Geometry *g = geo.getDrawable( i )->asGeometry();
        assert(g->getVertexArray());
        if (g->getVertexArray()->getType() != osg::Array::Vec3ArrayType)
            throw "Vertex array is not Vec3. Not implemented";        // TODO
        const osg::Vec3Array & vecs= *static_cast<osg::Vec3Array *>(g->getVertexArray());
        numVertice += vecs.getNumElements();
    }
    return numVertice;
}


void
WriterNodeVisitor::buildFaces(osg::Geode     &    geo,
                              ListTriangle   &    listTriangles,
                              bool                texcoords)
{
    MapIndices index_vert;
    unsigned int nbFace = 0;
    Lib3dsMesh *mesh = lib3ds_mesh_new( getUniqueName(geo.getName().empty() ? geo.className() : geo.getName(), "geo").c_str() );
    unsigned int nbTriangles = listTriangles.size();

    lib3ds_mesh_resize_faces(mesh, nbTriangles);

    unsigned int nbVertices = calcVertices(geo);
    if (listTriangles.size() >= MAX_FACES-2 ||
       ((nbVertices) >= MAX_VERTICES-2))
    {
        osg::notify(osg::ALWAYS) << "Sorting elements..." << std::endl;
        WriterCompareTriangle cmp(geo, nbVertices);
        std::sort(listTriangles.begin(), listTriangles.end(), cmp);
    }

    for (ListTriangle::iterator it = listTriangles.begin(); it != listTriangles.end(); ++it) //Go through the triangle list to define meshs
    {
        // Using -2 due to the fact that we treat 3 faces in one time (=the algorithm may overrun the limit by 2).
        if ((index_vert.size() >= MAX_VERTICES-2 ||        // If mesh is full
            nbFace >= MAX_FACES-2))
        {
            // Finnishing mesh
            lib3ds_mesh_resize_faces(mesh, nbFace);
            buildMesh(geo, index_vert, texcoords, mesh);

            // Creating a new mesh
            index_vert.clear();
            mesh = lib3ds_mesh_new( getUniqueName(geo.getName().empty() ? geo.className() : geo.getName(), "geo").c_str());
            nbTriangles -= nbFace;
            nbFace = 0;
            lib3ds_mesh_resize_faces(mesh, nbTriangles);
        }
        Lib3dsFace & face = mesh->faces[nbFace++];
        face.index[0] = getMeshIndexForGeometryIndex(index_vert, it->first.t1, it->second);
        face.index[1] = getMeshIndexForGeometryIndex(index_vert, it->first.t2, it->second);
        face.index[2] = getMeshIndexForGeometryIndex(index_vert, it->first.t3, it->second);
        face.material = it->first.material;
    }
    buildMesh(geo, index_vert, texcoords, mesh); //When a Mesh is completed without restriction of vertices number
}

void 
WriterNodeVisitor::createListTriangle(osg::Geometry    *    geo, 
                                      ListTriangle    &    listTriangles,
                                      bool            &    texcoords,
                                      unsigned int    &   drawable_n)
{
    unsigned int nbVertices = 0;
    {
        if (geo->getVertexArray() && geo->getVertexArray()->getType() != osg::Array::Vec3ArrayType)
            throw "Vertex array is not Vec3. Not implemented";        // TODO
        const osg::Vec3Array * vecs = geo->getVertexArray() ? static_cast<osg::Vec3Array *>(geo->getVertexArray()) : NULL;
        if (vecs) 
        {
            nbVertices = geo->getVertexArray()->getNumElements();
            // Texture coords
            if (geo->getTexCoordArray(0) && geo->getTexCoordArray(0)->getType() != osg::Array::Vec2ArrayType)
                throw "Texture coords array is not Vec2. Not implemented";        // TODO
            const osg::Vec2Array * texvecs = geo->getTexCoordArray(0) ? static_cast<osg::Vec2Array *>(geo->getTexCoordArray(0)) : NULL;
            if (texvecs) 
            {
                unsigned int nb = geo->getTexCoordArray(0)->getNumElements();
                if (nb != geo->getVertexArray()->getNumElements()) throw "There are more/less texture coords than vertices!";
                texcoords = true;
            }
        }
    }

    if (nbVertices==0) return;

    int material = processStateSet(_currentStateSet.get());    

    for(unsigned int i = 0; i < geo->getNumPrimitiveSets(); ++i) //Fill the Triangle List
    {
        osg::PrimitiveSet* ps = geo->getPrimitiveSet(i);
        PrimitiveIndexWriter pif(geo, listTriangles, drawable_n, material);
        ps->accept(pif);
    }
}

bool WriterNodeVisitor::suceedLastApply() const
{
    return _suceedLastApply;
}

void WriterNodeVisitor::failedApply()
{
    _suceedLastApply = false;
    osg::notify(osg::NOTICE) << "Error going through node" << std::endl;
}

void WriterNodeVisitor::apply( osg::Geode &node )
{
    pushStateSet(node.getStateSet());
    //_nameStack.push_back(node.getName());
    //osg::Matrix m = osg::computeLocalToWorld(getNodePath());
    unsigned int count = node.getNumDrawables();
    ListTriangle listTriangles;
    bool texcoords = false;
    for ( unsigned int i = 0; i < count; i++ )
    {
        osg::Geometry *g = node.getDrawable( i )->asGeometry();
        if ( g != NULL )
        {
            pushStateSet(g->getStateSet());
            createListTriangle(g, listTriangles, texcoords, i);
            popStateSet(g->getStateSet());
        }
    }
    if (count > 0)
    {
        buildFaces(node, listTriangles, texcoords);
    }
    popStateSet(node.getStateSet());
    //_nameStack.pop_back();
    if (suceedLastApply())
        traverse(node);
}

void WriterNodeVisitor::apply(osg::Group &node)
{
    Lib3dsMeshInstanceNode * parent = _cur3dsNode;
    Lib3dsMeshInstanceNode * node3ds = lib3ds_node_new_mesh_instance(NULL, getUniqueName(node.getName().empty() ? node.className() : getFileName(node.getName()), "grp").c_str(), NULL, NULL, NULL);
    lib3ds_file_append_node(file3ds, reinterpret_cast<Lib3dsNode*>(node3ds), reinterpret_cast<Lib3dsNode*>(parent));
    _cur3dsNode = node3ds;
    if (suceedLastApply())
        traverse(node);
    _cur3dsNode = parent;
}

void WriterNodeVisitor::apply(osg::MatrixTransform &node)
{
    Lib3dsMeshInstanceNode * parent = _cur3dsNode;

    const osg::Matrix & m = node.getMatrix();
    //const osg::Matrix m( osg::computeWorldToLocal(getNodePath()) );        // [NEEDS TESTING!] 3DS matrices always contain world to local transformation (not local transform; ie. from parent)

    // Transform data used to be given to lib3ds_node_new_mesh_instance(), but it seems buggy (pivot problem? bug in conversion?).
    float pos[3];
    float scl[3];
    float rot[4];
    osg::Vec3 osgScl, osgPos;
    osg::Quat osgRot, osgSo;
    m.decompose(osgPos, osgRot, osgScl, osgSo);
    copyOsgVectorToLib3dsVector(pos, osgPos);
    copyOsgVectorToLib3dsVector(scl, osgScl);
    copyOsgQuatToLib3dsQuat(rot, osgRot);
    Lib3dsMeshInstanceNode * node3ds = lib3ds_node_new_mesh_instance
        (NULL, getUniqueName(node.getName().empty() ? node.className() : node.getName(), "mtx").c_str(), pos, scl, rot);

    //// Create a mesh instance with no transform and then copy the matrix (doesn't work)
    //Lib3dsMeshInstanceNode * node3ds = lib3ds_node_new_mesh_instance
    //    (NULL, getUniqueName(node.getName().empty() ? node.className() : node.getName(), "mtx").c_str(), NULL, NULL, NULL);
    //    copyOsgMatrixToLib3dsMatrix(node3ds->base.matrix, m);

    lib3ds_file_append_node(file3ds, reinterpret_cast<Lib3dsNode*>(node3ds), reinterpret_cast<Lib3dsNode*>(parent));
    _cur3dsNode = node3ds;
    if (suceedLastApply())
        traverse(node);
    _cur3dsNode = parent;
}