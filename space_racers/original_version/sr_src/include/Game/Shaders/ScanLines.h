///creates animatable scanlines across an image///
#ifndef SCANLINES_H_
#define SCANLINES_H_

const char scanlines[] = 
	"#version 120\n" 

	"uniform sampler2D baseTexture;"
	"uniform float time;"
	"const float strength = 0.4;"
	"const float lineCount = 120.0;" //number of scanlines up to max vertical res

	"const float cosTime = 19.5;" //scroll frequency in hertz

	"void main()"
	"{"		
		"vec2 coord = gl_TexCoord[0].xy;"
		"vec4 basePixel = texture2D(baseTexture, coord);"
		"if(basePixel.a < 0.2) discard;"

		//create scanlines with sin/cos
		"vec2 sinCos = vec2(sin(coord.y * lineCount), cos(coord.y * lineCount + time * cosTime));"
		"vec3 result = basePixel.rgb * vec3(sinCos.x, sinCos.y, sinCos.x) * strength;"

		//blend result with original texture
		"basePixel.rgb += clamp(strength, 0.0, 1.0) * (result/* - basePixel.rgb*/);"
		"basePixel.rg /= 2.0;"

		"gl_FragColor = basePixel;"
	"}";

const char ghost[] = 
	"#version 120\n"

	"uniform sampler2D texture;"
	"uniform vec2 ghostOffset = vec2(0.025);"
	"const float transparency = 0.4;" //ghost transparency

	"void main()"
	"{"

		"vec4 colour = texture2D(texture, gl_TexCoord[0].xy);"
		"vec4 ghost = texture2D(texture, gl_TexCoord[0].xy + ghostOffset);"
		"ghost.a *= transparency * gl_Color.a;"

		"colour = mix(colour, ghost, transparency);"
		"colour.a *= gl_Color.a;"
		"gl_FragColor = colour;"

	"}";

#endif