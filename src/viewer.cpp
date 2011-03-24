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

    std::string ppm_file_name;
    int video_framerate = 60;

    std::vector<std::string> conffiles;

    SDLAppInit("Mandelbulb", "mandelbulb");

    ConfFile conf;

    try {
        gViewerSettings.parseArgs(argc, argv, conf, &conffiles);

        if(conffiles.size()>0) {
            std::string conffile = conffiles[conffiles.size()-1];
            conf.load(conffile);
            gViewerSettings.parseArgs(argc, argv, conf, &conffiles);
        }

        gViewerSettings.importDisplaySettings(conf);
        gViewerSettings.importViewerSettings(conf);

    } catch(ConfFileException& exception) {
        SDLAppQuit(exception.what());
    }

    display.enableShaders(true);

    if(gViewerSettings.multisample) {
        display.multiSample(4);
    }

    display.enableVsync(true);

    display.init("Mandelbulb Viewer", gViewerSettings.display_width, gViewerSettings.display_height, gViewerSettings.fullscreen);

    if(gViewerSettings.multisample) glEnable(GL_MULTISAMPLE_ARB);

    MandelbulbViewer* viewer = 0;

    try {
        viewer = new MandelbulbViewer(conf);

        if(gViewerSettings.output_ppm_filename.size()) {
            viewer->createVideo(gViewerSettings.output_ppm_filename, gViewerSettings.output_framerate);
        }

        viewer->run();

    } catch(ResourceException& exception) {

        char errormsg[1024];
        snprintf(errormsg, 1024, "failed to load resource '%s'", exception.what());

        SDLAppQuit(errormsg);

    } catch(SDLAppException& exception) {

        if(exception.showHelp()) {
            gViewerSettings.help();
        } else {
            SDLAppQuit(exception.what());
        }
    } catch(PPMExporterException& exception) {

        char errormsg[1024];
        snprintf(errormsg, 1024, "could not write to '%s'", exception.what());

        SDLAppQuit(errormsg);
    }

    if(viewer != 0) delete viewer;

    display.quit();

    return 0;
}

MandelbulbViewer::MandelbulbViewer(ConfFile& conf) : SDLApp() {
    shaderfile = "MandelbulbQuick";

    debug = false;

    mousemove = false;

    shader = 0;
    time_elapsed = 0;
    paused = false;

    mandelbulb.setPos(vec3f(0.0, 0.0, 0.0));
    mandelbulb.rotateX(90.0f * DEGREES_TO_RADIANS);

    view.setPos(vec3f(0.0, 0.0, 2.6));

    beatCount     = 0;
    beatTimer     = 0.0;
    beatGlowDepth = 0.0;
    beatGlowMulti = 0.0;

    pulse = -1.0;

    play = false;
    record = false;

    mouselook = false;
    roll      = false;

    TextureResource* cursor_texture = texturemanager.grab("cursor.png");

    cursor.useSystemCursor(false);
    cursor.showCursor(true);
    cursor.setCursorTexture(cursor_texture);

    scanline_count      = 0;
    scanline_batch_size = 0;
    
    scanline_target_fps = 30.0f;
    scanline_target_rps = 10.0f;
    
    scanline_mode       = true;
    scanline_debug      = false;

    runtime = 0.0;
    frame_skip = 0;
    frame_count = 0;
    fixed_tick_rate = 0.0;

    frameExporter = 0;
    record_frame_skip  = 10.0;
    record_frame_delta = 0.0;

    message_timer = 0.0;

    srand(time(0));

    randomizeJuliaSeed();

    //ignore mouse motion until we have finished setting up
    SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

    //load recording
    if(conf.hasSection("camera")) {
        campath.load(conf);
        play=true;
    }

    render_width  = display.width;
    render_height = display.height;

    if(gViewerSettings.viewscale != 1.0f) {
        render_width  *= gViewerSettings.viewscale;
        render_height *= gViewerSettings.viewscale;
    }
}

MandelbulbViewer::~MandelbulbViewer() {
    if(shader != 0) delete shader;
    if(frameExporter != 0) delete frameExporter;
}

void MandelbulbViewer::createVideo(std::string filename, int video_framerate) {
    if(campath.size()==0) {
        SDLAppQuit("nothing to record");
    }

    int fixed_framerate = video_framerate;

    frame_count = 0;
    frame_skip  = 0;

    //calculate appropriate tick rate for video frame rate
    /*
    while(fixed_framerate<60) {
        fixed_framerate += video_framerate;
        this->frame_skip++;
    }
    */

    this->fixed_tick_rate = 1.0f / ((float) fixed_framerate);

    this->frameExporter = new PPMExporter(filename);
}

void MandelbulbViewer::randomizeJuliaSeed() {
    gViewerSettings.julia_c = vec3f( rand() % 1000, rand() % 1000, rand() % 1000 ).normal();
}

void MandelbulbViewer::randomizeColours() {
    gViewerSettings.backgroundColor = vec4f(vec3f(rand() % 100, rand() % 100, rand() % 100).normal(), 1.0);
    gViewerSettings.diffuseColor = vec4f(vec3f(rand() % 100, rand() % 100, rand() % 100).normal(), 1.0);
    gViewerSettings.ambientColor = vec4f(vec3f(rand() % 100, rand() % 100, rand() % 100).normal(), 1.0);
    gViewerSettings.lightColor   = vec4f(vec3f(rand() % 100, rand() % 100, rand() % 100).normal(), 1.0);
    gViewerSettings.glowColour   = vec3f(rand() % 100, rand() % 100, rand() % 100).normal();
}

void MandelbulbViewer::init() {
    display.setClearColour(vec3f(0.0, 0.0, 0.0));

    shader = shadermanager.grab(shaderfile);

    rendertex = display.emptyTexture(display.width, display.height, GL_RGBA);
    frametex  = display.emptyTexture(display.width, display.height, GL_RGBA);

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

        if (e->keysym.sym == SDLK_1) {
            gViewerSettings.pulsate = !gViewerSettings.pulsate;
        }

        if (e->keysym.sym == SDLK_2) {
            gViewerSettings.rave = !gViewerSettings.rave;
        }

        if (e->keysym.sym == SDLK_3) {
            gViewerSettings.pulsateFov = !gViewerSettings.pulsateFov;
        }

        if (e->keysym.sym == SDLK_F11) {
            gViewerSettings.beat = gViewerSettings.beat > 0.0f ? 0.0f : 0.218f;
        }

        if (e->keysym.sym == SDLK_r) {
            toggleRecord();
        }

        if(e->keysym.sym == SDLK_e) {
            resetCamPath();
        }

        if (e->keysym.sym == SDLK_p) {
            togglePlay();
        }

        if (e->keysym.sym == SDLK_f) {
            if(!record) addWaypoint(-1.0);
        }

        if(e->keysym.sym == SDLK_g) {
            removeWaypoint();
        }

        if (e->keysym.sym ==  SDLK_c) {
            randomizeColours();
        }

        if (e->keysym.sym ==  SDLK_v) {
            gViewerSettings.constantSpeed = !gViewerSettings.constantSpeed;
        }

        if (e->keysym.sym ==  SDLK_b) {
            gViewerSettings.backgroundGradient = !gViewerSettings.backgroundGradient;
        }

        if (e->keysym.sym ==  SDLK_j) {
            gViewerSettings.juliaset = !gViewerSettings.juliaset;
        }

        if (e->keysym.sym ==  SDLK_k) {
            randomizeJuliaSeed();
        }

        if(e->keysym.sym ==  SDLK_h) {
            gViewerSettings.animated = !gViewerSettings.animated;
        }

        if (e->keysym.sym ==  SDLK_LEFTBRACKET) {
            gViewerSettings.epsilonScale = std::max( gViewerSettings.epsilonScale / 1.1, 0.00001);
        }

        if (e->keysym.sym ==  SDLK_RIGHTBRACKET) {
            gViewerSettings.epsilonScale = std::min( gViewerSettings.epsilonScale * 1.1, 2.0);
        }

        if (e->keysym.sym ==  SDLK_COMMA) {
            gViewerSettings.aoSteps /= 1.1f;
        }

        if (e->keysym.sym ==  SDLK_PERIOD) {
            gViewerSettings.aoSteps *= 1.1f;
        }

        if (e->keysym.sym ==  SDLK_F1) {
            if(gViewerSettings.maxIterations>1) gViewerSettings.maxIterations--;
        }

        if (e->keysym.sym ==  SDLK_F2) {
            gViewerSettings.maxIterations++;
        }

        if (e->keysym.sym ==  SDLK_F3) {
            gViewerSettings.fogDistance -= 0.1;
        }

        if (e->keysym.sym ==  SDLK_F4) {
            gViewerSettings.fogDistance += 0.1;
        }

        if (e->keysym.sym ==  SDLK_F5) {
            gViewerSettings.glowDepth -= 0.1;
        }

        if (e->keysym.sym ==  SDLK_F6) {
            gViewerSettings.glowDepth += 0.1;
        }

        if (e->keysym.sym ==  SDLK_F7) {
            gViewerSettings.glowMulti /= 1.1;
        }

        if (e->keysym.sym ==  SDLK_F8) {
            gViewerSettings.glowMulti *= 1.1;
        }

        if (e->keysym.sym ==  SDLK_F9) {
            if(!(play||record)) saveRecording();
        }

        if (e->keysym.sym ==  SDLK_HOME) {
            if(gViewerSettings.fov>1) gViewerSettings.fov -= 1 ;
        }

        if (e->keysym.sym ==  SDLK_END) {
            if(gViewerSettings.fov<180) gViewerSettings.fov += 1;
        }

        if (e->keysym.sym ==  SDLK_MINUS) {
//            if(power>1.0) power -= 1.0;
            gViewerSettings.power -= 1.0;
        }

        if (e->keysym.sym ==  SDLK_EQUALS) {
            gViewerSettings.power += 1.0;
        }

        if (e->keysym.sym == SDLK_q) {
            debug = !debug;
        }

        if (e->keysym.sym == SDLK_z) {
            scanline_debug = !scanline_debug;
        }

        if(e->keysym.sym == SDLK_SPACE) {
            paused = !paused;
        }
    }

}

void MandelbulbViewer::setMessage(const std::string& message, const vec3f& colour) {
    this->message = message;
    message_colour = colour;
    message_timer = 10.0;
}

void MandelbulbViewer::saveRecording() {

    ConfFile conf;

    //get next free recording name
    char recname[256];
    struct stat finfo;
    int recno = 1;

    while(recno < 10000) {
        snprintf(recname, 256, "mandelbulb-%04d.mdb", recno);
        if(stat(recname, &finfo) != 0) break;
        recno++;
    }

    conf.setFilename(recname);

    gViewerSettings.exportDisplaySettings(conf);
    gViewerSettings.exportViewerSettings(conf);
    campath.save(conf);

    try {
        conf.save();
    } catch(ConfFileException& exception) {
        SDLAppQuit(exception.what());
    }

    setMessage("Wrote " + conf.getFilename());
}

void MandelbulbViewer::resetCamPath() {
    if(play || record) return;

    campath.clear();

    setMessage("Camera path reset");
}

void MandelbulbViewer::toggleRecord() {
    if(play) return;

    record = !record;
    record_frame_delta = 0.0;

    //start new recording
    if(record) {
        campath.clear();
        setMessage("Recording", vec3f(1.0, 0.0, 0.0));
    } else {
        saveRecording();
    }
}

void MandelbulbViewer::togglePlay() {
    if(record) return;

    play = !play;
    campath.reset();
}

void MandelbulbViewer::removeWaypoint() {
    if(record || play) return;

    if(campath.size()>0) {
        int size = campath.size();

        campath.deleteLast();

        char msgbuff[256];
        snprintf(msgbuff, 256, "Deleted Waypoint %d", campath.size());
        setMessage(msgbuff);

        campath.getLastCamera(view);
    }
}

void MandelbulbViewer::addWaypoint(float duration) {
    if(play) return;

    if(campath.size()==0) duration = 0.0;

    //calculate duration from distance
    if(duration<0.0) {
        campath.setUnitsPerSecond(gViewerSettings.speed);
    } else {
        campath.setUnitsPerSecond(-1.0f);
    }

    ViewCameraEvent* e = new ViewCameraEvent(view, duration);

    campath.addEvent(e);

    vec3f red(1.0, 0.0, 0.0);

    char msgbuff[256];
    snprintf(msgbuff, 256, "%s %d", "Added Waypoint", campath.size());

    setMessage(msgbuff, red);

}
void MandelbulbViewer::mouseMove(SDL_MouseMotionEvent *e) {

    mousemove = true;

    //debugLog("mouseMove %d %d\n", e->xrel, e->yrel);
    if(mouselook) {
        int warp_x = display.width/2;
        int warp_y = display.height/2;

         //this is an even we generated by warping the mouse below
         if(e->x == warp_x && e->y == warp_y) return;

        SDL_WarpMouse(warp_x, warp_y);

        if(roll) {
            view.rotateZ(-(e->xrel / 10.0f) * DEGREES_TO_RADIANS);
            view.rotateX((e->yrel / 10.0f) * DEGREES_TO_RADIANS);
        } else {
            view.rotateY((e->xrel / 10.0f) * DEGREES_TO_RADIANS);
            view.rotateX((e->yrel / 10.0f) * DEGREES_TO_RADIANS);
        }
   } else {
        cursor.updatePos(vec2f(e->x, e->y));
   }
}

void MandelbulbViewer::mouseClick(SDL_MouseButtonEvent *e) {

    if(e->state == SDL_PRESSED) {
        if(e->button == SDL_BUTTON_RIGHT) {
            //save mouse position
            mousepos = vec2f(e->x, e->y);

            mouselook=true;
            cursor.showCursor(false);
        }

        if(e->button == SDL_BUTTON_LEFT && mouselook) {
            roll = true;
        }

        if(e->button == SDL_BUTTON_WHEELUP) {
            gViewerSettings.speed *= 2.0;
        }

        if(e->button == SDL_BUTTON_WHEELDOWN) {
            gViewerSettings.speed /= 2.0;
        }

    }

    if(e->state == SDL_RELEASED) {
        if(e->button == SDL_BUTTON_LEFT && mouselook) {
            roll = false;
        }

        if(e->button == SDL_BUTTON_RIGHT) {
            mouselook=false;
            cursor.showCursor(true);

            //warp to last position
            SDL_WarpMouse(mousepos.x, mousepos.y);
            cursor.updatePos(mousepos);
        }
    }

}

void MandelbulbViewer::moveCam(float dt) {

    mat3f camRotation = view.getRotationMatrix();

    vec3f campos = view.getPos();

    float cam_distance = campos.length2();
    cam_distance *= cam_distance;

    float amount = gViewerSettings.constantSpeed ?
        gViewerSettings.speed * dt : gViewerSettings.speed * std::min(1.0f, cam_distance) * dt;

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

void MandelbulbViewer::drawAlignedQuad(int w, int h) {

    glPushMatrix();

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
    //dt = std::max(dt, 1.0f/25.0f);

    //if exporting a video use a fixed tick rate rather than time based
    if(frameExporter != 0) dt = fixed_tick_rate;
    dt *= gViewerSettings.timescale;

    runtime += dt;

    logic(runtime, dt);
    draw(runtime, dt);

    //extract frames based on frameskip setting
    //if frameExporter defined
    if(frameExporter != 0) {
        if(frame_count % (frame_skip+1) == 0) {
            frameExporter->dump();
        }
    }

    if(!scanline_mode || scanline_count >= render_height) {
        if(scanline_mode) scanline_count = 0;
        frame_count++;
    }

    cursor.logic(dt);
    cursor.draw();
}

void MandelbulbViewer::logic(float t, float dt) {

    if(scanline_mode && scanline_count > 0) {
        //drawing previous frame
        moveCam(dt);
        return;
    }

    if(play) {
        campath.logic(dt, &view);
        if(campath.isFinished()) {
            play = false;
            if(frameExporter!=0) {
                appFinished=true;
                return;
            }
        }
    } else {
        moveCam(dt);
    }

    //set render width/height
    render_width  = display.width * gViewerSettings.viewscale;
    render_height = display.height * gViewerSettings.viewscale;

    if(mouselook) {
        render_width  *= 0.25;
        render_height *= 0.25;
    }

    //roll doesnt make any sense unless mouselook is enabled
    if(!mouselook) {
        roll = false;
    }

    if(record) {
        record_frame_delta += dt;

        if(mousemove || frame_count % record_frame_skip == 0) {
            addWaypoint(record_frame_delta);
//            addWaypoint(-1.0);
            record_frame_delta = 0.0;
        }
    }

    mousemove = false;

    viewRotation = view.getRotationMatrix();

    if(paused) return;

    time_elapsed += dt;

    //update beat
    if(gViewerSettings.beat>0.0) {
        beatTimer += dt;

        if(beatTimer>gViewerSettings.beat*2.0) {
            beatTimer=0.0;
            beatCount++;

            if(gViewerSettings.beatPeriod>0 && beatCount % gViewerSettings.beatPeriod == 0) {
                gViewerSettings.glowColour = vec3f(rand() % 100, rand() % 100, rand() % 100).normal();
            }
        }

        pulse = beatTimer/gViewerSettings.beat;

        if(pulse>1.0) pulse = 2.0-pulse;

        beatGlowDepth = gViewerSettings.glowDepth * 0.5 + 0.5 * gViewerSettings.glowDepth * pulse;
        beatGlowMulti = gViewerSettings.glowMulti * 0.5 + 0.5 * gViewerSettings.glowMulti * pulse;
    }

    //update julia seed
    _julia_c = gViewerSettings.julia_c;

    if(gViewerSettings.animated) {
        _julia_c = gViewerSettings.julia_c + vec3f(sinf(time_elapsed), sinf(time_elapsed), atan(time_elapsed)) * 0.1;
    }

    //to avoid a visible sphere we need to set the bounding
    //sphere to be greater than the camera's distance from the
    //origin
    if(gViewerSettings.backgroundGradient) {
        gViewerSettings.bounding = std::max(gViewerSettings.bounding, view.getPos().length2());
    }

    mandelbulb.rotateX(gViewerSettings.rotation.x * dt * DEGREES_TO_RADIANS);
    mandelbulb.rotateY(gViewerSettings.rotation.y * dt * DEGREES_TO_RADIANS);
    mandelbulb.rotateZ(gViewerSettings.rotation.z * dt * DEGREES_TO_RADIANS);

    frame_count++;

    if(play) {
        vec3f green(0.0, 1.0, 0.0);

        char msgbuff[256];
        snprintf(msgbuff, 256, "Playing %d / %d", campath.getIndex(), campath.size());
        setMessage(std::string(msgbuff), green);
    }

    if(message_timer>0.0) message_timer -= dt;
}


void MandelbulbViewer::drawMandelbulb(float dt) {

    vec3f campos = view.getPos();

    display.mode2D();

    //draw partially drawn image
    if(scanline_mode) {

        //on initial frame capture the background colour onto the render tex / frame tex
        if(frame_count == 0 && scanline_count == 0) {
            glEnable(GL_TEXTURE_2D);

            glBindTexture(GL_TEXTURE_2D, rendertex);
            glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, render_width, render_height, 0);

            glBindTexture(GL_TEXTURE_2D, frametex);
            glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, display.width, display.height, 0);
        }

        //draw partial frame
        if(scanline_count > 0) {

            glEnable(GL_TEXTURE_2D);
            glDisable(GL_BLEND);

            glBindTexture(GL_TEXTURE_2D, rendertex);

            glBegin(GL_QUADS);
                glTexCoord2i(1,0);
                glVertex2i(render_width, render_height);

                glTexCoord2i(0,0);
                glVertex2i(0, render_height);

                glTexCoord2i(0,1);
                glVertex2i(0,0);

                glTexCoord2i(1,1);
                glVertex2i(render_width,0);
            glEnd();
        }
    }


    //enable shader
    shader->use();

    //configure shader
    shader->setFloat("width",  render_width);
    shader->setFloat("height", render_height);

    shader->setVec3("camera",         campos);
    shader->setVec3("cameraFine",     vec3f(0.0f, 0.0f, 0.0f));
    shader->setFloat("cameraZoom",    gViewerSettings.cameraZoom);

    shader->setInteger("julia", gViewerSettings.juliaset);
    shader->setVec3("julia_c",  _julia_c);

    shader->setInteger("radiolaria", gViewerSettings.radiolaria);
    shader->setFloat("radiolariaFactor", gViewerSettings.radiolariaFactor);

    shader->setFloat("power", gViewerSettings.power);

    shader->setFloat("bounding", gViewerSettings.bounding );
    shader->setFloat("bailout",  gViewerSettings.bailout );

    shader->setInteger("antialiasing", gViewerSettings.antialiasing);

    shader->setInteger("phong", gViewerSettings.phong);
    shader->setFloat("shadows", gViewerSettings.shadows);

    shader->setFloat("ambientOcclusion", gViewerSettings.ambientOcclusion);
    shader->setFloat("ambientOcclusionEmphasis", gViewerSettings.ambientOcclusionEmphasis);

    shader->setFloat("colorSpread",      gViewerSettings.colorSpread);
    shader->setFloat("rimLight",         gViewerSettings.rimLight);
    shader->setFloat("specularity",      gViewerSettings.specularity);
    shader->setFloat("specularExponent", gViewerSettings.specularExponent);

    shader->setVec3("light", gViewerSettings.light);

    shader->setVec4("backgroundColor", gViewerSettings.backgroundColor);
    shader->setVec4("diffuseColor",    gViewerSettings.diffuseColor);
    shader->setVec4("ambientColor",    gViewerSettings.ambientColor);
    shader->setVec4("lightColor",      gViewerSettings.lightColor);

    shader->setMat3("viewRotation", viewRotation);
    shader->setMat3("objRotation",  mandelbulb.getRotationMatrix());

    shader->setInteger("maxIterations", gViewerSettings.maxIterations);
    shader->setInteger("stepLimit",     gViewerSettings.stepLimit);
    shader->setFloat("epsilonScale",    gViewerSettings.epsilonScale);

    shader->setFloat("aoSteps", gViewerSettings.aoSteps);

    shader->setFloat("fogDistance", gViewerSettings.fogDistance);


    if(gViewerSettings.beat>0.0) {
        shader->setFloat("glowDepth", beatGlowDepth);
        shader->setFloat("glowMulti", beatGlowMulti);
    } else {
        shader->setFloat("glowDepth", gViewerSettings.glowDepth);
        shader->setFloat("glowMulti", gViewerSettings.glowMulti);
    }

    shader->setInteger("Rave", gViewerSettings.rave);
    shader->setFloat("Pulse", gViewerSettings.pulsate ? pulse : -1.0f);
    shader->setFloat("PulseScale", gViewerSettings.pulseScale);

    shader->setVec3("glowColour", gViewerSettings.glowColour);


    shader->setInteger("backgroundGradient", gViewerSettings.backgroundGradient);

    if(!gViewerSettings.pulsateFov) {
        shader->setFloat("fov", gViewerSettings.fov);
    } else {
        float pulse_fov = gViewerSettings.fov * sinf(pulse*0.5+0.5) * gViewerSettings.pulseFovScale;

        shader->setFloat("fov", pulse_fov);
    }

    //set clipping area to area of scanline_batch_size
    if(scanline_mode) {

        //TODO: calculate based on previous value + dt ?

        if(scanline_batch_size == 0) {

            scanline_batch_size = 1;

        } else {
            //adjust the number of scanlines to render
            //based on whether or not we are meeting our rps/fps targets
            
            
            //try to meet rps target
            if(((float)render_height)*scanline_target_rps > ((float)scanline_batch_size)/dt) {
                scanline_batch_size++;

            //try to meet fps target
            } else if(dt > (1.0f/scanline_target_fps))
                scanline_batch_size--;
            else
                scanline_batch_size++;

            if(scanline_batch_size<1) scanline_batch_size = 1;
        }

        //render the minimum of the remaining number of lines, and the batch size
        int lines_to_render = std::min(scanline_batch_size, render_height - scanline_count);

        glEnable(GL_SCISSOR_TEST);
        glScissor(0, display.height - render_height + scanline_count, render_width, lines_to_render);

        scanline_count += lines_to_render;
    }

    //render
    drawAlignedQuad(render_width, render_height);

    //disable clipping
    if(scanline_mode) glDisable(GL_SCISSOR_TEST);

    //stop using shader
    glUseProgramObjectARB(0);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    bool resize_frame = display.width != render_width && display.height != render_height;

    if(scanline_mode || resize_frame) {

        //render to either working or completed texture depending on progress
        GLuint render_target = rendertex;

        if(scanline_mode && scanline_count >= render_height) {
            render_target = frametex;
        } else {
            render_target = rendertex;
        }

        glBindTexture(GL_TEXTURE_2D, render_target);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, display.height - render_height, render_width, render_height, 0);

        display.mode2D();

        //redraw last finished frame
        if(scanline_mode && (scanline_debug || render_target != frametex) || resize_frame) {

            if(scanline_mode && render_target != frametex)
                glBindTexture(GL_TEXTURE_2D, frametex);

            glColor4f(1.0f, 1.0f, 1.0f, scanline_debug ? 0.5f : 1.0f);

            glBegin(GL_QUADS);
                glTexCoord2i(1,0);
                glVertex2i(display.width,display.height);

                glTexCoord2i(0,0);
                glVertex2i(0,display.height);

                glTexCoord2i(0,1);
                glVertex2i(0,0);

                glTexCoord2i(1,1);
                glVertex2i(display.width,0);
            glEnd();

            //draw the rendered portion over the top so we can see the progress
            if(scanline_mode && scanline_debug && render_target != frametex) {
                glBindTexture(GL_TEXTURE_2D, render_target);

                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

                glEnable(GL_BLEND);
                glBlendFunc(GL_ONE, GL_ONE);

                glBegin(GL_QUADS);
                    glTexCoord2i(1,0);
                    glVertex2i(display.width,display.height);

                    glTexCoord2i(0,0);
                    glVertex2i(0,display.height);

                    glTexCoord2i(0,1);
                    glVertex2i(0,0);

                    glTexCoord2i(1,1);
                    glVertex2i(display.width,0);
                glEnd();
            }

        }

    }

    // TODO: why is the first frame corrupt?
    if(scanline_mode && frame_count<2) display.clear();
}

void MandelbulbViewer::draw(float t, float dt) {
    if(appFinished) return;

    display.clear();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    drawMandelbulb(dt);

//    glActiveTextureARB(GL_TEXTURE0);

    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    if(message_timer>0.0 && frameExporter==0) {
        glColor4f(message_colour.x, message_colour.y, message_colour.z, message_timer/10.0f);
        font.draw(0, 2, message);
    }

    glColor4f(1.0, 1.0, 1.0, 1.0);

    if(debug) {
        vec3f campos = view.getPos();

        font.print(0, 20, "fps: %.2f", fps);
        font.print(0, 40, "camera: %.2f,%.2f,%.2f %.2f", campos.x, campos.y, campos.z, gViewerSettings.speed);
        font.print(0, 60, "power: %.2f", gViewerSettings.power);
        font.print(0, 80, "maxIterations: %d", gViewerSettings.maxIterations);
        font.print(0, 100,"epsilonScale: %.5f", gViewerSettings.epsilonScale);
        font.print(0, 120,"aoSteps: %.5f", gViewerSettings.aoSteps);
        font.print(0, 140,"dt: %.5f", dt);

        if(scanline_mode) {
            font.print(0, 160, "rps: %.2f, %d / %d (batch: %d)", ((float)scanline_batch_size / dt)/(float)render_height, scanline_count, render_height, scanline_batch_size);
        }
    }

}
