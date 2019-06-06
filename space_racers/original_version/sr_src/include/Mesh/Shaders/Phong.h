//phong shader using currently applied material settings
#ifndef PHONG_H_
#define PHONG_H_


const char phongVert[] = 
	"#version 120\n"
	"varying vec3 N;"
	"varying vec3 v;"

	"void main(void)"
	"{"
		"v = vec3(gl_ModelViewMatrix * gl_Vertex);"
		"N = normalize(gl_NormalMatrix * gl_Normal);"
		"gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
	"}";


const char phongFrag[] = 
	"#version 120\n"
	"varying vec3 N;"
	"varying vec3 v;"

	"void main (void)"
	"{"
		"vec4 diffuse;"
		"vec4 spec;"
		"vec4 ambient;"

		"vec3 L = normalize(gl_LightSource[0].position.xyz - v);"
		"vec3 E = normalize(-v);"
		"vec3 R = normalize(reflect(-L,N));"  

   		"ambient = gl_FrontMaterial.ambient;"

  		"diffuse = clamp(gl_FrontMaterial.diffuse * max(dot(N,L), 0.0), 0.0, 1.0);"
   		"spec = clamp(gl_FrontMaterial.specular * pow(max(dot(R,E),0.0),0.3 * gl_FrontMaterial.shininess), 0.0, 1.0);"

		"gl_FragColor = ambient + diffuse + spec;"
	"}";

#endif // PHONG_H_