///spherically distorts and crops a tiled map to resemble an asteroid - as planet shader only with updated coord system
#ifndef ROIDS_H_
#define ROIDS_H_

const char shadow[] = 
	"#version 120\n"
	"const float minRadius = 0.3;"
	"const float maxRadius = 0.5;"
	"const vec2 centre = vec2(0.5);"

	"void main()"
	"{"
		"float alpha = 0.5;" //default value
		"float distance = distance(centre, gl_TexCoord[0].xy);"
		"const float step = 1.0 / (maxRadius - minRadius);" //fade edge
		"alpha *= 1.0 - clamp((distance - minRadius) * step, 0.0, 1.0);"

		"gl_FragColor = vec4(vec3(0.0), alpha);"
	"}";

const char roidMap[] =
"#version 120\n"

"uniform sampler2D baseTexture;"
"uniform sampler2D normalTexture;"
"uniform sampler2D globeNormalTexture;"
"uniform float radius = 0.5;" //normalised text coord - so 0.5 would be half texture width
"uniform vec2 texRes = vec2(800.0, 600);" //should be window size
"uniform vec2 camPos = vec2(720.0);"

"const vec2 centre = vec2(0.5);"
"uniform vec2 lightPos = vec2(1000.0);" //world position scaled by view
"const vec3 lightSpecColour = vec3(0.28, 0.22, 0.20);"
"const float lightSpecPower = 32.0;" //larger is more glossy

"void main(void)"
"{"
//globe normal
"	vec3 globeNormalColour = texture2D(globeNormalTexture, gl_TexCoord[0].xy).rgb;"
"	globeNormalColour.g = 1.0 - globeNormalColour.g;"

//offset coord for distortion
"	vec2 offset = gl_FragCoord.xy / texRes;" //should be (gl_FragCoord + scaledPosition) / texRes
"	offset.y = 1.0 - offset.y;"
"	vec2 coord = gl_TexCoord[0].xy + offset;"

"	coord += globeNormalColour.rg / 4.0;"
"	coord += (centre + offset);"

//get pixels at coord
"	vec3 colour = texture2D(baseTexture, coord).rgb;"
"	vec3 normalColour = texture2D(normalTexture, coord).rgb;"

//calc normal / spec / shading
//"normalColour.g = 1.0 - normalColour.g;" //invert G
"	vec3 normal = normalColour * 2.0 - 1.0;"
"	vec3 globeNormal = globeNormalColour * 2.0 - 1.0;"
"	normal = normalize(normal + globeNormal);"


//calculate the colour intensity based on normal and add specular to pixels facing light
"	vec3 worldLight = vec3((lightPos/* - gl_FragCoord.xy*/) / texRes, 0.7);"

//"worldLight.y = 1.0 - worldLight.y;" //invert light Y 
"	vec3 lightDir = normalize(worldLight);"
"	float colourIntensity = max(dot(normal, lightDir), 0.0);"
"	vec3 specular = vec3(0.0);"
"	vec3 diffuse = vec3(0.0);"
"	if(colourIntensity > 0.0)"
"{"
//vector half way between light and view direction
"	vec3 halfVec = normalize(worldLight + vec3(0.5));" //fixed 2D view, so view is centred (actually this isnt true, may fix this later)
"	float specModifier = max(dot(normal, halfVec), 0.0);"
"	specular = pow(specModifier, lightSpecPower) * lightSpecColour;"
//"specular /= 3.0;" //just to make less glaring
"	diffuse = colour * colourIntensity;"
"}"

"	float alpha = 1.0;" //calc alpha to make texture appear circular
"	float distance = distance(centre, gl_TexCoord[0].xy);"
"	if(distance > radius) alpha = 0.0;"
//"else alpha = 1.0;"	
		
//output
"	gl_FragColor = vec4(clamp(specular + diffuse, 0.0, 0.9), alpha);"

"}";


#endif