//masks off and outputs areas marked as self illum///
#ifndef SELF_ILLUM
#define SELF_ILLUM

const char sillumVert[] = 
	"#version 120\n"

	"void main()"
	"{"
		"gl_Position = ftransform();"
		"gl_TexCoord[0] = (gl_TextureMatrix[0] * gl_MultiTexCoord0);"

	"}";

const char sillumFrag[] = 
	"#version 120\n"

	"uniform sampler2D colour;"
	"uniform sampler2D mask;"

	"void main()"
	"{"
		  "vec2 uv = gl_TexCoord[0].st;"
		  "vec4 base = texture2D(colour, uv);"
		  "float maskValue = texture2D(mask, uv).b;"
		  "gl_FragColor = vec4(base.rgb * maskValue, base.a);"
	"}";

#endif //SELF_ILLUM