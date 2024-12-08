/*  =================== File Information =================
	File Name: ppm.cpp
	Description:
	Author: Michael Shah
	Last Modified: 4/2/14

	Purpose: 
	Usage:	
	===================================================== */

#include <FL/gl.h>
#include <iostream>
#include <string>
#include <fstream>
#include "shaders/ppm.h"

/*	===============================================
Desc:	Default constructor for a ppm
Precondition: _fileName is the image file name. It is also expected that the file is of type "ppm"
              It is expected that width and height are also set in the constructor. 
Postcondition: The array 'color' is allocated memory according to the image dimensions.
				width and height private members are set based on ppm header information.
=============================================== */ 
ppm::ppm(std::string _fileName){
	textureID = -1;
  /* Algorithm
      Step 1: Parse header of PPM
      Step 2: Read in colors into array
      Step 3: Allocate memory for width and height dimensions
  */


  // Open an input file stream for reading a file
  std::ifstream ppmFile(_fileName.c_str());
  // If our file successfully opens, begin to process it.
  if (ppmFile.is_open()) {
	  // line will store one line of input
	  std::string line;
	  // Our loop invariant is to continue reading input until
	  // we reach the end of the file and it reads in a NULL character
	  std::cout << "Reading in ppm file: " << _fileName << std::endl;
	  // Our delimeter pointer which is used to store a single token in a given
	  // string split up by some delimeter(s) specified in the strtok function
	  char* delimeter_pointer;
	  int iteration = 0;
	  int pos = 0;
	  while (getline(ppmFile, line)) {
		  char* copy = new char[line.length() + 1];
		  strcpy(copy, line.c_str());
		  delimeter_pointer = strtok(copy, " ");

		  if (copy[0] == '#') {
			  continue;
		  }

		  // Read in the magic number
		  if (iteration == 0) {
			  magicNumber = delimeter_pointer;
			  std::cout << "Magic Number: " << magicNumber << std::endl;
			  if (magicNumber.compare("P3") != 0) {
				  std::cout <<  "Incorrect image file format.Cannot load texutre" << std::endl;
				  break;
			  }
		  }
		  // Read in dimensions
		  else if (iteration == 1) {
			  width = atoi(delimeter_pointer);
			  std::cout << "width: " << width << " ";
			  delimeter_pointer = strtok(NULL, " ");
			  height = atoi(delimeter_pointer);
			  std::cout << "height: " << height << std::endl;
			  // Allocate memory for the color array
			  if (width > 0 && height > 0) {
				  color = new char[width*height * 3];
				  if (color == NULL) {
					  std::cout << "Unable to allocate memory for ppm" << std::endl;
					  break;
				  }
			  }
			  else {
				  std::cout << "PPM not parsed correctly, width and height dimensions are 0" << std::endl;
				  break;
			  }
		  }
		  else if (iteration == 2) {
			  std::cout << "color range: 0-" << delimeter_pointer << std::endl;

			  int num = width * height * 3;
			  for (int i = 0; i < num; i++) {
				  int value;
				  ppmFile >> value;
                  //std::cout << i << ": " << value << std::endl;
				  color[i] = value;
			  }
		  }
		  delete [] copy;
		  iteration++;
	  }
	  ppmFile.close();
  }
  else{
      std::cout << "Unable to open ppm file: " << _fileName << std::endl;
  } 
}



/*	===============================================
Desc:	Default destructor for a ppm
Precondition: 
Postcondition: 'color' array memory is deleted,
=============================================== */ 
ppm::~ppm(){
	if (textureID != -1) {
		glDeleteTextures(1, &textureID);
	}
	
	if (color!=NULL){
		delete[] color;
		color=NULL;
	}	
}

/*  ===============================================
Desc: Sets a pixel in our array a specific color
Precondition: 
Postcondition:
=============================================== */ 
void ppm::setPixel(int x, int y, int r, int g, int b){
  if(x > width || y > height){
    return;
  }
  else{
    /*std::cout << "modifying pixel at " 
              << x << "," << y << "from (" <<
              (int)color[x*y] << "," << (int)color[x*y+1] << "," << (int)color[x*y+2] << ")";
*/
    color[(x*3)+height*(y*3)] = r;
    color[(x*3)+height*(y*3)+1] = g;
    color[(x*3)+height*(y*3)+2] = b;
/*
    std::cout << " to (" << (int)color[x*y] << "," << (int)color[x*y+1] << "," << (int)color[x*y+2] << ")" << std::endl;
*/
  }
}

unsigned int ppm::bindTexture() {
	if (textureID == -1) {
		glGenTextures(1, &textureID);
	}
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, color);

	return textureID;
}

unsigned int ppm::getTextureID() {
	return textureID;
}