/* 
*  Under e.g. Ubuntu 20.04 xwindow events are not fired by SDL when using SDL_CreateWindowFrom with native x11 xlib window.
*  It's needed to do things manually to accomplish this.
*  This class creates a native transparent openGL enabled xlib window and associates it to a SDL_Window.
*  Additionally a custom SDL_PollEvent is available which should handle almost all usual SDL events.
*  Xevents can be retrieved also parallel using the event procedure.
*  To get mouse state incl. button SDL_GetGlobalMouseState must be used instead of SDL_GetMouseState!
*  This class works too with the SDL_GPU library.
*  This class depends on addtional libraries: xlib, xrender, SDL2, glx and if you want to use it sdl-gpu
*  It was tested and worked with Ubuntu 20.04, SDL 2.0.13, sdl-gpu 0.12.0
*  It works even with ImGui docking branch with imgui_impl_opengl3.h 
*    All events inklusive clipboard are working (a small patch is maybe neccessary, see above: To get mouse state... ).
*  To use it create an own class and derive it from this class
*  
*  EXAMPLE:
#include <SDL2/SDL.h>
#include "sdlx11.hpp"
//#define USE_SDL_GPU_LIB
#ifdef USE_SDL_GPU_LIB
#include <SDL_gpu.h>
#endif
class MySDLx11App : public SDLx11
{
    #ifdef USE_SDL_GPU_LIB
    GPU_Target* screen_;
    #endif
public:
    void run()
    {
        SDL_Event event;
        bool done = false;
        SDL_Create("Transparent native xlib window with SDL2 & openGL (hit escape to leave)", 
                   0, 0, 1280, 720, 0, false, 1.0f);
        #ifdef USE_SDL_GPU_LIB
        GPU_SetInitWindow(SDL_GetWindowID(sdl_window_));
        screen_ = GPU_Init(1280, 720, GPU_DEFAULT_INIT_FLAGS);
        #endif
        while (!done)
        {
            while (SDL_PollEvent(&event)) // use custom SDL_PollEvent from base class as usual
            {
                printf("SDL_PollEvent got event %d\n", event.type);
                if (event.type == SDL_QUIT
                || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
                    done = true;
            }
            #ifdef USE_SDL_GPU_LIB
            GPU_ClearRGBA(screen_, 0, 0, 0, 0);
            // draw easter egg
            GPU_EllipseFilled(screen_, 200, 200, 100, 70, 90.0f, {136, 78, 160, 255});
            GPU_Flip(screen_);
            #else // usual SDL
            // play around with background color and alpha value, see frame_alpha value in function SDL_Create as well
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
            SDL_RenderClear(renderer_);
            // draw solid blue rect
            SDL_Rect r = {100, 100, 100, 100};
            SDL_SetRenderDrawColor(renderer_, 0, 0, 255, 255);
            SDL_RenderFillRect(renderer_, &r);
            SDL_RenderPresent(renderer_);
            #endif
            SDL_Delay(1000 / 60);
        }
    }
};
int main(int argc, char** argv)
{
    MySDLx11App app;
    app.run();
    return 0;
}
*/
#pragma once
#include <X11/Xlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_image.h>

#define SPEED 100

class SDLx11
{
protected:
    Display*      xdisplay_;
    Window        xwindow_;
    SDL_Renderer* renderer_;
    SDL_Window*   sdl_window_;
    SDL_SysWMinfo sdlSysWMinfo_; // get access to SDLs xdisplay
    SDL_DisplayMode dm;

public:
    SDLx11() : xdisplay_(NULL), xwindow_(0), renderer_(NULL), sdl_window_(NULL) 
        { memset(&sdlSysWMinfo_, 0, sizeof(SDL_SysWMinfo)); }
    virtual ~SDLx11() { SDL_Destroy(); }

    // create window & renderer
    SDL_Renderer*
    SDL_Create(
        const char *title, 
        int x, int y, int w, int h, 
        Uint32 render_flags = 0, 
        bool fullscreen = false, 
        double frame_alpha = 1.0);

    // create just window
    SDL_Window*
    SDL_CreateWindowEx(
        const char *title, 
        int x, int y, int w, int h, 
        bool fullscreen = false, 
        double frame_alpha = 1.0);

    void SDL_Destroy();

    int SDL_PollEvent(SDL_Event*);
};