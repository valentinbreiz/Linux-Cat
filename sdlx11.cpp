#include "sdlx11.hpp"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <GL/glx.h>
#include <SDL2/SDL.h>

SDL_Window*
SDLx11::SDL_CreateWindowEx(const char *title, int x, int y, int w, int h, bool fullscreen, double frame_alpha)
{
    xdisplay_ = XOpenDisplay(0);
    const char *xserver = getenv("DISPLAY");

    if (xdisplay_ == 0)
    {
        fprintf(stderr, "Could not establish a connection to X-server '%s'\n", xserver);
        exit(1);
    }

    // query Visual for "TrueColor" and 32 bits depth (RGBA)

    static int visualData[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_DEPTH_SIZE, 16,
        None};

    XVisualInfo *visual;
    XRenderPictFormat *pict_format;
    int numfbconfigs;
    GLXFBConfig fbconfig = 0, *fbconfigs = glXChooseFBConfig(xdisplay_, DefaultScreen(xdisplay_), visualData, &numfbconfigs);

    for (int i = 0; i < numfbconfigs; i++)
    {
        visual = (XVisualInfo*) glXGetVisualFromFBConfig(xdisplay_, fbconfigs[i]);
        if (!visual)
            continue;

        pict_format = XRenderFindVisualFormat(xdisplay_, visual->visual);
        if (!pict_format)
            continue;

        fbconfig = fbconfigs[i];
        if (pict_format->direct.alphaMask > 0)
            break;
    }

    if (!fbconfig)
    {
        fprintf(stderr, "No matching FB config found!");
        exit(1);
    }

    // create transparent window

    XSetWindowAttributes attr;
    attr.colormap = XCreateColormap(xdisplay_, RootWindow(xdisplay_, visual->screen), visual->visual, AllocNone);
    // make sure to select same event_mask as SDL would do to be able to handle them in our xevent procedure
    attr.event_mask = FocusChangeMask | EnterWindowMask | LeaveWindowMask |
                      ExposureMask | ButtonPressMask | ButtonReleaseMask |
                      PointerMotionMask | KeyPressMask | KeyReleaseMask |
                      PropertyChangeMask | StructureNotifyMask | 
                      ButtonPressMask | ButtonReleaseMask | KeymapStateMask;
    attr.background_pixmap = None;
    attr.border_pixel = 0;
    attr.override_redirect = True;

    xwindow_ = XCreateWindow(xdisplay_, DefaultRootWindow(xdisplay_),
                             x, y, w, h, // x,y,width,height : are possibly opverwriteen by window manager
                             0,
                             visual->depth,
                             InputOutput,
                             visual->visual,
                             CWColormap | CWEventMask | CWBackPixmap | CWBorderPixel,
                             &attr);

    XCreateGC(xdisplay_, xwindow_, 0, 0);

    // set title bar name of window
    XStoreName(xdisplay_, xwindow_, title);

    // say window manager which position we would prefer
    XSizeHints sizehints;
    sizehints.flags = PPosition | PSize;
    sizehints.x = x;
    sizehints.y = y;
    sizehints.width = w;
    sizehints.height = h;
    XSetWMNormalHints(xdisplay_, xwindow_, &sizehints);

    // Switch On If user pressed close key let window manager only send notification
    Atom wm_delete_window = XInternAtom(xdisplay_, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(xdisplay_, xwindow_, &wm_delete_window, 1);

    // create OpenGL context
    // oldstyle context:
    // GLXContext glcontext = glXCreateContext(xdisplay_, visual, NULL, True);
    // New style:
    #define GLX_CONTEXT_MINOR_VERSION_ARB        0x2092
    typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
    GLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = 
        (GLXCREATECONTEXTATTRIBSARBPROC) glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

    int attribs[] = { // change it for your needs
        GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
        GLX_CONTEXT_MINOR_VERSION_ARB, 1,
        0};

    GLXContext glcontext = glXCreateContextAttribsARB(xdisplay_, fbconfig, 0, true, attribs);

    if (!glcontext)
    {
        fprintf(stderr, "X11 server '%s' does not support OpenGL\n", xserver);
        exit(1);
    }

    if (! glXMakeCurrent(xdisplay_, xwindow_, glcontext))
    {
        fprintf(stderr, "OpenGL glXMakeCurrent failed!\n");
        exit(1);
    }

    // make title bar transparent as well
    unsigned long opacity = (unsigned long)(0xFFFFFFFFul * frame_alpha);
    Atom XA_NET_WM_WINDOW_OPACITY = XInternAtom(xdisplay_, "_NET_WM_WINDOW_OPACITY", False);
    XChangeProperty(xdisplay_, xwindow_, XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char *) &opacity, 1L);

    // now let the window appear to the user
    XMapWindow(xdisplay_, xwindow_);
    glXSwapBuffers(xdisplay_, xwindow_);

    if (fullscreen)
    {
        Atom wm_state = XInternAtom(xdisplay_, "_NET_WM_STATE", false);
        Atom fullscreen = XInternAtom(xdisplay_, "_NET_WM_STATE_FULLSCREEN", false);
        XEvent xev;
        memset(&xev, 0, sizeof(xev));
        xev.type = ClientMessage;
        xev.xclient.window = xwindow_;
        xev.xclient.message_type = wm_state;
        xev.xclient.format = 32;
        xev.xclient.data.l[0] = 1;
        xev.xclient.data.l[1] = fullscreen;
        xev.xclient.data.l[2] = 0;
 
        // didn't check yet for multiple monitors, this snipped may help to start enabling this
        //Atom fullmons = XInternAtom(xdisplay_, "_NET_WM_FULLSCREEN_MONITORS", False);
        //XEvent xev;
        //memset(&xev, 0, sizeof(xev));
        //xev.type = ClientMessage;
        //xev.xclient.window = xwindow_;
        //xev.xclient.message_type = fullmons;
        //xev.xclient.format = 32;
        //xev.xclient.data.l[0] = 0; /* your topmost monitor number */
        //xev.xclient.data.l[1] = 0; /* bottommost */
        //xev.xclient.data.l[2] = 0; /* leftmost */
        //xev.xclient.data.l[3] = 1; /* rightmost */
        //xev.xclient.data.l[4] = 0; /* source indication */

        XSendEvent(xdisplay_, DefaultRootWindow(xdisplay_), false,
                   SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    }

    sdl_window_ = SDL_CreateWindowFrom((void *)xwindow_);

    if (sdl_window_ == NULL)
    {
        fprintf(stderr, "SDL error SDL_CreateWindowFrom: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_ShowWindow(sdl_window_);
    
    // SDL_CreateWindowFrom use its own xdisplay, so manipulate mask accordingly that SDL_PollEvent will handle our events
    // Button*Mask events are private and can not be set here and must be handled later in our xevent proc.
    XSetWindowAttributes attributes;
    SDL_VERSION(&sdlSysWMinfo_.version);
    SDL_GetWindowWMInfo(sdl_window_, &sdlSysWMinfo_);
    attributes.event_mask = FocusChangeMask | EnterWindowMask | LeaveWindowMask | 
        ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask |
        PropertyChangeMask | StructureNotifyMask | KeymapStateMask
        /*| ButtonPressMask | ButtonReleaseMask*/;
    XChangeWindowAttributes(sdlSysWMinfo_.info.x11.display, sdlSysWMinfo_.info.x11.window, CWEventMask, &attributes);

    XFlush(sdlSysWMinfo_.info.x11.display);

    return sdl_window_;
}

SDL_Renderer*
SDLx11::SDL_Create(const char *title, int x, int y, int w, int h, Uint32 render_flags, bool fullscreen, double frame_alpha)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        fprintf(stderr, "SDL error SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Window *win = SDL_CreateWindowEx(title, x, y, w, h, fullscreen, frame_alpha);
    SDL_SetWindowBordered(win, SDL_FALSE);
    SDL_SetWindowAlwaysOnTop(win, SDL_TRUE);

    renderer_ = SDL_CreateRenderer(win, -1, render_flags);

    if (renderer_ == NULL)
    {
        fprintf(stderr, "SDL error SDL_CreateRenderer: %s\n", SDL_GetError());
        exit(1);
    }

    return renderer_;
}

void SDLx11::SDL_Destroy()
{
    if (renderer_)  SDL_DestroyRenderer(renderer_);
    if (xwindow_)   XDestroyWindow(xdisplay_, (Window) xwindow_);
    if (xdisplay_)  XCloseDisplay(xdisplay_);
    
    xdisplay_   = NULL;
    xwindow_    = 0;
    renderer_   = NULL;
    sdl_window_ = NULL;
}

int SDLx11::SDL_PollEvent(SDL_Event* e)
{
    // just handle a few window messages and mouse button/wheel for SDL, the rest should be handled by SDL
    while (xdisplay_ && XPending(xdisplay_) > 0)
    {
        XEvent event;
        XNextEvent(xdisplay_, &event);

        // handle mouse button and wheel events and pass them SDL
        if (event.type==ButtonPress || event.type==ButtonRelease)
        {
            SDL_Event sdlevent;
            int button = event.xbutton.button;

            if (button > 3 && button < 8)
            {   // wheel X buttons 4-7
                if (event.type == ButtonRelease)
                {
                    int xticks = 0, yticks = 0;
                    switch (button) {
                        case 4: yticks =  1; break;
                        case 5: yticks = -1; break;
                        case 6: xticks =  1; break;
                        case 7: xticks = -1; break;
                    }
                    sdlevent.type = SDL_MOUSEWHEEL;
                    sdlevent.wheel.windowID = SDL_GetWindowID(sdl_window_);
                    sdlevent.wheel.which = 0;
                    sdlevent.wheel.x = xticks;
                    sdlevent.wheel.y = yticks;
                    sdlevent.wheel.direction = button&1 ? SDL_MOUSEWHEEL_FLIPPED : SDL_MOUSEWHEEL_NORMAL;
                    SDL_PushEvent(&sdlevent);
                }
            }
            else
            {   // the other X mouse buttons, sort 4-7 out and reorder buttons above 7
                if (button > 7) button -= (8-SDL_BUTTON_X1);
                sdlevent.button.button = button;
                sdlevent.button.windowID = SDL_GetWindowID(sdl_window_);
                sdlevent.type = event.type==ButtonPress ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
                SDL_PushEvent(&sdlevent);
            }
        }

        switch (event.type)
        {
            case ClientMessage: // now handle SDL_WINDOWEVENT
                if (event.xclient.message_type == XInternAtom(xdisplay_, "WM_PROTOCOLS", 1) 
                    && event.xclient.data.l[0] == (int) XInternAtom(xdisplay_, "WM_DELETE_WINDOW", 1))
                {   // SDL_WINDOWEVENT_CLOSE / SDL_QUIT
                    SDL_Event sdlevent;
                    sdlevent.window.windowID = SDL_GetWindowID(sdl_window_);
                    sdlevent.type = SDL_WINDOWEVENT;
                    sdlevent.window.event = SDL_WINDOWEVENT_CLOSE;
                    SDL_PushEvent(&sdlevent);
                    sdlevent.type = SDL_QUIT;
                    SDL_PushEvent(&sdlevent);
                }
                break;
            // ...
        }
    }

    // call and return SDLs SDL_PollEvent for the other events
    return ::SDL_PollEvent(e);
}