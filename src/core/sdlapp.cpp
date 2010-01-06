/*
    Copyright (c) 2009 Andrew Caudwell (acaudwell@gmail.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. The name of the author may not be used to endorse or promote products
       derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "sdlapp.h"

std::string gSDLAppResourceDir;

#ifdef _WIN32
std::string gSDLAppPathSeparator = "\\";
#else
std::string gSDLAppPathSeparator = "/";
#endif

bool SDLAppDirExists(std::string dir) {
    struct stat st;
    return !stat(dir.c_str(), &st) && S_ISDIR(st.st_mode);
}

void SDLAppInit() {
    if(gSDLAppResourceDir.size()>0) return;

    std::string resource_dir = "data/";
    std::string shader_dir   = "data/shaders/";
    std::string fonts_dir    = "data/fonts/";
#ifdef _WIN32
    char szAppPath[MAX_PATH];
    GetModuleFileName(0, szAppPath, MAX_PATH);

    // Extract directory
    std::string exepath = std::string(szAppPath);

    int pos = exepath.rfind("\\");

    std::string path = exepath.substr(0, pos+1);
    resource_dir = path + std::string("\\data\\");
    shader_dir   = path + std::string("\\data\\shaders\\");
    fonts_dir    = path + std::string("\\data\\fonts\\");
#else
    //get working directory
    char cwd_buff[1024];

    if(getcwd(cwd_buff, 1024) == cwd_buff) {
        resource_dir = std::string(cwd_buff) + std::string("/") + resource_dir;
        fonts_dir    = std::string(cwd_buff) + std::string("/") + fonts_dir;
    }

#endif

#ifdef SDLAPP_RESOURCE_DIR
    if (SDLAppDirExists(SDLAPP_RESOURCE_DIR)) {
        resource_dir = SDLAPP_RESOURCE_DIR;
        shader_dir   = SDLAPP_RESOURCE_DIR + std::string("/shaders/");
        fonts_dir    = SDLAPP_RESOURCE_DIR + std::string("/fonts/");
    }
#endif

#ifdef SDLAPP_FONT_DIR
    if (SDLAppDirExists(SDLAPP_FONT_DIR)) {
        fonts_dir    = SDLAPP_FONT_DIR;
    }
#endif

    texturemanager.setDir(resource_dir);
    fontmanager.setDir(fonts_dir);

#ifdef SDLAPP_SHADER_SUPPORT
    shadermanager.setDir(shader_dir);
#endif

    gSDLAppResourceDir = resource_dir;
}

void SDLAppParseArgs(int argc, char *argv[], int* xres, int* yres, bool* fullscreen, std::vector<std::string>* otherargs) {
    SDLAppInit();

    for (int i=1; i<argc; i++) {
        debugLog("argv[%d] = %s\n", i, argv[i]);

        if (!strcmp(argv[i],"-f")) {
            *fullscreen = 1;
            continue;
        }
        else if (!strcmp(argv[i],"-w")) {
            *fullscreen = 0;
            continue;
        }

        //get video mode
        if(strchr(argv[i], '-') != 0 && strlen(argv[i])>1) {
            std::string argstr(argv[i], 1, strlen(argv[i])-1);
            debugLog("%s\n", argstr.c_str());
            size_t x = argstr.rfind("x");

            if(x != std::string::npos) {
                std::string widthstr  = argstr.substr(0, x);
                std::string heightstr = argstr.substr(x+1);

                int width = atoi(widthstr.c_str());
                int height = atoi(heightstr.c_str());

                if(width!=0 && height!=0) {
                    debugLog("w=%d, h=%d\n",width,height);

                    *xres = width;
                    *yres = height;
                    continue;
                }
            }
        }

        // non display argument
        if(otherargs != 0) {
            otherargs->push_back(std::string(argv[i]));
        }
    }
}

SDLApp::SDLApp() {
    fps=0;
    return_code=0;
    appFinished=false;
}

void SDLApp::updateFramerate() {
    if(fps_updater>0) {
        fps = (float)frame_count / (float)fps_updater * 1000.0f;
    } else {
        fps = 0;
    }
    fps_updater = 0;
    frame_count = 0;
}

bool SDLApp::isFinished() {
    return appFinished;
}

int SDLApp::returnCode() {
    return return_code;
}

void SDLApp::stop(int return_code) {
    this->return_code = return_code;
    appFinished=true;
}

int SDLApp::run() {

    Uint32 msec=0, last_msec=0, buffer_msec=0, total_msec = 0;

    frame_count = 0;
    fps_updater = 0;

    if(!appFinished) init();

    msec = SDL_GetTicks();
    last_msec = msec;

    while(!appFinished) {
        last_msec = msec;
        msec      = SDL_GetTicks();

        Uint32 delta_msec = msec - last_msec;

        // cant have delta ticks be 0
        buffer_msec += delta_msec;
        if(buffer_msec < 1) {
            SDL_Delay(1);
            continue;
        }

        delta_msec = buffer_msec;
        buffer_msec =0;

        //determine time elapsed since last time we were here
        total_msec += delta_msec;

        float t  = total_msec / 1000.0f;
        float dt = delta_msec / 1000.0f;

        fps_updater += delta_msec;

        //update framerate if a second has passed
        if (fps_updater >= 1000) {
            updateFramerate();
        }

        //process new events
        SDL_Event event;
        while ( SDL_PollEvent(&event) ) {

            switch(event.type) {
                case SDL_QUIT:
                    appFinished=true;
                    break;

                case SDL_MOUSEMOTION:
                    mouseMove(&event.motion);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    mouseClick(&event.button);
                    break;

                case SDL_MOUSEBUTTONUP:
                    mouseClick(&event.button);
                    break;

                case SDL_KEYDOWN:
                    keyPress(&event.key);
                    break;

                case SDL_KEYUP:
                    keyPress(&event.key);
                    break;

                default:
                    break;
            }
        }

        update(t, dt);

        //update display
        display.update();
        frame_count++;
    }

    return return_code;
}
