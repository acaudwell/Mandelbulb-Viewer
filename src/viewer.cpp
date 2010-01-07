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

#include "viewer.h"

int main(int argc, char *argv[]) {

    int width  = 1024;
    int height = 768;
    bool fullscreen=false;
    bool multisample=false;

    for (int i=0; i<argc; i++) {
        if(i==0) continue;

        std::string args(argv[i]);

        if(args == "--multi-sampling") {
            multisample = true;
            continue;
        }
    }

    SDLAppParseArgs(argc, argv, &width, &height, &fullscreen);

    display.enableShaders(true);

    if(multisample) {
        display.multiSample(4);
    }

    display.enableVsync(true);

    display.init("Mandelbulb Viewer", width, height, fullscreen);

    if(multisample) glEnable(GL_MULTISAMPLE_ARB);

    MandelbulbViewer* viewer = new MandelbulbViewer();
    viewer->run();

    delete viewer;

    display.quit();

    return 0;
}

MandelbulbViewer::MandelbulbViewer() : SDLApp() {
    shaderfile = "MandelbulbQuick";

    shader = 0;
    time_elapsed = 0;
    paused = false;

    mandelbulb.setPos(vec3f(0.0, 0.0, 0.0));
    mandelbulb.rotateX(90.0f * DEGREES_TO_RADIANS);

    view.setPos(vec3f(0.0, 0.0, 2.6));

    power = 8.0f;
    maxIterations = 6;

    play = false;
    record = false;

    frame_count = 0;
    frame_skip  = 30;
    frame_delta = 0.0;

    animated = false;
    juliaset = false;
    backgroundGradient = true;

    //todo: config file for defaults?

    backgroundColor = vec4f(0.0, 0.0, 0.0, 1.0);
    diffuseColor    = vec4f(0.0, 0.85, 0.99, 1.0);
    ambientColor    = vec4f(0.67, 0.85, 1.0, 1.0);
    lightColor      = vec4f(0.48, 0.59, 0.66, 1.0);

    srand(time(0));

    randomizeJuliaSeed();

    //ignore mouse motion until we have finished setting up
    SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
}

MandelbulbViewer::~MandelbulbViewer() {
    if(shader != 0) delete shader;
}

void MandelbulbViewer::randomizeJuliaSeed() {
    juliaseed = vec3f( rand() % 1000, rand() % 1000, rand() % 1000 ) / 1000.0f;
}

void MandelbulbViewer::randomizeColours() {
    backgroundColor = vec4f(vec3f(rand() % 100, rand() % 100, rand() % 100).normal(), 1.0);
    diffuseColor = vec4f(vec3f(rand() % 100, rand() % 100, rand() % 100).normal(), 1.0);
    ambientColor = vec4f(vec3f(rand() % 100, rand() % 100, rand() % 100).normal(), 1.0);
    lightColor   = vec4f(vec3f(rand() % 100, rand() % 100, rand() % 100).normal(), 1.0);
}

void MandelbulbViewer::init() {
    display.setClearColour(vec3f(0.0, 0.0, 0.0));

    SDL_ShowCursor(false);
    SDL_WM_GrabInput(SDL_GRAB_ON);

    shader = shadermanager.grab(shaderfile);

    font = fontmanager.grab("FreeSans.ttf", 16);
    font.dropShadow(true);

    //we are ready receive mouse motion events now
    SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
}

void MandelbulbViewer::keyPress(SDL_KeyboardEvent *e) {

    if (e->type == SDL_KEYDOWN) {

        if (e->keysym.sym == SDLK_ESCAPE) {
            appFinished=true;
        }

        if (e->keysym.sym == SDLK_r) {
            toggleRecord();
        }

        if (e->keysym.sym == SDLK_p) {
            togglePlay();
        }

        if (e->keysym.sym == SDLK_o) {
            addWaypoint(1.0);
        }

        if (e->keysym.sym ==  SDLK_c) {
            randomizeColours();
        }

        if (e->keysym.sym ==  SDLK_b) {
            backgroundGradient = !backgroundGradient;
        }

        if (e->keysym.sym ==  SDLK_j) {
            juliaset = !juliaset;
        }

        if (e->keysym.sym ==  SDLK_k) {
            randomizeJuliaSeed();
        }

        if(e->keysym.sym ==  SDLK_h) {
            animated = !animated;
        }

        if (e->keysym.sym ==  SDLK_LEFTBRACKET) {
            if(maxIterations>1) maxIterations--;
        }

        if (e->keysym.sym ==  SDLK_RIGHTBRACKET) {
            maxIterations++;
        }

        if (e->keysym.sym ==  SDLK_MINUS) {
            if(power>1.0) power -= 1.0;
        }

        if (e->keysym.sym ==  SDLK_EQUALS) {
            power += 1.0;
        }

        if (e->keysym.sym == SDLK_q) {
            debug = !debug;
        }

        if(e->keysym.sym == SDLK_SPACE) {
            paused = !paused;
        }
    }

}

void MandelbulbViewer::toggleRecord() {
    if(play) return;

    record = !record;

    frame_delta = 0.0;

    //start new recording
    if(record) {
        campath.clear();
    }
}

void MandelbulbViewer::togglePlay() {
    if(record) return;

    play = !play;
    campath.reset();
}

void MandelbulbViewer::addWaypoint(float duration) {
    if(campath.size()==0) duration = 0.0;

    ViewCameraMoveEvent* e = new ViewCameraMoveEvent(view, duration);
    campath.addEvent(e);
}
void MandelbulbViewer::mouseMove(SDL_MouseMotionEvent *e) {

    //debugLog("mouseMove %d %d\n", e->xrel, e->yrel);

    view.rotateY((e->xrel / 10.0f) * DEGREES_TO_RADIANS);
    view.rotateX((e->yrel / 10.0f) * DEGREES_TO_RADIANS);
}

void MandelbulbViewer::mouseClick(SDL_MouseButtonEvent *e) {
}

void MandelbulbViewer::moveCam(float dt) {

    mat3f camRotation = view.getRotationMatrix();

    vec3f campos = view.getPos();

    float cam_distance = campos.length2();
    cam_distance *= cam_distance;

    float amount = std::min(1.0f, cam_distance) * dt;

    Uint8* keyState = SDL_GetKeyState(NULL);

    bool forward  = keyState[SDLK_w];
    bool left     = keyState[SDLK_a];
    bool backward = keyState[SDLK_s];
    bool right    = keyState[SDLK_d];
    bool up       = keyState[SDLK_UP];
    bool down     = keyState[SDLK_DOWN];

    if(left || right) {
        vec3f inc = camRotation.X() * amount;
        if(right) campos += inc;
        else campos -= inc;
    }

    if(up || down) {
        vec3f inc = camRotation.Y() * amount;
        if(up) campos += inc;
        else campos -= inc;
    }

    if(forward || backward) {
        vec3f inc = camRotation.Z() * amount;
        if(forward) campos += inc;
        else campos -= inc;
    }

    view.setPos(campos);
}

void MandelbulbViewer::drawAlignedQuad() {

    glPushMatrix();

    int w = display.width;
    int h = display.height;

    glBegin(GL_QUADS);
        glTexCoord2i(1,-1);
        glVertex2i(w,h);

        glTexCoord2i(-1,-1);
        glVertex2i(0,h);

        glTexCoord2i(-1,1);
        glVertex2i(0,0);

        glTexCoord2i(1,1);
        glVertex2i(w,0);
/*
        glTexCoord2i(1,1);
        glVertex2i(0,0);

        glTexCoord2i(-1,1);
        glVertex2i(w,0);

        glTexCoord2i(-1,-1);
        glVertex2i(w,h);

        glTexCoord2i(1,-1);
        glVertex2i(0,h);
*/
    glEnd();

    glPopMatrix();
}

void MandelbulbViewer::update(float t, float dt) {
    dt = std::min(dt, 1.0f/60.0f);

    logic(t,dt);
    draw(t, dt);
}

void MandelbulbViewer::logic(float t, float dt) {
    if(play) {
        campath.logic(dt, &view);
        if(campath.isFinished()) play = false;
    } else {
        moveCam(dt);
    }

    if(record) {
        frame_delta += dt;
        if(frame_count % frame_skip == 0) {
            addWaypoint(frame_delta);
            frame_delta = 0.0;
        }
    }

//    float amount = 90 * dt;
//    mandelbulb.rotateY(dt * 90.0f * DEGREES_TO_RADIANS);

    viewRotation = view.getRotationMatrix();
    frame_count++;
}

void MandelbulbViewer::draw(float t, float dt) {
    display.clear();

    if(!paused) {
        time_elapsed += dt;
    }

    vec3f _juliaseed = juliaseed;

    if(animated) {
        _juliaseed = _juliaseed + vec3f(sinf(time_elapsed), sinf(time_elapsed), atan(time_elapsed)) * 0.1;
    }

    vec3f campos = view.getPos();

    float bounding = 3.0f;

    //to avoid a visible sphere we need to set the bounding
    //sphere to be greater than the camera's distance from the
    //origin
    if(backgroundGradient) {
        bounding = std::max(bounding, campos.length2());
    }

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    display.mode2D();

    shader->use();
    shader->setFloat("width",  display.width);
    shader->setFloat("height", display.height);

    shader->setVec3("camera",         campos);
    shader->setVec3("cameraFine",     vec3f(0.0f, 0.0f, 0.0f));
    shader->setFloat("cameraZoom",    0.0f);

    shader->setInteger("julia", juliaset);
    shader->setVec3("julia_c", _juliaseed);

    shader->setInteger("radiolaria", 0);
    shader->setFloat("radiolariaFactor", 0.0f);

    shader->setFloat("power", power);
    shader->setFloat("bounding", bounding );
    shader->setFloat("bailout", 4.0f );

    shader->setInteger("antialiasing", 0);
    shader->setInteger("phong", 1);
    shader->setFloat("shadows", 0.0f);
    shader->setFloat("ambientOcclusion", 0.8f);
    shader->setFloat("ambientOcclusionEmphasis", 0.58f);
    shader->setFloat("colorSpread",      0.2f);
    shader->setFloat("rimLight",         0.0f);
    shader->setFloat("specularity",      0.0f);
    shader->setFloat("specularExponent", 15.0f);

    shader->setVec3("light", vec3f(38, -42, 38));

    shader->setVec4("backgroundColor", backgroundColor);
    shader->setVec4("diffuseColor",    diffuseColor);
    shader->setVec4("ambientColor",    ambientColor);
    shader->setVec4("lightColor",      lightColor);

    shader->setMat3("viewRotation", viewRotation);
    shader->setMat3("objRotation",  mandelbulb.getRotationMatrix());

    shader->setInteger("maxIterations", maxIterations);
    shader->setInteger("stepLimit",     110);
    shader->setFloat("epsilonScale",    1.0);

    shader->setInteger("backgroundGradient", backgroundGradient);
    shader->setFloat("fov",  45.0);

    drawAlignedQuad();

    glUseProgramObjectARB(0);
//    glActiveTextureARB(GL_TEXTURE0);

    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    if(record) {
        glColor4f(1.0, 0.0, 0.0, 1.0);
        font.print(0, 0, "Recording %d", campath.size());
    }

    if(play) {
        glColor4f(0.0, 1.0, 0.0, 1.0);
        font.print(0, 0, "Playing %d / %d", campath.getIndex(), campath.size());
    }

    glColor4f(1.0, 1.0, 1.0, 1.0);

    if(debug) {
        font.print(0, 0, "fps: %.2f", fps);
        font.print(0, 20, "camera: %.2f,%.2f,%.2f", campos.x, campos.y, campos.z);
        font.print(0, 40, "power: %.2f", power);
        if(juliaset) {
            font.print(0, 80, "juliaset seed: %.2f,%.2f,%.2f", _juliaseed.x, _juliaseed.y, _juliaseed.z);
        }
    }

}
