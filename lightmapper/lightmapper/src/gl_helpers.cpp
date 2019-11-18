#include "gl_helpers.h"
#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

//int loadSimpleObjFile(const char* filename, vertex_t** vertices, unsigned int* vertexCount, unsigned short** indices, unsigned int* indexCount)
//{
//    FILE* file = fopen(filename, "rt");
//    if (!file)
//        return 0;
//    char line[1024];
//
//    // first pass
//    unsigned int np = 0, nn = 0, nt = 0, nf = 0;
//    while (!feof(file))
//    {
//        fgets(line, 1024, file);
//        if (line[0] == '#') continue;
//        if (line[0] == 'v')
//        {
//            if (line[1] == ' ') { np++; continue; }
//            if (line[1] == 'n') { nn++; continue; }
//            if (line[1] == 't') { nt++; continue; }
//            assert(!"unknown vertex attribute");
//        }
//        if (line[0] == 'f') { nf++; continue; }
//        assert(!"unknown identifier");
//    }
//    assert(np && np == nn && np == nt && nf); // only supports obj files without separately indexed vertex attributes
//
//    // allocate memory
//    *vertexCount = np;
//    *vertices = (vertex_t*)calloc(np, sizeof(vertex_t));
//    *indexCount = nf * 3;
//    *indices = (unsigned short*)calloc(nf * 3, sizeof(unsigned short));
//
//    // second pass
//    fseek(file, 0, SEEK_SET);
//    unsigned int cp = 0, cn = 0, ct = 0, cf = 0;
//    while (!feof(file))
//    {
//        fgets(line, 1024, file);
//        if (line[0] == '#') continue;
//        if (line[0] == 'v')
//        {
//            if (line[1] == ' ') { float* p = (*vertices)[cp++].position; char* e1, * e2; p[0] = (float)strtod(line + 2, &e1); p[1] = (float)strtod(e1, &e2); p[2] = (float)strtod(e2, 0); continue; }
//            if (line[1] == 'n') { /*float *n = (*vertices)[cn++].n; char *e1, *e2; n[0] = (float)strtod(line + 3, &e1); n[1] = (float)strtod(e1, &e2); n[2] = (float)strtod(e2, 0);*/ continue; } // no normals needed
//            if (line[1] == 't') { float* t = (*vertices)[ct++].texCoord; char* e1;      t[0] = (float)strtod(line + 3, &e1); t[1] = (float)strtod(e1, 0);                                continue; }
//            assert(!"unknown vertex attribute");
//        }
//        if (line[0] == 'f')
//        {
//            unsigned short* tri = (*indices) + cf;
//            cf += 3;
//            char* e1, * e2, * e3 = line + 1;
//            for (int i = 0; i < 3; i++)
//            {
//                unsigned long pi = strtoul(e3 + 1, &e1, 10);
//                assert(e1[0] == '/');
//                unsigned long ti = strtoul(e1 + 1, &e2, 10);
//                assert(e2[0] == '/');
//                unsigned long ni = strtoul(e2 + 1, &e3, 10);
//                assert(pi == ti && pi == ni);
//                tri[i] = (unsigned short)(pi - 1);
//            }
//            continue;
//        }
//        assert(!"unknown identifier");
//    }
//
//    fclose(file);
//    return 1;
//}

GLuint loadShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    if (shader == 0)
    {
        fprintf(stderr, "Could not create shader!\n");
        return 0;
    }
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        fprintf(stderr, "Could not compile shader!\n");
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen)
        {
            char* infoLog = (char*)malloc(infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            fprintf(stderr, "%s\n", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}
GLuint loadProgram(const char* vp, const char* fp, const char** attributes, int attributeCount)
{
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vp);
    if (!vertexShader)
        return 0;
    GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fp);
    if (!fragmentShader)
    {
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program == 0)
    {
        fprintf(stderr, "Could not create program!\n");
        return 0;
    }
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    for (int i = 0; i < attributeCount; i++)
        glBindAttribLocation(program, i, attributes[i]);

    glLinkProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        fprintf(stderr, "Could not link program!\n");
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen)
        {
            char* infoLog = (char*)malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(program, infoLen, NULL, infoLog);
            fprintf(stderr, "%s\n", infoLog);
            free(infoLog);
        }
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

glm::mat4 fpsCameraViewMatrix(GLFWwindow* window)
{
    // initial camera config
    static glm::vec3 position(0.f, 0.3f, 1.5f);
    static glm::vec2 rotation(0.f);

    glm::mat4 rotMat = glm::rotate(glm::mat4(1.f), rotation.y * static_cast<float>(M_PI / 180.f), glm::vec3(0.f, 1.f, 0.f));
    rotMat = glm::rotate(rotMat, rotation.x * static_cast<float>(M_PI / 180.f), glm::vec3(1.f, 0.f, 0.f));

    if (!ImGui::GetIO().WantCaptureKeyboard
        && !ImGui::GetIO().WantCaptureMouse)
    {
        // mouse look
        static double lastMouse[] = { 0.0, 0.0 };
        double mouse[2];
        glfwGetCursorPos(window, &mouse[0], &mouse[1]);
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            rotation[0] += (float)(mouse[1] - lastMouse[1]) * -0.2f;
            rotation[1] += (float)(mouse[0] - lastMouse[0]) * -0.2f;
        }
        lastMouse[0] = mouse[0];
        lastMouse[1] = mouse[1];



        // keyboard movement (WSADEQ)
        float speed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 0.1f : 0.01f;
        glm::vec3 movement(0.f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement[2] -= speed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement[2] += speed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement[0] -= speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement[0] += speed;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) movement[1] -= speed;
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) movement[1] += speed;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);

        position += glm::vec3(rotMat * glm::vec4(movement, 1.f));
    }

    // construct view matrix
    return glm::inverse(glm::translate(glm::mat4(1.f), position) * rotMat);
}