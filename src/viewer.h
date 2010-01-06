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

#include "core/sdlapp.h"
#include "core/display.h"
#include "core/shader.h"
#include "core/fxfont.h"
#include "core/pi.h"

#include "vcamera.h"

class MandelbulbViewer : public SDLApp {

    Shader* shader;
    FXFont font;

    bool debug;

    std::string shaderfile;

    void drawAlignedQuad();

    float time_elapsed;

    bool play;
    bool record;
    int frame_skip;
    int frame_count;
    float frame_delta;

    bool record_frame;

    ViewCameraPath campath;

    bool paused;
    bool animated;

    ViewCamera view;
    Object3D mandelbulb;

    mat3f viewRotation;
    mat3f objRotation;

    bool juliaset;
    vec3f juliaseed;


    float power;
    int maxIterations;

    vec4f  backgroundColor;
    vec4f  diffuseColor;
    vec4f  ambientColor;
    vec4f  lightColor;

    void randomizeJuliaSeed();
    void randomizeColours();

    void togglePlay();
    void toggleRecord();

    void addWaypoint(float duration);

    void moveCam(float dt);
public:
    MandelbulbViewer();
    ~MandelbulbViewer();

    void logic(float t, float dt);
    void draw(float t, float dt);

    //inherited methods
    void init();
    void update(float t, float dt);
    void keyPress(SDL_KeyboardEvent *e);
    void mouseMove(SDL_MouseMotionEvent *e);
    void mouseClick(SDL_MouseButtonEvent *e);
};

#endif