///extract and blend shaders for bloom post///
#ifndef BLOOM_SHADER_H_
#define BLOOM_SHADER_H_

const char* bloomBlend = 
	"#version 120\n"
	"uniform sampler2D baseTexture;"
	"uniform sampler2D glowTexture;"

	"uniform float bloomIntensity = 1.8;"
	"uniform float baseIntensity = 1.2;"

	"uniform float bloomSaturation = 2.8;"
	"uniform float baseSaturation = 2.5;"

	"vec4 AdjustSaturation(vec4 colour, float saturation)"
	"{"
		"float grey = dot(colour.rgb, vec3(0.299, 0.587, 0.114));"
		"return vec4(mix(vec3(grey), colour.rgb, saturation), 1.0);"
	"}"

	"void main()"
	"{"
		"vec4 base = texture2D(baseTexture, vec2(gl_TexCoord[0]));"
		"vec4 bloom = texture2D(glowTexture, vec2(gl_TexCoord[0]));"

		"base = AdjustSaturation(base, baseSaturation) * baseIntensity;"
		"bloom = AdjustSaturation(bloom, bloomSaturation) * bloomIntensity;"
	
		"base *= (1 - clamp(bloom, 0.0, 1.0)); "

		"gl_FragColor = base + bloom;"
	"}";


const char* bloomExtract = 
	"#version 120\n"
	"uniform sampler2D texture;"
	"uniform float threshold = 0.27;"
	"void main()"
	"{"	
		"vec4 tc = texture2D(texture, vec2(gl_TexCoord[0]));"
		"gl_FragColor = clamp((tc - threshold) / (1 - threshold), 0.0, 1.0);"
	"}";

#endif //BLOOM_SHADER_H_