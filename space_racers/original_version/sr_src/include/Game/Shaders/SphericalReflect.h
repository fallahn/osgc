
#ifndef SPHERICAL_REFLECT_H_
#define SPHERICAL_REFLECT_H_

const char sphericalReflect[] = 
	"#version 120\n"

	"uniform sampler2D baseTexture;"
	"uniform sampler2D reflectTexture;"
	"uniform float radius;" //normalised text coord - so 0.5 would be half texture width
	"uniform vec2 position;" //in texture coords
	"const vec2 centre = vec2(0.5);"

	"void main(void)"
	"{"
		"vec4 colour = texture2D(baseTexture, gl_TexCoord[0].xy);"
		
		"float alpha;" //calc alpha of reflect map
		"float distance = distance(centre, gl_TexCoord[0].xy);"
		"if(distance > radius) alpha = 0.0;"
		"else alpha = 1.0;"
		
		//calc blend val based on distance from centre
		"float blend = min(distance, radius) / radius;"
		"blend *= 0.4;"//reduce blend amount
		
		//offset coord for distortion
		"vec2 offset = position;"
		"offset.y = 1.0 - position.y;"
		"vec2 coord = gl_TexCoord[0].xy + offset;"
		"coord -= centre;"
		//"float percent = 1.0 + ((0.5 - distance) / 0.5) * 1.5;" //pinch
		"float percent = 1.0 - ((radius - distance) / radius);" //bulge
		"coord *= percent;"
		"coord += centre;"
		"vec3 reflectColour = texture2D(reflectTexture, coord);"

		//mix and output
		"gl_FragColor = mix(colour, vec4(reflectColour, alpha), blend);"
		//"gl_FragColor = vec4(reflectColour, alpha);"
	"}";

#endif