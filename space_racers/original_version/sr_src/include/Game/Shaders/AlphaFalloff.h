//creates a gradient in transparency from the centre of the given texture//
#ifndef FALLOFF_H_
#define FALLOFF_H_

const char falloff[] = 
	"#version 120\n"

	"uniform float radius;" //distance before alpha reaches 0
	"uniform vec4 colour;" //fill colour
	"uniform vec2 position;"
	//"uniform float alphaStrength = 1.0;" //so we can reduce overall transparency
	"uniform float verticalResolution;" //so we can flip Y coords

	"void main(void)"
	"{"
		"vec2 centre = position;"
		"centre.y = verticalResolution - centre.y;"
		"float distance = distance(centre, gl_FragCoord.xy);"
		"float alpha = 1.0 - min(distance, radius) / radius;"

		"gl_FragColor = vec4(colour.rgb, alpha);"/* * alphaStrength*/
	"}";

#endif