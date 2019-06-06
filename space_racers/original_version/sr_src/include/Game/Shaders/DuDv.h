//***Adds a sine wave distortion and modulates a DuDv map***//
#ifndef DUDV_H_
#define DUDV_H_

const char dudvShader[] = //adds sine wave and dudv based distortion for water effects
	"#version 120\n"
	"uniform sampler2D baseTexture;"
	"uniform sampler2D displaceTexture;"
	"uniform float time = 1.0;" //time in seconds
	"uniform float offset;" //frag coords start at bottom, centred pixel

	"void main(void)"
	"{"
		//sin distort for ripples
		"const float sinWidth = 0.04;" //smaller is wider const these if not modding x coord
		"const float sinHeight = 0.001;" //larger is taller
		"const float sinTime = 9.5;" //larger is faster (if time is input in seconds then this value hertz)
		"vec2 coord = gl_TexCoord[0].xy;"
		"coord.y += sin((gl_FragCoord.x + offset) * sinWidth + time * sinTime) * sinHeight;"

		//displace with dudv map
		"const float strength = 0.05;" //distortion strength
		"vec2 displacement = texture2D(displaceTexture, gl_TexCoord[0].xy).rg * 2.0 - 1.0;"
		"displacement *= strength;"
		"gl_FragColor = texture2D(baseTexture, coord + displacement.xy);"
	"}";

#endif