/*!
   @file   SceneParser.h
   @author Eric Tamura (October 2006)
   @author Nong Li (August 2007)
   @author Remco Chang (December 2013)
*/

#ifndef SCENEPARSER_H
#define SCENEPARSER_H

#include "SceneData.h"
#include <fstream>
#include <vector>
#include <map>

class TiXmlElement;


//! This class parses the scene graph specified by the CS123 Xml file format.
class SceneParser
{
   public:
      //! Create a parser, passing it the scene file.
      SceneParser(const std::string& filename);
      //! Clean up all data for the scene
      ~SceneParser();

      //! Parse the scene.  Returns false if scene is invalid.
      bool parse();

      //! Returns global scene data
      bool getGlobalData(SceneGlobalData& data);

      //! Returns camera data
      bool getCameraData(SceneCameraData& data);

      //! Returns the root scenegraph node
      SceneNode* getRootNode();

      //! Returns number of lights in the scene
      int getNumLights();

      //! Returns the ith light data
      bool getLightData(int i, SceneLightData& data);


   private:
      //the filename should be contained within this parser implementation
      //if you want to parse a new file, instantiate a different parser
      bool parseGlobalData(const TiXmlElement* scenefile);
      bool parseCameraData(const TiXmlElement* scenefile);
      bool parseLightData(const TiXmlElement* scenefile);
      bool parseObjectData(const TiXmlElement* scenefile);
      bool parseTransBlock(const TiXmlElement* transblock, SceneNode* node);
      bool parsePrimitive(const TiXmlElement* prim, SceneNode* node);
      
      std::string file_name;
      mutable std::map<std::string, SceneNode*> m_objects;
      SceneCameraData m_cameraData;
      std::vector<SceneLightData*> m_lights;
      SceneGlobalData m_globalData;
      std::vector<SceneNode*> m_nodes;
};

class Texture {
public:
    // Constructor with filename
    explicit Texture(const std::string& filename);
    // Optional: Default constructor
    Texture() : filename(""), width(0), height(0) {}
    // 使用默认的拷贝构造函数和赋值运算符
    Texture(const Texture&) = default;
    Texture& operator=(const Texture&) = default;

    // Accessors
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    glm::vec3 getColor(float u, float v, float repeatU, float repeatV) const;

private:
    // Attributes
    std::string filename;
    int width;
    int height;
    std::map<std::pair<int, int>, glm::vec3> colors;

    // Load the PPM file (simplified version)
    void loadFromFile(const std::string& filename);
};

#endif

