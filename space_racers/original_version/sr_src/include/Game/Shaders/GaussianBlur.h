//2 stage gaussian blur filter//
#ifndef GAUSSIAN_BLUR_H_
#define GAUSSIAN_BLUR_H_

const char gaussianBlur[] = 
	"#version 120\n"
	"uniform vec2 resolution = vec2(1.0 / 1440.0, 1.0 / 1080.0);" //normalised texture size
	"uniform sampler2D texture;"
	"uniform float orientation = 0.0;"
	"uniform int blurAmount = 10;" //could be 10?
	"uniform float blurScale = 1.0;"
	"uniform float blurStrength = 0.25;"

	"float Gaussian(float x, float deviation)"
	"{"
		"return (1.0 / sqrt(2.0 * 3.141592 * deviation)) * exp(-((x * x) / (2.0 * deviation)));"
	"}"

	"void main ()"
	"{" 
		//Locals
		"float halfBlur = float(blurAmount) * 0.5;"
		"vec4 colour = vec4(0.0);"
		"vec4 texColour = vec4(0.0);"
		//Gaussian deviation
		"float deviation = halfBlur * 0.35;"
		"deviation *= deviation;"
		"float strength = 1.0 - blurStrength;"
		"if(orientation == 0)"
		"{"
			// Horizontal blur 
			"for(int i = 0; i < 10; ++i)"
			"{" 
				"if(i >= blurAmount) break;"
				"float offset = float(i) - halfBlur;"
				"texColour = texture2D(texture, gl_TexCoord[0].xy + vec2(offset * resolution.x * blurScale, 0.0)) * Gaussian(offset * strength, deviation);"
				"colour += texColour;"
			"}" 
		"}"
		"else"
		"{" 
			// Vertical blur
			"for(int i = 0; i < 10; ++i)"
			"{" 
				"if (i >= blurAmount) break;"
				"float offset = float(i) - halfBlur;"
				"texColour = texture2D(texture, gl_TexCoord[0].xy + vec2(0.0, offset * resolution.y * blurScale)) * Gaussian(offset * strength, deviation);"
				"colour += texColour;"
			"}"
		"} "
		// Apply colour
		"gl_FragColor = clamp(colour, 0.0, 1.0);"
		"gl_FragColor.w = 1.0;"
	"}";


const char gaussianBlur2[] = 
	"#version 120\n"
	"uniform sampler2D texture;"
	"uniform vec2 resolution = vec2(180.0, 135.0);"
	"uniform int orientation = 0;"

	"float offset[3] = float[3]( 0.0, 1.3846153846, 3.2307692308 );"
	"float weight[3] = float[3]( 0.2270270270, 0.3162162162, 0.0702702703 );"

	"void main()"
	"{"
		"vec4 tc = vec4(1.0, 0.0, 0.0, 0.0);"

		"vec2 uv = gl_TexCoord[0].xy;"
		"tc = texture2D(texture, uv) * weight[0];"

		"if(orientation == 0)"
		"{"
			"for(int i = 1; i < 3; i++)"
			"{"
				"tc += texture2D(texture, uv + vec2(offset[i]) / resolution.x, 0.0) * weight[i];"
				"tc += texture2D(texture, uv - vec2(offset[i]) / resolution.x, 0.0) * weight[i];"
			"}"
		"}"
		"else"
		"{"
			"for(int i = 1; i < 3; i++)"
			"{"
				"tc += texture2D(texture, uv + vec2(0.0, offset[i]) / resolution.y) * weight[i];"
				"tc += texture2D(texture, uv - vec2(0.0, offset[i]) / resolution.y) * weight[i];"
			"}"
		"}"

		"gl_FragColor = tc;"
	"}";

#endif GAUSSIAN_BLUR_H_