///neon blend shader using overlay type blending//
#ifndef NEON_BLEND_H_
#define NEON_BLEND_H_

const char neonShader[] = 
	"uniform sampler2D diffuseMap;"
	"uniform sampler2D neonMap;"
	"uniform vec4 neonColour = vec4(1.0, 0.0, 0.0, 1.0);"

	//recreates the 'overlay' blend mode from photoshop
	"float BlendOverlay(float base, float blend)"
	"{"
		"return (base < 0.5) ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend));"
	"}"

	"void main()"
	"{"
		//blend colour with neon map
		"vec4 neonBase = texture2D(neonMap, gl_TexCoord[0].xy);"
		"vec4 neon = vec4(vec3(BlendOverlay(neonBase.r, neonColour.r),"
								"BlendOverlay(neonBase.g, neonColour.g),"
								"BlendOverlay(neonBase.b, neonColour.b)),"
								"dot(neonBase.rgb, vec3(0.299, 0.587, 0.114)));"

		//mix neon map with diffuse colour
		"vec4 diffuseColour = texture2D(diffuseMap, gl_TexCoord[0].xy);"
		"gl_FragColor = mix(diffuseColour, neon, neon.a);"
	"}";


#endif