// Distortion OpenGL Demo by Philip Rideout
// Licensed under the Creative Commons Attribution 3.0 Unported License. 
// http://creativecommons.org/licenses/by/3.0/

#include <stdlib.h>
#include <stdbool.h>
#include "pez.h"
#include "vmath.h"

typedef struct {
    int VertexCount;
    int LineIndexCount;
    int FillIndexCount;
    GLuint LineVao;
    GLuint FillVao;
} MeshPod;

typedef struct {
    Matrix4 Projection;
    Matrix4 Modelview;
    Matrix4 View;
    Matrix4 Model;
} TransformsPod;

struct {
    float Theta;
    GLuint LitProgram;
    GLuint SimpleProgram;
    MeshPod Cylinder;
    TransformsPod Transforms;
} Globals;

typedef struct {
    Vector3 Position;
} Vertex;

static GLuint LoadProgram(const char* vsKey, const char* gsKey, const char* fsKey);
static GLuint CurrentProgram();
static MeshPod CreateCylinder();

#define u(x) glGetUniformLocation(CurrentProgram(), x)
#define a(x) glGetAttribLocation(CurrentProgram(), x)
#define offset(x) ((const GLvoid*)x)

const int Slices = 24;
const int Stacks = 8;

PezConfig PezGetConfig()
{
    PezConfig config;
    config.Title = __FILE__;
    config.Width = 853;
    config.Height = 480;
    config.Multisampling = false;
    config.VerticalSync = true;
    return config;
}

void PezInitialize()
{
    const PezConfig cfg = PezGetConfig();

    // Compile shaders
    Globals.SimpleProgram = LoadProgram("Simple.VS", 0, "Simple.FS");
    Globals.LitProgram = LoadProgram("Lit.VS", "Lit.GS", "Lit.FS");

    // Set up viewport
    float fovy = 16 * TwoPi / 180;
    float aspect = (float) cfg.Width / cfg.Height;
    float zNear = 0.1, zFar = 300;
    Globals.Transforms.Projection = M4MakePerspective(fovy, aspect, zNear, zFar);

    // Create geometry
    Globals.Cylinder = CreateCylinder();

    // Misc Initialization
    Globals.Theta = 0;
    glClearColor(0.9, 0.9, 1.0, 1);
    glLineWidth(1.5);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1,1);
}

void PezUpdate(float seconds)
{
    const float RadiansPerSecond = 0.5f;
    Globals.Theta += seconds * RadiansPerSecond;
    
    // Create the model-view matrix:
    Globals.Transforms.Model = M4MakeRotationY(Globals.Theta);
    Point3 eye = {0, 1, 4};
    Point3 target = {0, 0, 0};
    Vector3 up = {0, 1, 0};
    Globals.Transforms.View = M4MakeLookAt(eye, target, up);
    Globals.Transforms.Modelview = M4Mul(Globals.Transforms.View, Globals.Transforms.Model);
}

void PezRender()
{
    Matrix4 mvp = M4Mul(Globals.Transforms.Projection, Globals.Transforms.Modelview);
    Vector3 LightPosition = {0.5, 0.25, 1.0}; // world space
    Vector3 EyePosition = {0, 0, 1}; // world space
    Matrix3 m = M3Transpose(M4GetUpper3x3(Globals.Transforms.Model));
    Vector3 Lhat = M3MulV3(m, V3Normalize(LightPosition)); // object space
    Vector3 Eye =  M3MulV3(m, V3Normalize(EyePosition)); // object space
    Vector3 Hhat = V3Normalize(V3Add(Lhat, Eye));

    int instanceCount = 1;
    MeshPod* mesh = &Globals.Cylinder;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
    glUseProgram(Globals.LitProgram);
    glUniform3f(u("SpecularMaterial"), 0.4, 0.4, 0.4);
    glUniform4f(u("FrontMaterial"), 0, 0, 1, 1);
    glUniform4f(u("BackMaterial"), 0.5, 0.5, 0, 1);
    glUniform3fv(u("Hhat"), 1, &Hhat.x);
    glUniform3fv(u("Lhat"), 1, &Lhat.x);
    glUniformMatrix4fv(u("ModelviewProjection"), 1, 0, (float*) &mvp);

    glBindVertexArray(mesh->FillVao);
    glDrawElementsInstanced(GL_TRIANGLES, mesh->FillIndexCount, GL_UNSIGNED_SHORT, 0, instanceCount);

    glUseProgram(Globals.SimpleProgram);
    glUniform4f(u("Color"), 0, 0, 0, 1);
    glUniformMatrix4fv(u("ModelviewProjection"), 1, 0, (float*) &mvp);

    glDepthMask(GL_FALSE);
    glBindVertexArray(mesh->LineVao);
    glDrawElementsInstanced(GL_LINES, mesh->LineIndexCount, GL_UNSIGNED_SHORT, 0, instanceCount);
    glDepthMask(GL_TRUE);
}

void PezHandleMouse(int x, int y, int action)
{
}

static GLuint CurrentProgram()
{
    GLuint p;
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*) &p);
    return p;
}

static GLuint LoadProgram(const char* vsKey, const char* gsKey, const char* fsKey)
{
    GLchar spew[256];
    GLint compileSuccess;
    GLuint programHandle = glCreateProgram();

    const char* vsSource = pezGetShader(vsKey);
    pezCheck(vsSource != 0, "Can't find vshader: %s\n", vsKey);
    GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsHandle, 1, &vsSource, 0);
    glCompileShader(vsHandle);
    glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(vsHandle, sizeof(spew), 0, spew);
    pezCheck(compileSuccess, "Can't compile vshader:\n%s", spew);
    glAttachShader(programHandle, vsHandle);

    if (gsKey) {
        const char* gsSource = pezGetShader(gsKey);
        pezCheck(gsSource != 0, "Can't find gshader: %s\n", gsKey);
        GLuint gsHandle = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(gsHandle, 1, &gsSource, 0);
        glCompileShader(gsHandle);
        glGetShaderiv(gsHandle, GL_COMPILE_STATUS, &compileSuccess);
        glGetShaderInfoLog(gsHandle, sizeof(spew), 0, spew);
        pezCheck(compileSuccess, "Can't compile gshader:\n%s", spew);
        glAttachShader(programHandle, gsHandle);
    }

    const char* fsSource = pezGetShader(fsKey);
    pezCheck(fsSource != 0, "Can't find fshader: %s\n", fsKey);
    GLuint fsHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsHandle, 1, &fsSource, 0);
    glCompileShader(fsHandle);
    glGetShaderiv(fsHandle, GL_COMPILE_STATUS, &compileSuccess);
    glGetShaderInfoLog(fsHandle, sizeof(spew), 0, spew);
    pezCheck(compileSuccess, "Can't compile fshader:\n%s", spew);
    glAttachShader(programHandle, fsHandle);

    glLinkProgram(programHandle);
    GLint linkSuccess;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
    glGetProgramInfoLog(programHandle, sizeof(spew), 0, spew);
    pezCheck(linkSuccess, "Can't link shaders:\n%s", spew);
    glUseProgram(programHandle);
    return programHandle;
}

static Vector3 EvaluateCylinder(float s, float t)
{
    Vector3 range;
    range.x = 0.5 * cos(t * TwoPi);
    range.y = s - 0.5;
    range.z = 0.5 * sin(t * TwoPi);
    return range;
}

static MeshPod CreateCylinder()
{
    const int VertexCount = (Slices+1) * (Stacks+1);
    const int FillIndexCount = (Slices+1) * Stacks * 6;

    const int circles = (Stacks+1)*Slices;
    const int longitudinal = Stacks*Slices;
    const int LineIndexCount = 2 * (circles + longitudinal);
 
    MeshPod mesh;
    glGenVertexArrays(1, &mesh.FillVao);
    glBindVertexArray(mesh.FillVao);

    // Create a buffer with positions
    GLuint positionsVbo;
    if (1) {
        Vertex verts[VertexCount];
        Vertex* pVert = &verts[0];
        float ds = 1.0f / Stacks;
        float dt = 1.0f / Slices;

        // The upper bounds in these loops are tweaked to reduce the
        // chance of precision error causing an incorrect # of iterations.
        for (float s = 0; s < 1 + ds / 2; s += ds) {
            for (float t = 0; t < 1 + dt / 2; t += dt) {
                pVert->Position = EvaluateCylinder(s, t);
                ++pVert;
            }
        }

        pezCheck(pVert - &verts[0] == VertexCount, "Tessellation error.");

        GLsizeiptr size = sizeof(verts);
        const GLvoid* data = &verts[0].Position.x;
        GLenum usage = GL_STATIC_DRAW;
        glGenBuffers(1, &positionsVbo);
        glBindBuffer(GL_ARRAY_BUFFER, positionsVbo);
        glBufferData(GL_ARRAY_BUFFER, size, data, usage);
    }

    // Create a buffer of 16-bit triangle indices
    GLuint trianglesVbo;
    if (1) {
        GLushort inds[FillIndexCount];
        GLushort* pIndex = &inds[0];
        GLushort n = 0;
        for (GLushort j = 0; j < Stacks; j++) {
            int vps = Slices+1; // vertices per stack
            for (GLushort i = 0; i < vps; i++) {
                *pIndex++ = (n + i + vps);
                *pIndex++ = n + (i + 1) % vps;
                *pIndex++ = n + i;
                
                *pIndex++ = (n + (i + 1) % vps + vps);
                *pIndex++ = (n + (i + 1) % vps);
                *pIndex++ = (n + i + vps);
            }
            n += vps;
        }

        pezCheck(pIndex - &inds[0] == FillIndexCount, "Tessellation error.");

        GLsizeiptr size = sizeof(inds);
        const GLvoid* data = &inds[0];
        GLenum usage = GL_STATIC_DRAW;
        glGenBuffers(1, &trianglesVbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trianglesVbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
    }

    // Create a buffer of 16-bit line indices
    GLuint lineVbo;
    if (1) {

        GLushort inds[LineIndexCount];
        GLushort* pIndex = &inds[0];

        // Circles:
        GLushort n = 0;
        for (GLushort j = 0; j < Stacks+1; j++) {
            for (GLushort i = 0; i < Slices; i++) {
                *pIndex++ = n + i;
                *pIndex++ = n + i + 1;
            }
            n += Slices + 1;
        }

        // Longitudinal:
        n = 0;
        for (GLushort j = 0; j < Stacks; j++) {
            for (GLushort i = 0; i < Slices; i++) {
                *pIndex++ = n + i;
                *pIndex++ = n + i + (Slices + 1);
            }
            n += Slices + 1;
        }

        pezCheck(pIndex - &inds[0] == LineIndexCount, "Tessellation error.");

        GLsizeiptr size = sizeof(inds);
        const GLvoid* data = &inds[0];
        GLenum usage = GL_STATIC_DRAW;
        glGenBuffers(1, &lineVbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineVbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
    }

    mesh.VertexCount = VertexCount;
    mesh.FillIndexCount = FillIndexCount;
    mesh.LineIndexCount = LineIndexCount;

    glGenVertexArrays(1, &mesh.FillVao);
    glBindVertexArray(mesh.FillVao);
    glBindBuffer(GL_ARRAY_BUFFER, positionsVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trianglesVbo);
    glVertexAttribPointer(a("Position"), 3, GL_FLOAT, GL_FALSE, 12, 0);
    glEnableVertexAttribArray(a("Position"));

    glGenVertexArrays(1, &mesh.LineVao);
    glBindVertexArray(mesh.LineVao);
    glBindBuffer(GL_ARRAY_BUFFER, positionsVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineVbo);
    glVertexAttribPointer(a("Position"), 3, GL_FLOAT, GL_FALSE, 12, 0);
    glEnableVertexAttribArray(a("Position"));

    return mesh;
}
