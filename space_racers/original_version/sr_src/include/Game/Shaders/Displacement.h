///displacement shader using either a DuDv or rg channels of normal map///
#ifndef DISPLACEMENT_H_
#define DISPLACEMENT_H_

const char displacment[] = 
	"#version 120\n"
	"uniform sampler2D baseTexture;"
	"uniform sampler2D displaceTexture;"
	"uniform sampler2D alphaMask;"
	"const float strength = 0.2;" //distortion strength

	"void main(void)"
	"{"
		//displace with dudv map
		"vec2 displacement = texture2D(displaceTexture, gl_TexCoord[0].xy).rg * 2.0 - 1.0;"
		"displacement *= strength;"
		"vec4 colour = texture2D(baseTexture, gl_TexCoord[0].xy + displacement.xy);"
		"colour.a *= 1.0 - texture2D(alphaMask, gl_TexCoord[0].xy).a;"
		"gl_FragColor = colour;"
	"}";

#endif //DISPLACEMENT_H_