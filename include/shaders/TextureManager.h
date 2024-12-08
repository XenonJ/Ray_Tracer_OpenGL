/*  =================== File Information =================
	File Name: object.h
	Description:
	Author: Michael Shah
	Last Modified: 4/2/14

	Purpose: 
	Usage:	

	Further Reading resources: 
	===================================================== */
#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#if defined(__APPLE__)
#  include <OpenGL/gl3.h> // defines OpenGL 3.0+ functions
#else
#  if defined(WIN32)
#    define GLEW_STATIC 1
#  endif
#  include <GL/glew.h>
#endif
#include <FL/glu.h>
#include "ppm.h"

#include <map>

class TextureManager {
	public:
		TextureManager();
		~TextureManager();

		/*	===============================================
		Desc:
		Precondition: 
		Postcondition:
		=============================================== */
		void loadTexture(std::string textureName, std::string fileName);
		void deleteTexture(std::string textureName);
		unsigned int getTextureID (std::string textureName);
		
	private:
		std::map<std::string, ppm*> textures;
};

#endif