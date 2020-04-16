// Pez was developed by Philip Rideout and released under the MIT License.

#ifdef Linux
#include <GL/glew.h>
#include <GL/gl.h>
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#ifndef GL3_PROTOTYPES
#define GL3_PROTOTYPES
#endif
#endif
#include <GL/glx.h>
#include <libgen.h>

#include "pez.h"
#include "bstrlib.h"
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <wchar.h>

// TODO : USE_MOTIF_BORDERLESS_WINDOW is currently defined at build time.
// Next step : use an environment variable and pass it as argv[1] instead ?

#ifdef USE_MOTIF_BORDERLESS_WINDOW
#include <Xm/MwmUtil.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>

#ifdef Linux
#include <stdlib.h>
const char * env_var ="MESA_GL_VERSION_OVERRIDE=4.5";
const char * env_var2 ="MESA_GLSL_VERSION_OVERRIDE=450";
#endif /* Linux */

typedef struct PlatformContextRec
{
    Display* MainDisplay;
    Window MainWindow;
} PlatformContext;

unsigned int GetMicroseconds()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000000 + tp.tv_usec;
}

int main(int argc, char** argv)
{
#ifdef Linux
    putenv((char *)env_var);
    putenv((char *)env_var2);
#endif
    int attrib[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 24,
        None
    };
    
    PlatformContext context;

    context.MainDisplay = XOpenDisplay(NULL);
    int screenIndex = DefaultScreen(context.MainDisplay);
    Window root = RootWindow(context.MainDisplay, screenIndex);

    int fbcount;
    PFNGLXCHOOSEFBCONFIGPROC glXChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC)glXGetProcAddress((GLubyte*)"glXChooseFBConfig");
    GLXFBConfig *fbc = glXChooseFBConfig(context.MainDisplay, screenIndex, attrib, &fbcount);
    if (!fbc)
        pezFatal("Failed to retrieve a framebuffer config\n");

    PFNGLXGETVISUALFROMFBCONFIGPROC glXGetVisualFromFBConfig = (PFNGLXGETVISUALFROMFBCONFIGPROC) glXGetProcAddress((GLubyte*)"glXGetVisualFromFBConfig");
    if (!glXGetVisualFromFBConfig)
        pezFatal("Failed to get a GLX function pointer\n");

    PFNGLXGETFBCONFIGATTRIBPROC glXGetFBConfigAttrib = (PFNGLXGETFBCONFIGATTRIBPROC) glXGetProcAddress((GLubyte*)"glXGetFBConfigAttrib");
    if (!glXGetFBConfigAttrib)
        pezFatal("Failed to get a GLX function pointer\n");

    if (PezGetConfig().Multisampling) {
        int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;
        for ( int i = 0; i < fbcount; i++ ) {
            XVisualInfo *vi = glXGetVisualFromFBConfig( context.MainDisplay, fbc[i] );
            if (!vi) {
                continue;
            }
            int samp_buf, samples;
            glXGetFBConfigAttrib( context.MainDisplay, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
            glXGetFBConfigAttrib( context.MainDisplay, fbc[i], GLX_SAMPLES       , &samples  );
            //printf( "  Matching fbconfig %d, visual ID 0x%2x: SAMPLE_BUFFERS = %d,"
            //        " SAMPLES = %d\n", 
            //        i, (unsigned int) vi->visualid, samp_buf, samples );
            if ( best_fbc < 0 || (samp_buf && samples > best_num_samp) )
                best_fbc = i, best_num_samp = samples;
            if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp )
                worst_fbc = i, worst_num_samp = samples;
            XFree( vi );
        }
        fbc[0] = fbc[ best_fbc ];
    }

    XVisualInfo *visinfo = glXGetVisualFromFBConfig(context.MainDisplay, fbc[0]);
    if (!visinfo)
        pezFatal("Error: couldn't create OpenGL window with this pixel format.\n");

    XSetWindowAttributes attr;
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(context.MainDisplay, root, visinfo->visual, AllocNone);
    attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask |
                      PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

    context.MainWindow = XCreateWindow(
        context.MainDisplay,
        root,
        0, 0,
        PezGetConfig().Width, PezGetConfig().Height, 0,
        visinfo->depth,
        InputOutput,
        visinfo->visual,
        CWBackPixel | /*CWBorderPixel |*/ CWColormap | CWEventMask,
        &attr
    );

#ifdef USE_MOTIF_BORDERLESS_WINDOW
    int borderless = 1;

    if (borderless) {
        Atom mwmHintsProperty = XInternAtom(context.MainDisplay, "_MOTIF_WM_HINTS", 0);
        MwmHints hints = {0};
        hints.flags = MWM_HINTS_DECORATIONS;
        hints.decorations = 0;
        XChangeProperty(context.MainDisplay, context.MainWindow, mwmHintsProperty, mwmHintsProperty, 32,
                        PropModeReplace, (unsigned char *)&hints, PROP_MWM_HINTS_ELEMENTS);
    }
#endif
    XMapWindow(context.MainDisplay, context.MainWindow);

    int centerWindow = 1;
    if (centerWindow) {
        Screen* pScreen = XScreenOfDisplay(context.MainDisplay, screenIndex);
        int left = XWidthOfScreen(pScreen)/2 - PezGetConfig().Width/2;
        int top = XHeightOfScreen(pScreen)/2 - PezGetConfig().Height/2;
        XMoveWindow(context.MainDisplay, context.MainWindow, left, top);
    }

    GLXContext glcontext;
    if (0 && PEZ_FORWARD_COMPATIBLE_GL) {
        GLXContext tempContext = glXCreateContext(context.MainDisplay, visinfo, NULL, True);
        PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribs = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((GLubyte*)"glXCreateContextAttribsARB");
        if (!glXCreateContextAttribs) {
            pezFatal("Your platform does not support OpenGL 4.0.\n"
                          "Try changing PEZ_FORWARD_COMPATIBLE_GL to 0.\n");
        }
        int fbcount = 0;
#ifdef Linux
    glewExperimental = GL_TRUE;
    glewInit();
#endif
        GLXFBConfig *framebufferConfig = glXChooseFBConfig(context.MainDisplay, screenIndex, 0, &fbcount);
        if (!framebufferConfig) {
            pezFatal("Can't create a framebuffer for OpenGL 4.0.\n");
        } else {
            int attribs[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
                GLX_CONTEXT_MINOR_VERSION_ARB, 0,
                GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                // WIP source : https://www.khronos.org/registry/OpenGL/extensions/ARB/GLX_ARB_create_context.txt
                //GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                0
            }; 
            glcontext = glXCreateContextAttribs(context.MainDisplay, framebufferConfig[0], NULL, True, attribs);
            glXMakeCurrent(context.MainDisplay, 0, 0);
            glXDestroyContext(context.MainDisplay, tempContext);
        } 
    } else {
        glcontext = glXCreateContext(context.MainDisplay, visinfo, NULL, True);
    }

    glXMakeCurrent(context.MainDisplay, context.MainWindow, glcontext);

    PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC) glXGetProcAddress((GLubyte*)"glXSwapIntervalSGI");
    if (glXSwapIntervalSGI) {
        glXSwapIntervalSGI(PezGetConfig().VerticalSync ? 1 : 0);
    }

    // Reset OpenGL error state:
    glGetError();

    // Lop off the trailing .c
    bstring name = bfromcstr(PezGetConfig().Title);
    bstring shaderPrefix = bmidstr(name, 0, blength(name) - 1);
    pezSwInit(bdata(shaderPrefix));
    bdestroy(shaderPrefix);

    // Set up the Shader Wrangler
    const char* currdir = dirname(argv[0]);
    if (!currdir || !*currdir) {
        pezSwAddPath("./", ".glsl");
    } else if (currdir[strlen(currdir) - 1] == '/') {
        pezSwAddPath(currdir, ".glsl");
    } else {
        bstring dir = bformat("%s/", currdir);
        pezSwAddPath(bdata(dir), ".glsl");
        bdestroy(dir);
    }

    pezSwAddPath("../", ".glsl");
    char qualifiedPath[128];
    strcpy(qualifiedPath, pezResourcePath());
    strcat(qualifiedPath, "/");
    pezSwAddPath(qualifiedPath, ".glsl");
    pezSwAddDirective("*", "#version 420");

    // Perform user-specified intialization
    pezPrintString("OpenGL Version: %s\n", glGetString(GL_VERSION));
    PezInitialize();
    bstring windowTitle = bmidstr(name, 0, blength(name) - 2);
    XStoreName(context.MainDisplay, context.MainWindow, bdata(windowTitle));
    bdestroy(windowTitle);
    bdestroy(name);
    
    // -------------------
    // Start the Game Loop
    // -------------------

    unsigned int previousTime = GetMicroseconds();
    int done = 0;
    while (!done) {
        
        if (glGetError() != GL_NO_ERROR)
            pezFatal("OpenGL error.\n");

        if (XPending(context.MainDisplay)) {
            XEvent event;
    
            XNextEvent(context.MainDisplay, &event);
            switch (event.type)
            {
                case Expose:
                    //redraw(display, event.xany.window);
                    break;
                
                case ConfigureNotify:
                    //resize(event.xconfigure.width, event.xconfigure.height);
                    break;
                
#ifdef PEZ_MOUSE_HANDLER
                case ButtonPress:
                    PezHandleMouse(event.xbutton.x, event.xbutton.y, PEZ_DOWN);
                    break;

                case ButtonRelease:
                    PezHandleMouse(event.xbutton.x, event.xbutton.y, PEZ_UP);
                    break;

                case MotionNotify:
                    PezHandleMouse(event.xmotion.x, event.xmotion.y, PEZ_MOVE);
                    break;
#endif

                case KeyRelease:
                case KeyPress: {
                    XComposeStatus composeStatus;
                    char asciiCode[32];
                    KeySym keySym;

                    int len = XLookupString(&event.xkey, asciiCode, sizeof(asciiCode), &keySym, &composeStatus);

                    if (len == 0)
                        fprintf( stderr, "Buffer empty");

                    switch (asciiCode[0]) {
                        case 'x': case 'X': case 'q': case 'Q':
                        case 0x1b:
                            done = 1;
                            break;
                    }
                }
            }
        }

        unsigned int currentTime = GetMicroseconds();
        unsigned int deltaTime = currentTime - previousTime;
        previousTime = currentTime;
        
        PezUpdate((float) deltaTime / 1000000.0f);

        PezRender(0);
        glXSwapBuffers(context.MainDisplay, context.MainWindow);
    }

    pezSwShutdown();

    return 0;
}

void pezPrintStringW(const wchar_t* pStr, ...)
{
    va_list a;
    va_start(a, pStr);

    wchar_t msg[1024] = {0};
    vswprintf(msg, countof(msg), pStr, a);
    fputws(msg, stderr);
}

void pezPrintString(const char* pStr, ...)
{
    va_list a;
    va_start(a, pStr);

    char msg[1024] = {0};
    vsnprintf(msg, countof(msg), pStr, a);
    fputs(msg, stderr);
}

void pezFatalW(const wchar_t* pStr, ...)
{
    fwide(stderr, 1);

    va_list a;
    va_start(a, pStr);

    wchar_t msg[1024] = {0};
    vswprintf(msg, countof(msg), pStr, a);
    fputws(msg, stderr);
    exit(1);
}

void _pezFatal(const char* pStr, va_list a)
{
    char msg[1024] = {0};
    vsnprintf(msg, countof(msg), pStr, a);
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

void pezFatal(const char* pStr, ...)
{
    va_list a;
    va_start(a, pStr);
    _pezFatal(pStr, a);
}

void pezCheck(int condition, ...)
{
    va_list a;
    const char* pStr;

    if (condition)
        return;

    va_start(a, condition);
    pStr = va_arg(a, const char*);
    _pezFatal(pStr, a);
}

void pezCheckPointer(void* p, ...)
{
    va_list a;
    const char* pStr;

    if (p != NULL)
        return;

    va_start(a, p);
    pStr = va_arg(a, const char*);
    _pezFatal(pStr, a);
}

int pezIsPressing(char key)
{
    return 0;
}

const char* pezResourcePath()
{
    return ".";
}
