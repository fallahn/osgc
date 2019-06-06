//***listed as dynamic soft shadow, actually a ray casting blur***//
#ifndef SOFTSHADOW_H_
#define SOFTSHADOW_H_

const char softShadow[] = 
	"#version 120\n"

	"uniform sampler2D texture;"
	"uniform vec2 lightPosition = vec2(0.5, 0.5);"
	"const float intensity = 0.2;"
	"const float decay = 0.98;"//78
	"const float density = 0.95;"
	"const float weight = 0.7;"

	"const int NUM_SAMPLES = 80;"

	"void main()"
	"{"
		"vec2 deltaTextCoord = vec2(gl_TexCoord[0].xy - lightPosition.xy);"
		"vec2 texCoord =  gl_TexCoord[0].xy;"

		"deltaTextCoord *= 1.0 / float(NUM_SAMPLES) * density;"
		"float illuminationDecay = 1.0;"
	
		"for(int i = 0; i < NUM_SAMPLES; i++)"
		"{"
			"texCoord -= deltaTextCoord;"
			"vec4 sample = texture2D(texture, texCoord);"
			//"sample.rgb = vec3(0.1);" //desaturate - skip this if ray casting a light source rather than shadows
			"sample.a *= 0.8;" //increase transparency
			"sample *= illuminationDecay * weight;"
			"gl_FragColor += sample;"
			"illuminationDecay *= decay;"
		"}"
		"gl_FragColor *= intensity;"
	"}";

#endif