/*
    Copyright (C) 2009 Andrew Caudwell (acaudwell@gmail.com)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version
    3 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MANDELBULB_VIEWER_H
#define MANDELBULB_VIEWER_H

#include <time.h>

#include "core/sdlapp.h"
#include "core/display.h"
#include "core/shader.h"
#include "core/fxfont.h"
#include "core/pi.h"
#include "core/mousecursor.h"

#include "viewer_settings.h"

#include "vcamera.h"
#include "ppm.h"

class MandelbulbViewer : public SDLApp {

    bool mousemove;

    int render_width;
    int render_height;

    Shader* shader;
    FXFont font;

    bool debug;

    std::string shaderfile;

    std::string message;
    float message_timer;
    vec3f message_colour;


    float time_elapsed;

    bool play;
    bool record;
    int record_frame_skip;
    float record_frame_delta;
    FrameExporter* frameExporter;

    float runtime;
    float fixed_tick_rate;

    int frame_skip;
    int frame_count;
    int last_frame;

    bool scanline_mode;
    bool scanline_debug;
    int  scanline_count;
    int  scanline_batch_size;
    
    float scanline_target_fps;
    float scanline_target_rps;

    ViewCameraPath campath;

    bool paused;

    vec2f mousepos;
    bool roll;
    bool mouselook;
    MouseCursor cursor;

    ViewCamera view;
    Object3D mandelbulb;

    mat3f viewRotation;
    mat3f objRotation;

    float speed;

    float beatTimer;
    int   beatCount;
    float beatGlowDepth;
    float beatGlowMulti;
    float pulse;

    vec3f _julia_c;

    void randomizeJuliaSeed();
    void randomizeColours();

    void togglePlay();
    void toggleRecord();
    void resetCamPath();

    void addWaypoint(float duration);
    void removeWaypoint();

    void moveCam(float dt);

    void setMessage(const std::string& message, const vec3f& colour = vec3f(1.0, 1.0, 1.0));
    void setDefaults();

    GLuint rendertex;
    GLuint frametex;

    void drawAlignedQuad(int w, int h);

    void drawMandelbulb(float dt);
public:
    MandelbulbViewer(ConfFile& conf);
    ~MandelbulbViewer();

    void logic(float t, float dt);
    void draw(float t, float dt);

    void createVideo(std::string filename, int video_framerate);

    void saveRecording();

    //inherited methods
    void init();
    void update(float t, float dt);
    void keyPress(SDL_KeyboardEvent *e);
    void mouseMove(SDL_MouseMotionEvent *e);
    void mouseClick(SDL_MouseButtonEvent *e);
};

#endif
