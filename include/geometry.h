/*  =================== File Information =================
	File Name: geometry.h
	Description:
	Author: Michael Shah

	Purpose:
	Examples:
	===================================================== */

#include <cstddef>
#ifndef GEOMETRY_H
#define GEOMETRY_H

/*  ============== Vertex ==============
	Purpose: Stores properties of each vertex
	Use: Each vertex is used in face structure.

	Notes: You may not have all of the data available
	below.  In this case, you would want to make an
	optimization, and only store (x,y,z) cooridinates for example.
	==================================== */  
struct vertex{
	// position in 3D space
	float x,y,z;		
	// I believe this is used to determine if a vertex can be removed.
	// Search for polygon decimation or vertex removal for more ideas.
	// source: graphics.standford.edu/software/vrip/plyusage.html
	float confidence;	
	// I believe this has to do with lighting and shading, and this value
	// determines the shading coefficent used. This can be useful for when
	// we color the model.
	// source: www.okino.com/conv/imp_ply.htm
	float intensity;
	// Color values
	float r,g,b;	
	// surface normal values	
	float nx,ny,nz;		
	// texture coordinates
	float u,v,w;		
};

/*  ============== Face ==============
	Purpose: Store list of vertices that make up a polygon.
			In modern versions of OpenGL this value will always be 3(a triangle)
	Use: Used in Shape data structure.
	==================================== */  
struct face{
	// The number of vertices that make up a single face.
	// A face can have anywhere from 3 to n vertices.
	int vertexCount;
	// Stores an index(integer value) list of vertices. See struct 'vertex' for more information
	int* vertexList;

	// Default constructor
	face(){
		vertexCount = 0;
		vertexList = NULL;
	}
};

#endif