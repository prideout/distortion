// Distortion OpenGL Demo by Philip Rideout
// Licensed under the Creative Commons Attribution 3.0 Unported License. 
// http://creativecommons.org/licenses/by/3.0/

#include <stdlib.h>
#include <stdbool.h>
#include "pez.h"
#include "vmath.h"

struct {
    GLuint Position;
    GLuint TexCoord;
} Attr;

typedef struct {
    int VertexCount;
    int LineIndexCount;
    int FillIndexCount;
    GLuint LineVao;
    GLuint FillVao;
} MeshPod;

struct {
    float Theta;
    GLuint LitProgram;
    GLuint SimpleProgram;
    GLuint QuadProgram;
    GLuint GridProgram;
    MeshPod Cylinder;
    Matrix4 Projection;
    Matrix4 View;
    GLuint FboTexture;
    GLuint FboHandle;
    GLuint QuadVao;
    MeshPod Grid;
} Globals;

typedef struct {
    Point3 Position;
    Vector2 TexCoord;
} Vertex;

static GLuint LoadProgram(const char* vsKey, const char* gsKey, const char* fsKey);
static GLuint CurrentProgram();
static MeshPod CreateCylinder();
static GLuint CreateQuad();
static MeshPod CreateGrid(int rows, int cols);
static GLuint CreateRenderTarget(GLuint* colorTexture);

#define u(x) glGetUniformLocation(CurrentProgram(), x)
#define offset(x) ((const GLvoid*)x)

const int Slices = 24;
const int Stacks = 8;
const int GridRows = 20;
const int GridCols = 36;

PezConfig PezGetConfig()
{
    PezConfig config;
    config.Title = __FILE__;
    config.Width = 853*3/2;
    config.Height = 480*3/2;
    config.Multisampling = true;
    config.VerticalSync = true;
    return config;
}

void PezInitialize()
{
    const PezConfig cfg = PezGetConfig();

    // Assign the vertex attributes to integer slots:
    GLuint* pAttr = (GLuint*) &Attr;
    for (int a = 0; a < sizeof(Attr) / sizeof(GLuint); a++) {
        *pAttr++ = a;
    }

    // Compile shaders
    Globals.SimpleProgram = LoadProgram("Simple.VS", 0, "Simple.FS");
    Globals.QuadProgram = LoadProgram("Quad.VS", 0, "Quad.FS");
    Globals.GridProgram = LoadProgram("Grid.VS", 0, "Grid.FS");
    Globals.LitProgram = LoadProgram("Lit.VS", "Lit.GS", "Lit.FS");

    // Set up viewport
    float fovy = 16 * TwoPi / 180;
    float aspect = (float) cfg.Width / cfg.Height;
    float zNear = 0.1, zFar = 300;
    Globals.Projection = M4MakePerspective(fovy, aspect, zNear, zFar);
    Point3 eye = {0, 1, 4};
    Point3 target = {0, 0, 0};
    Vector3 up = {0, 1, 0};
    Globals.View = M4MakeLookAt(eye, target, up);

    // Create offscreen buffer:
    Globals.FboHandle = CreateRenderTarget(&Globals.FboTexture);
    glUseProgram(Globals.QuadProgram);
    Globals.QuadVao = CreateQuad();
    Globals.Grid = CreateGrid(GridRows, GridCols);

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
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1,1);
}

void PezUpdate(float seconds)
{
    const float RadiansPerSecond = 0.5f;
    Globals.Theta += seconds * RadiansPerSecond;
    ///Globals.Theta = Pi / 4;
}

void PezRender()
{
    #define Instances 7

    Matrix4 Model[Instances];
    Model[0] = M4MakeRotationY(Globals.Theta);
    Model[1] = M4Mul(M4Mul(
                     M4MakeTranslation((Vector3){0, 0, 0.6}),
                     M4MakeScale(V3MakeFromScalar(0.25))),
                     M4MakeRotationX(Pi/2)
    );
    Model[2] = Model[3] = Model[4] = Model[1];
    Model[1] = M4Mul(M4MakeRotationY(Globals.Theta), Model[1]);
    Model[2] = M4Mul(M4MakeRotationY(Globals.Theta + Pi/2), Model[2]);
    Model[3] = M4Mul(M4MakeRotationY(Globals.Theta - Pi/2), Model[3]);
    Model[4] = M4Mul(M4MakeRotationY(Globals.Theta + Pi), Model[4]);
    Model[5] = M4Mul(M4Mul(
                     M4MakeScale(V3MakeFromScalar(0.5)),
                     M4MakeTranslation((Vector3){0, 1.25, 0})),
                     M4MakeRotationY(-Globals.Theta)
    );
    Model[6] = M4Mul(M4Mul(
                     M4MakeScale(V3MakeFromScalar(0.5)),
                     M4MakeTranslation((Vector3){0, -1.25, 0})),
                     M4MakeRotationY(-Globals.Theta)
    );

    Vector3 LightPosition = {0.5, 0.25, 1.0}; // world space
    Vector3 EyePosition = {0, 0, 1};          // world space

    Matrix4 MVP[Instances];
    Vector3 Lhat[Instances];
    Vector3 Hhat[Instances];
    for (int i = 0; i < Instances; i++) {
        Matrix4 mv = M4Mul(Globals.View, Model[i]);
        MVP[i] = M4Mul(Globals.Projection, mv);
        Matrix3 m = M3Transpose(M4GetUpper3x3(Model[i]));
        Lhat[i] = M3MulV3(m, V3Normalize(LightPosition));    // object space
        Vector3 Eye =  M3MulV3(m, V3Normalize(EyePosition)); // object space
        Hhat[i] = V3Normalize(V3Add(Lhat[i], Eye));
    }

    int instanceCount = Instances;
    MeshPod* mesh = &Globals.Cylinder;

    glBindFramebuffer(GL_FRAMEBUFFER, Globals.FboHandle);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
  
    glUseProgram(Globals.LitProgram);
    glUniform3f(u("SpecularMaterial"), 0.4, 0.4, 0.4);
    glUniform4f(u("FrontMaterial"), 0, 0, 1, 1);
    glUniform4f(u("BackMaterial"), 0.5, 0.5, 0, 1);
    glUniform3fv(u("Hhat"), Instances, &Hhat[0].x);
    glUniform3fv(u("Lhat"), Instances, &Lhat[0].x);
    glUniformMatrix4fv(u("ModelviewProjection"), Instances, 0, (float*) &MVP[0]);

    glBindVertexArray(mesh->FillVao);
    glDrawElementsInstanced(GL_TRIANGLES, mesh->FillIndexCount, GL_UNSIGNED_SHORT, 0, instanceCount);

    glUseProgram(Globals.SimpleProgram);
    glUniform4f(u("Color"), 0, 0, 0, 1);
    glUniformMatrix4fv(u("ModelviewProjection"), Instances, 0, (float*) &MVP[0]);

    glDepthMask(GL_FALSE);
    glBindVertexArray(mesh->LineVao);
    glDrawElementsInstanced(GL_LINES, mesh->LineIndexCount, GL_UNSIGNED_SHORT, 0, instanceCount);
    glDepthMask(GL_TRUE);

    glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(Globals.QuadProgram);
    glBindTexture(GL_TEXTURE_2D, Globals.FboTexture);
    glBindVertexArray(Globals.Grid.FillVao);
    glDrawElements(GL_TRIANGLES, Globals.Grid.FillIndexCount, GL_UNSIGNED_SHORT, 0);

    if (1) {
        glUseProgram(Globals.GridProgram);
        glBindVertexArray(Globals.Grid.LineVao);
        glDrawElements(GL_LINES, Globals.Grid.LineIndexCount, GL_UNSIGNED_SHORT, 0);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
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

static Point3 EvaluateCylinder(float s, float t)
{
    Point3 range;
    const float h = 1.0;
    range.x = 0.5 * cos(t * TwoPi);
    range.y = h * (s - 0.5);
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
 
    // Create a buffer with positions
    GLuint positionsVbo;
    if (1) {
        Point3 verts[VertexCount];
        Point3* pVert = &verts[0];
        float ds = 1.0f / Stacks;
        float dt = 1.0f / Slices;

        // The upper bounds in these loops are tweaked to reduce the
        // chance of precision error causing an incorrect # of iterations.
        for (float s = 0; s < 1 + ds / 2; s += ds) {
            for (float t = 0; t < 1 + dt / 2; t += dt) {
                *pVert++ = EvaluateCylinder(s, t);
            }
        }

        pezCheck(pVert - &verts[0] == VertexCount, "Tessellation error.");

        GLsizeiptr size = sizeof(verts);
        const GLvoid* data = &verts[0].x;
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

    MeshPod mesh;
    mesh.VertexCount = VertexCount;
    mesh.FillIndexCount = FillIndexCount;
    mesh.LineIndexCount = LineIndexCount;

    glGenVertexArrays(1, &mesh.FillVao);
    glBindVertexArray(mesh.FillVao);
    glBindBuffer(GL_ARRAY_BUFFER, positionsVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trianglesVbo);
    glVertexAttribPointer(Attr.Position, 3, GL_FLOAT, GL_FALSE, 12, 0);
    glEnableVertexAttribArray(Attr.Position);

    glGenVertexArrays(1, &mesh.LineVao);
    glBindVertexArray(mesh.LineVao);
    glBindBuffer(GL_ARRAY_BUFFER, positionsVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineVbo);
    glVertexAttribPointer(Attr.Position, 3, GL_FLOAT, GL_FALSE, 12, 0);
    glEnableVertexAttribArray(Attr.Position);

    return mesh;
}

static GLuint CreateRenderTarget(GLuint* colorTexture)
{
    pezCheck(GL_NO_ERROR == glGetError(), "OpenGL error on line %d",  __LINE__);
    PezConfig cfg = PezGetConfig();

    glGenTextures(1, colorTexture);
    glBindTexture(GL_TEXTURE_2D, *colorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cfg.Width, cfg.Height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    pezCheck(GL_NO_ERROR == glGetError(), "Unable to create color texture.");

    GLuint fboHandle;
    glGenFramebuffers(1, &fboHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *colorTexture, 0);

    GLuint depthBuffer;
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, cfg.Width, cfg.Height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

    pezCheck(GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER), "Invalid FBO.");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return fboHandle;
}

static MeshPod CreateGrid(int rows, int columns)
{
    MeshPod grid;
    grid.VertexCount = (columns+1) * (rows+1);
    grid.FillIndexCount = 6 * columns * rows;
    int horizontalLines = rows * (columns + 1);
    int verticalLines = columns * (rows + 1);
    grid.LineIndexCount = 2 * (horizontalLines + verticalLines);

    // Create a buffer with interleaved positions and texture coordinates
    GLuint positionsVbo;
    if (1) {
        Vertex verts[grid.VertexCount];
        Vertex* pVert = &verts[0];
        float ds = 1.0f / columns;
        float dt = 1.0f / rows;

        // The upper bounds in these loops are tweaked to reduce the
        // chance of precision error causing an incorrect # of iterations.
        for (float s = 0; s < 1 + ds / 2; s += ds) {
            for (float t = 0; t < 1 + dt / 2; t += dt) {
                
                float x = s*2-1;
                float y = t*2-1;

                float theta  = atan2(y,x);
                float radius = sqrt(x*x+y*y);

                /*
                radius = sqrt(radius);
                x = radius * cos(theta);
                y = radius * sin(theta);
                */

                radius = radius * radius;
                float u = 0.5 * (1.0 + radius * cos(theta));
                float v = 0.5 * (1.0 + radius * sin(theta));

                pVert->Position = (Point3){x, y, 0};
                pVert->TexCoord.x = u;
                pVert->TexCoord.y = v;
                ++pVert;
            }
        }

        pezCheck(pVert - &verts[0] == grid.VertexCount, "Tessellation error.");

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
        GLushort inds[grid.FillIndexCount];
        GLushort* pIndex = &inds[0];
        GLushort n = 0;
        for (GLushort j = 0; j < columns; j++) {
            int vps = rows+1; // vertices per row
            for (GLushort i = 0; i < rows; i++) {
                *pIndex++ = (n + i + vps);
                *pIndex++ = n + (i + 1);
                *pIndex++ = n + i;
                
                *pIndex++ = (n + (i + 1) + vps);
                *pIndex++ = (n + (i + 1));
                *pIndex++ = (n + i + vps);
            }
            n += vps;
        }

        pezCheck(pIndex - &inds[0] == grid.FillIndexCount, "Tessellation error.");

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
        GLushort inds[grid.LineIndexCount];
        GLushort* pIndex = &inds[0];

        // Horizontal:
        GLushort n = 0;
        for (GLushort j = 0; j < columns+1; j++) {
            for (GLushort i = 0; i < rows; i++) {
                *pIndex++ = n + i;
                *pIndex++ = n + i + 1;
            }
            n += rows + 1;
        }

        // Vertical:
        n = 0;
        for (GLushort j = 0; j < columns; j++) {
            for (GLushort i = 0; i < rows+1; i++) {
                *pIndex++ = n + i;
                *pIndex++ = n + i + (rows + 1);
            }
            n += rows + 1;
        }

        pezCheck(pIndex - &inds[0] == grid.LineIndexCount, "Tessellation error.");

        GLsizeiptr size = sizeof(inds);
        const GLvoid* data = &inds[0];
        GLenum usage = GL_STATIC_DRAW;
        glGenBuffers(1, &lineVbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineVbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
    }
    
    glGenVertexArrays(1, &grid.FillVao);
    glBindVertexArray(grid.FillVao);
    glBindBuffer(GL_ARRAY_BUFFER, positionsVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, trianglesVbo);
    glVertexAttribPointer(Attr.Position, 3, GL_FLOAT, GL_FALSE, 20, 0);
    glEnableVertexAttribArray(Attr.Position);
    glVertexAttribPointer(Attr.TexCoord, 2, GL_FLOAT, GL_FALSE, 20, offset(12));
    glEnableVertexAttribArray(Attr.TexCoord);

    glGenVertexArrays(1, &grid.LineVao);
    glBindVertexArray(grid.LineVao);
    glBindBuffer(GL_ARRAY_BUFFER, positionsVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lineVbo);
    glVertexAttribPointer(Attr.Position, 3, GL_FLOAT, GL_FALSE, 20, 0);
    glEnableVertexAttribArray(Attr.Position);
    glDisableVertexAttribArray(Attr.TexCoord);

    glBindVertexArray(0);

    return grid;
}

static GLuint CreateQuad()
{
    const PezConfig cfg = PezGetConfig();

    int sourceWidth = cfg.Width;
    int sourceHeight = -cfg.Height;
    int destWidth = cfg.Width;
    int destHeight = cfg.Height;

    // Stretch to fit:
    float q[] = {
        -1, -1, 0, 1,
        +1, -1, 1, 1,
        -1, +1, 0, 0,
        +1, +1, 1, 0 };
        
    if (sourceHeight < 0) {
        sourceHeight = -sourceHeight;
        q[3] = 1-q[3];
        q[7] = 1-q[7];
        q[11] = 1-q[11];
        q[15] = 1-q[15];
    }

    float sourceRatio = (float) sourceWidth / sourceHeight;
    float destRatio = (float) destWidth  / destHeight;
    
    // Horizontal fit:
    if (sourceRatio > destRatio) {
        q[1] = q[5] = -destRatio / sourceRatio;
        q[9] = q[13] = destRatio / sourceRatio;

    // Vertical fit:    
    } else {
        q[0] = q[8] = -sourceRatio / destRatio;
        q[4] = q[12] = sourceRatio / destRatio;
    }

    GLuint vbo, vao;
    glUseProgram(Globals.QuadProgram);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(q), q, GL_STATIC_DRAW);
    glVertexAttribPointer(Attr.Position, 2, GL_FLOAT, GL_FALSE, 16, 0);
    glVertexAttribPointer(Attr.TexCoord, 2, GL_FLOAT, GL_FALSE, 16, offset(8));
    glEnableVertexAttribArray(Attr.Position);
    glEnableVertexAttribArray(Attr.TexCoord);
    return vao;
}
