#include "gl_helpers.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

int loadSimpleObjFile(const char* filename, vertex_t** vertices, unsigned int* vertexCount, unsigned short** indices, unsigned int* indexCount)
{
    FILE* file = fopen(filename, "rt");
    if (!file)
        return 0;
    char line[1024];

    // first pass
    unsigned int np = 0, nn = 0, nt = 0, nf = 0;
    while (!feof(file))
    {
        fgets(line, 1024, file);
        if (line[0] == '#') continue;
        if (line[0] == 'v')
        {
            if (line[1] == ' ') { np++; continue; }
            if (line[1] == 'n') { nn++; continue; }
            if (line[1] == 't') { nt++; continue; }
            assert(!"unknown vertex attribute");
        }
        if (line[0] == 'f') { nf++; continue; }
        assert(!"unknown identifier");
    }
    assert(np && np == nn && np == nt && nf); // only supports obj files without separately indexed vertex attributes

    // allocate memory
    *vertexCount = np;
    *vertices = calloc(np, sizeof(vertex_t));
    *indexCount = nf * 3;
    *indices = calloc(nf * 3, sizeof(unsigned short));

    // second pass
    fseek(file, 0, SEEK_SET);
    unsigned int cp = 0, cn = 0, ct = 0, cf = 0;
    while (!feof(file))
    {
        fgets(line, 1024, file);
        if (line[0] == '#') continue;
        if (line[0] == 'v')
        {
            if (line[1] == ' ') { float* p = (*vertices)[cp++].p; char* e1, * e2; p[0] = (float)strtod(line + 2, &e1); p[1] = (float)strtod(e1, &e2); p[2] = (float)strtod(e2, 0); continue; }
            if (line[1] == 'n') { /*float *n = (*vertices)[cn++].n; char *e1, *e2; n[0] = (float)strtod(line + 3, &e1); n[1] = (float)strtod(e1, &e2); n[2] = (float)strtod(e2, 0);*/ continue; } // no normals needed
            if (line[1] == 't') { float* t = (*vertices)[ct++].t; char* e1;      t[0] = (float)strtod(line + 3, &e1); t[1] = (float)strtod(e1, 0);                                continue; }
            assert(!"unknown vertex attribute");
        }
        if (line[0] == 'f')
        {
            unsigned short* tri = (*indices) + cf;
            cf += 3;
            char* e1, * e2, * e3 = line + 1;
            for (int i = 0; i < 3; i++)
            {
                unsigned long pi = strtoul(e3 + 1, &e1, 10);
                assert(e1[0] == '/');
                unsigned long ti = strtoul(e1 + 1, &e2, 10);
                assert(e2[0] == '/');
                unsigned long ni = strtoul(e2 + 1, &e3, 10);
                assert(pi == ti && pi == ni);
                tri[i] = (unsigned short)(pi - 1);
            }
            continue;
        }
        assert(!"unknown identifier");
    }

    fclose(file);
    return 1;
}

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

 void multiplyMatrices(float* out, float* a, float* b)
{
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 4; x++)
            out[y * 4 + x] = a[x] * b[y * 4] + a[4 + x] * b[y * 4 + 1] + a[8 + x] * b[y * 4 + 2] + a[12 + x] * b[y * 4 + 3];
}
void translationMatrix(float* out, float x, float y, float z)
{
    out[0] = 1.0f; out[1] = 0.0f; out[2] = 0.0f; out[3] = 0.0f;
    out[4] = 0.0f; out[5] = 1.0f; out[6] = 0.0f; out[7] = 0.0f;
    out[8] = 0.0f; out[9] = 0.0f; out[10] = 1.0f; out[11] = 0.0f;
    out[12] = x;    out[13] = y;    out[14] = z;    out[15] = 1.0f;
}
void rotationMatrix(float* out, float angle, float x, float y, float z)
{
    angle *= (float)M_PI / 180.0f;
    float c = cosf(angle), s = sinf(angle), c2 = 1.0f - c;
    out[0] = x * x * c2 + c;   out[1] = y * x * c2 + z * s; out[2] = x * z * c2 - y * s; out[3] = 0.0f;
    out[4] = x * y * c2 - z * s; out[5] = y * y * c2 + c;   out[6] = y * z * c2 + x * s; out[7] = 0.0f;
    out[8] = x * z * c2 + y * s; out[9] = y * z * c2 - x * s; out[10] = z * z * c2 + c;   out[11] = 0.0f;
    out[12] = 0.0f;         out[13] = 0.0f;         out[14] = 0.0f;         out[15] = 1.0f;
}
void transformPosition(float* out, float* m, float* p)
{
    float d = 1.0f / (m[3] * p[0] + m[7] * p[1] + m[11] * p[2] + m[15]);
    out[2] = d * (m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14]);
    out[1] = d * (m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13]);
    out[0] = d * (m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12]);
}
void transposeMatrix(float* out, float* m)
{
    out[0] = m[0]; out[1] = m[4]; out[2] = m[8]; out[3] = m[12];
    out[4] = m[1]; out[5] = m[5]; out[6] = m[9]; out[7] = m[13];
    out[8] = m[2]; out[9] = m[6]; out[10] = m[10]; out[11] = m[14];
    out[12] = m[3]; out[13] = m[7]; out[14] = m[11]; out[15] = m[15];
}
void perspectiveMatrix(float* out, float fovy, float aspect, float zNear, float zFar)
{
    float f = 1.0f / tanf(fovy * (float)M_PI / 360.0f);
    float izFN = 1.0f / (zNear - zFar);
    out[0] = f / aspect; out[1] = 0.0f; out[2] = 0.0f;                       out[3] = 0.0f;
    out[4] = 0.0f;       out[5] = f;    out[6] = 0.0f;                       out[7] = 0.0f;
    out[8] = 0.0f;       out[9] = 0.0f; out[10] = (zFar + zNear) * izFN;      out[11] = -1.0f;
    out[12] = 0.0f;       out[13] = 0.0f; out[14] = 2.0f * zFar * zNear * izFN; out[15] = 0.0f;
}
void fpsCameraViewMatrix(GLFWwindow* window, float* view)
{
    // initial camera config
    static float position[] = { 0.0f, 0.3f, 1.5f };
    static float rotation[] = { 0.0f, 0.0f };

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

    float rotationY[16], rotationX[16], rotationYX[16];
    rotationMatrix(rotationX, rotation[0], 1.0f, 0.0f, 0.0f);
    rotationMatrix(rotationY, rotation[1], 0.0f, 1.0f, 0.0f);
    multiplyMatrices(rotationYX, rotationY, rotationX);

    // keyboard movement (WSADEQ)
    float speed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 0.1f : 0.01f;
    float movement[3] = { 0 };
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement[2] -= speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement[2] += speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement[0] -= speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement[0] += speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) movement[1] -= speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) movement[1] += speed;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);

    float worldMovement[3];
    transformPosition(worldMovement, rotationYX, movement);
    position[0] += worldMovement[0];
    position[1] += worldMovement[1];
    position[2] += worldMovement[2];

    // construct view matrix
    float inverseRotation[16], inverseTranslation[16];
    transposeMatrix(inverseRotation, rotationYX);
    translationMatrix(inverseTranslation, -position[0], -position[1], -position[2]);
    multiplyMatrices(view, inverseRotation, inverseTranslation); // = inverse(translation(position) * rotationYX);
}