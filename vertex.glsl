// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Edits n stuff made by me, Cameron Hardy
// Date:    2016
// ==========================================================================
#version 410

// location indices for these attributes correspond to those specified in the
// InitializeGeometry() function of the main program
layout(location = 0) in vec2 VertexPosition;
layout(location = 1) in vec2 VertexUV;

// output to be interpolated between vertices and passed to the fragment stage
out vec2 uv;

uniform float zLevel; // zoom level
uniform vec2 offset; // x and y offset
uniform float theta; // angle of rotation

void main()
{
  // Let's make a rotation matrix
  mat2 rMatrix;
  rMatrix[0] = vec2(cos(theta), -sin(theta));
  rMatrix[1] = vec2(sin(theta), cos(theta));

  float x = (VertexPosition.x + offset.x); // add those offsets boy
  float y = (VertexPosition.y + offset.y); // add those other offsets too
  vec2 pos = vec2(x, y) * rMatrix; // u spin me right round
  gl_Position = vec4(pos, 0.0, zLevel);
	uv = VertexUV;
}
