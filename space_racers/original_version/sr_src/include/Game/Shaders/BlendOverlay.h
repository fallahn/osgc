//performs overlay blending a la photoshop//
#ifndef BLEND_OVERLAY_H_
#define BLEND_OVERLAY_H_

const char vertForward[] = 
	"void main()"
	"{"
		// transform the vertex position
	   "gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"

		// transform the texture coordinates
		"gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;"

		// forward the vertex color
		"gl_FrontColor = gl_Color;"
	"}";

const char blendOverlay[] = 
	"uniform sampler2D baseMap;"
	"uniform vec4 colour = vec4(1.0, 0.0, 0.0, 1.0);"

	//recreates the 'overlay' blend mode from photoshop
	"float BlendOverlay(float base, float blend)"
	"{"
		"return (base < 0.5) ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend));"
	"}"

	"void main()"
	"{"
		//blend colour with neon map
		"vec4 base = texture2D(baseMap, gl_TexCoord[0].xy);"
		"vec3 result = vec3(BlendOverlay(base.r, colour.r),"
								"BlendOverlay(base.g, colour.g),"
								"BlendOverlay(base.b, colour.b));"

		"gl_FragColor = vec4(result, gl_Color.a);"
	"}";

#endif //BLEND_OVERLAY_H_