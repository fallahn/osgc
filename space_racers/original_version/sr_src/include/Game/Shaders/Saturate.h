//saturation shader
#ifndef SATURATION_H_
#define SATURATION_H_

const char* saturate = 
	"#version 120\n"
	"uniform sampler2D texture;"
	//"uniform sampler2D alphaMask;"
	"uniform float amount = 1.8;"
	"uniform float intensity = 2.2;"

	"void main()"
	"{"
		"vec4 colour = texture2D(texture, gl_TexCoord[0].xy);"
		"float grey = dot(colour.rgb, vec3(0.299, 0.587, 0.114));"

		//"colour.a *= 1.0 - texture2D(alphaMask, gl_TexCoord[0]).a;"

		"gl_FragColor = vec4(mix(vec3(grey), colour.rgb, amount), colour.a) * intensity;"
	"}";

#endif //SATURATION_H_