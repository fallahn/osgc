///spherically distorts and crops a tiled map to resemble a planet
#ifndef PLANET_MAP_H_
#define PLANET_MAP_H_

const char planetMap[] = 
	"#version 120\n"

	"uniform sampler2D baseTexture;"
	"uniform sampler2D normalTexture;"
	"uniform sampler2D globeNormalTexture;"
	"uniform float radius = 0.249;" //normalised text coord - so 0.5 would be half texture width
	"uniform vec2 position = vec2(0.0);" //in texture coords

	"const vec2 centre = vec2(0.25);"
	"uniform vec3 lightPos = vec3(2.8, 0.2, 1.4);"
	"const vec3 lightSpecColour = vec3(0.24, 0.25, 0.27);"
	"const float lightSpecPower = 32.0;" //smaller is less glossy

	"void main(void)"
	"{"		
		//globe normal
		"vec3 globeNormalColour = texture2D(globeNormalTexture, gl_TexCoord[0].xy * 2.0).rgb;"
		"globeNormalColour.g = 1.0 - globeNormalColour.g;"

		//offset coord for distortion
		"vec2 offset = position;"
		"offset.y = 1.0 - position.y;"
		"vec2 coord = gl_TexCoord[0].xy + offset;"

		"coord += globeNormalColour.rg / 4.0;"
		"coord += (centre + offset);"
		
		//get pixels at coord
		"vec3 colour = texture2D(baseTexture, coord).rgb;"
		"vec3 normalColour = texture2D(normalTexture, coord).rgb;"

		//calc normal / spec / shading
		//"normalColour.g = 1.0 - normalColour.g;" //invert G
		"vec3 normal = normalColour * 2.0 - 1.0;"
		"vec3 globeNormal = globeNormalColour * 2.0 - 1.0;"
		"normal = normalize(normal + globeNormal);"
		

		//calculate the colour intensity based on normal and add specular to pixels facing light
		"vec3 lightDir = normalize(lightPos);"
		"float colourIntensity = max(dot(normal, lightDir), 0.0);"
		//"colourIntensity = clamp(colourIntensity * 2.5, 0.0, 1.0);"
		"vec3 specular = vec3(0.0);"
		"vec3 diffuse = vec3(0.0);"
		"if(colourIntensity > 0.0)"
		"{"
			//vector half way between light and view direction
			"vec3 halfVec = normalize(lightDir + vec3(0.5));" //fixed 2D view, so view is centred
			"float specModifier = max(dot(normal, halfVec), 0.0);"
			"specular = pow(specModifier, lightSpecPower) * lightSpecColour;"
			//"specular /= 1.5;"
			"diffuse = colour * colourIntensity;" // TODO multiply by distance to light
		"}"

		"float alpha;" //calc alpha to make texture appear circular
		"float distance = distance(centre, gl_TexCoord[0].xy);"
		"if(distance > radius) alpha = 0.0;"
		"else alpha = 1.0;"

		//output
		"gl_FragColor = vec4(clamp(diffuse + specular, 0.0, 0.8), alpha);"
	"}";


#endif //PLANET_MAP_H_