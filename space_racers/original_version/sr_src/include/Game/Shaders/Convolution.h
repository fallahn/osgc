//***convolution shader. Works on a 3x3 matrix for different effects***//
// See http://docs.gimp.org/en/plug-in-convmatrix.html for a list of example matrices
// Includes blur matrix by default

#ifndef CONVOLUTION_H_
#define CONVOLUTION_H_

//creates convolution effects based on a 3x3 matrix
const char convolutionShader[] =
	"#version 120\n"
	"uniform sampler2D texture;"
	//this is a blur matrix but we can plug in other matrices for other effects
	"const float matrix[9] = float[9](1.0, 2.0, 1.0, 2.0, 4.0, 2.0, 1.0, 2.0, 1.0);"
	"const float weight = 1.0 / 9.0;"
	"vec2 pixel = vec2(1.0 / 512.0, 1.0 / 320.0);" //size of texture shader is applied to

	"void main(void)"
	"{"
		"vec4 colour = vec4(0.0);"
		"vec2 coord = gl_TexCoord[0].xy;"
		"vec2 offset = pixel * 1.5;"
		"vec2 start = coord - offset;"
		"vec2 current = start;"

		"for(int i = 0; i < 9; i++)"
		"{"
			"colour += texture2D(texture, current) * matrix[i];"
			"current.x += pixel.x;"
			"if(i == 2 || i == 5)"
			"{"
				"current.x == start.x;"
				"current.y += pixel.y;"
			"}"
		"}"
		"gl_FragColor = colour * weight;"
	"}";

#endif