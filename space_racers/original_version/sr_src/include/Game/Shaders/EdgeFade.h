///fades edges of texture to transparent///
#ifndef EDGE_FADE_H_
#define EDGE_FADE_H_

const char edgeFade[] = 
	"#version 120\n"
	"uniform sampler2D texture;"
	"uniform float texWidth = 1120.0;" //overall width of texture
	"uniform float width = 160.0;" //width of texture to fade (ie sub area if using sprite subrect)
	"uniform float offset = 0.0;" //pixels from edge of texture to first fade
	"uniform float fadeDist = 30.f;" //how many pixels from the edge to go from transparent to opaque

	"void main()"
	"{"
		"vec4 colour = texture2D(texture, gl_TexCoord[0].xy);"
		"const float start = offset / texWidth;"
		"const float end = (offset + (width - fadeDist)) / texWidth;"
		"const float step = 1.0 / (fadeDist / texWidth);"
		//fade in
		"colour.a *= clamp((gl_TexCoord[0].x - start) * step, 0.0, 1.0);"
		//fade out
		"colour.a *= 1.0 - clamp((gl_TexCoord[0].x - end) * step, 0.0, 1.0);"
		"gl_FragColor = colour;"
	"}";
#endif