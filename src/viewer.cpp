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

//display help message + error (optional)
void mandelbulb_help() {

#ifdef _WIN32
    SDLAppCreateWindowsConsole();

    SDLAppResizeWindowsConsole(400);
#endif

    printf("Mandelbulb Viewer v%s\n", MANDELBULB_VIEWER_VERSION);

    printf("Usage: mandelbulb [OPTIONS] [FILE]\n");
    printf("\nOptions:\n");
    printf("  -h, --help                       Help\n\n");
    printf("  -WIDTHxHEIGHT                    Set window size\n");
    printf("  -f                               Fullscreen\n\n");

    printf("  --viewscale SCALE        Set the view scale (default: 1.0)\n");
    printf("  --timescale SCALE        Set the time scale (default: 1.0)\n\n");

    printf("  --multi-sampling         Enable multi-sampling\n\n");

    printf("  --output-ppm-stream FILE Write frames as PPM to a file ('-' for STDOUT)\n");
    printf("  --output-framerate FPS   Framerate of output (25,30,60)\n\n");

    printf("FILE may be a Mandelbulb conf file or a recording file.\n\n");

#ifdef _WIN32
    printf("Press Enter\n");
    getchar();
#endif

    exit(0);
}

int main(int argc, char *argv[]) {

    int width  = 1024;
    int height = 768;
    bool fullscreen=false;
    bool multisample=false;

    float viewscale = 1.0f;
    float timescale = 1.0f;

    std::string conffile = "mandelbulb.conf";

    std::string ppm_file_name;
    int video_framerate = 60;

    std::vector<std::string> arguments;

    SDLAppInit("Mandelbulb", "mandelbulb");

    SDLAppParseArgs(argc, argv, &width, &height, &fullscreen, &arguments);

    for(int i=0;i<arguments.size();i++) {
        std::string args = arguments[i];

        if(args == "-h" || args == "-?" || args == "--help") {
            mandelbulb_help();
        }

        if(args == "--output-ppm-stream") {

            if((i+1)>=arguments.size()) {
                SDLAppQuit("specify ppm output file or '-' for stdout");
            }

            ppm_file_name = arguments[++i];

#ifdef _WIN32
            if(ppm_file_name == "-") {
                SDLAppQuit("stdout PPM mode not supported on Windows");
            }
#endif
            continue;
        }

        if(args == "--output-framerate") {

            if((i+1)>=arguments.size()) {
                SDLAppQuit("specify framerate (25,30,60)");
            }

            video_framerate = atoi(arguments[++i].c_str());

            if(   video_framerate != 25
               && video_framerate != 30
               && video_framerate != 60) {
                SDLAppQuit("supported framerates are 25,30,60");
            }

            continue;
        }

        if(args == "--timescale") {

            if((i+1)>=arguments.size()) {
                SDLAppQuit("specify timescale");
            }

            timescale = atof(arguments[++i].c_str());

            if(timescale<=0.0f) {
                SDLAppQuit("timescale invalid");
            }

            continue;
        }

        if(args == "--viewscale") {

            if((i+1)>=arguments.size()) {
                SDLAppQuit("specify viewscale (0.0 - 1.0)");
            }

            viewscale = atof(arguments[++i].c_str());

            if(viewscale<=0.0f || viewscale > 1.0f) {
                SDLAppQuit("viewscale invalid");
            }

            continue;
        }


        if(args == "--multi-sampling") {
            multisample = true;
            continue;
        }

        // assume this is the log file
        if(args == "-" || args.size() >= 1 && args[0] != '-') {
            conffile = args;
            continue;
        }

        // unknown argument
        std::string arg_error = std::string("unknown option ") + std::string(args);

        SDLAppQuit(arg_error);

    }

    display.enableShaders(true);

    if(multisample) {
        display.multiSample(4);
    }

    display.enableVsync(true);

    display.init("Mandelbulb Viewer", width, height, fullscreen);

    if(multisample) glEnable(GL_MULTISAMPLE_ARB);

    MandelbulbViewer* viewer = 0;

    try {
        viewer = new MandelbulbViewer(conffile, viewscale, timescale);

        if(ppm_file_name.size()) {
            viewer->createVideo(ppm_file_name, video_framerate);
        }

        viewer->run();

    } catch(ResourceException& exception) {

        char errormsg[1024];
        snprintf(errormsg, 1024, "failed to load resource '%s'", exception.what());

        SDLAppQuit(errormsg);

    } catch(SDLAppException& exception) {

        if(exception.showHelp()) {
            mandelbulb_help();
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

MandelbulbViewer::MandelbulbViewer(std::string conffile, float viewscale, float timescale) : SDLApp() {
    shaderfile = "MandelbulbQuick";

    this->timescale = timescale;

    shader = 0;
    time_elapsed = 0;
    paused = false;

    mandelbulb.setPos(vec3f(0.0, 0.0, 0.0));
    mandelbulb.rotateX(90.0f * DEGREES_TO_RADIANS);

    view.setPos(vec3f(0.0, 0.0, 2.6));

    play = false;
    record = false;

    mouselook = false;
    roll      = false;

    timescale = 1.0;

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

    setDefaults();

    //ignore mouse motion until we have finished setting up
    SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

    if(conffile.size()) {

        conf.setFilename(conffile);

        if(readConfig()) {

            //load recording
            if(conf.hasSection("camera")) {
                campath.load(conf);
                play=true;
            }
        } else {
            conf.clear();
        }
    }

    vwidth  = display.width;
    vheight = display.height;

    if(viewscale != 1.0f) {
        vwidth  *= viewscale;
        vheight *= viewscale;
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

    SDL_ShowCursor(false);
}

void MandelbulbViewer::randomizeJuliaSeed() {
    julia_c = vec3f( rand() % 1000, rand() % 1000, rand() % 1000 ).normal();
}

void MandelbulbViewer::randomizeColours() {
    backgroundColor = vec4f(vec3f(rand() % 100, rand() % 100, rand() % 100).normal(), 1.0);
    diffuseColor = vec4f(vec3f(rand() % 100, rand() % 100, rand() % 100).normal(), 1.0);
    ambientColor = vec4f(vec3f(rand() % 100, rand() % 100, rand() % 100).normal(), 1.0);
    lightColor   = vec4f(vec3f(rand() % 100, rand() % 100, rand() % 100).normal(), 1.0);
}

void MandelbulbViewer::init() {
    display.setClearColour(vec3f(0.0, 0.0, 0.0));

    shader = shadermanager.grab(shaderfile);

    rendertex = display.emptyTexture(display.width, display.height, GL_RGBA);

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

        if (e->keysym.sym == SDLK_F9) {
            readConfig();
        }

        if (e->keysym.sym == SDLK_F10) {
            saveConfig(false);
        }

        if (e->keysym.sym == SDLK_F11) {
            beat = 0.22;
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

        if (e->keysym.sym ==  SDLK_v) {
            constantSpeed = !constantSpeed;
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
            epsilonScale = std::max( epsilonScale / 1.1, 0.00001);
        }

        if (e->keysym.sym ==  SDLK_RIGHTBRACKET) {
            epsilonScale = std::min( epsilonScale * 1.1, 2.0);
        }

        if (e->keysym.sym ==  SDLK_COMMA) {
            aoSteps /= 1.1f;
        }

        if (e->keysym.sym ==  SDLK_PERIOD) {
            aoSteps *= 1.1f;
        }

        if (e->keysym.sym ==  SDLK_F1) {
            if(maxIterations>1) maxIterations--;
        }

        if (e->keysym.sym ==  SDLK_F2) {
            maxIterations++;
        }

        if (e->keysym.sym ==  SDLK_F3) {
            fogDistance -= 0.25;
        }

        if (e->keysym.sym ==  SDLK_F4) {
            fogDistance += 0.25;
        }

        if (e->keysym.sym ==  SDLK_F5) {
            glowDepth -= 0.1;
        }

        if (e->keysym.sym ==  SDLK_F6) {
            glowDepth += 0.1;
        }

        if (e->keysym.sym ==  SDLK_F7) {
            glowMulti /= 1.1;
        }

        if (e->keysym.sym ==  SDLK_F8) {
            glowMulti *= 1.1;
        }

        if (e->keysym.sym ==  SDLK_MINUS) {
//            if(power>1.0) power -= 1.0;
            power -= 1.0;
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

void MandelbulbViewer::setMessage(const std::string& message, const vec3f& colour) {
    this->message = message;
    message_colour = colour;
    message_timer = 10.0;
}

void MandelbulbViewer::setDefaults() {

    fov = 45.0f;
    cameraZoom = 0.0f;
    speed = 0.25;

    constantSpeed = true;

    power = 8.0;
    bounding = 3.0f;
    bailout = 4.0f;

    stepLimit = 600;
    maxIterations = 6;
    epsilonScale = 1.0;
    aoSteps = 100.0;

    fogDistance = 0.0f;
    phong = true;
    antialiasing = 0;
    shadows = 0.0;
    specularity = 0.7;
    specularExponent = 15.0;
    ambientOcclusion = 0.5f;
    ambientOcclusionEmphasis = 0.58f;

    radiolaria = false;
    radiolariaFactor = 0.0f;

    colorSpread = 0.2;
    rimLight = 0.0;

    animated = false;
    juliaset = false;
    backgroundGradient = true;

    light = vec3f(38, -42, 38);

    backgroundColor = vec4f(0.0, 0.0, 0.0, 1.0);
    diffuseColor    = vec4f(0.0, 0.85, 0.99, 1.0);
    ambientColor    = vec4f(0.67, 0.85, 1.0, 1.0);
    lightColor      = vec4f(0.48, 0.59, 0.66, 1.0);
    glowColour      = vec3f(1.0, 1.0, 0.0);

    glowMulti = 1.0;
    glowDepth = 1.5;

    beat = 0.0;
    beatTimer = 0.0;
    beatGlowDepth = 0.0;
    beatGlowMulti = 0.0;
    beatCount = 0;
    beatPeriod = 8;
}


void MandelbulbViewer::saveConfig(bool saverec) {

    conf.clear();

    if(saverec) {
        //get next free recording name
        char recname[256];
        struct stat finfo;
        int recno = 1;

        while(1) {
            snprintf(recname, 256, "%06d.mdb", recno);
            if(stat(recname, &finfo) != 0) break;

            recno++;
        }

        conf.setFilename(recname);
    } else {
        conf.setFilename("mandelbulb.conf");
    }

    ConfSection* section = new ConfSection("mandelbulb");

    //save settings

    section->setEntry(new ConfEntry("animated", animated));
    section->setEntry(new ConfEntry("juliaset", juliaset));
    section->setEntry(new ConfEntry("julia_c", julia_c));
    section->setEntry(new ConfEntry("radiolaria", radiolaria));
    section->setEntry(new ConfEntry("radiolariaFactor", radiolariaFactor));

    section->setEntry(new ConfEntry("power", power));
    section->setEntry(new ConfEntry("bounding", bounding));
    section->setEntry(new ConfEntry("bailout", bailout));

    section->setEntry(new ConfEntry("antialiasing", antialiasing));
    section->setEntry(new ConfEntry("phong", phong));
    section->setEntry(new ConfEntry("shadows", shadows));
    section->setEntry(new ConfEntry("ambientOcclusion", ambientOcclusion));
    section->setEntry(new ConfEntry("ambientOcclusionEmphasis", ambientOcclusionEmphasis));
    section->setEntry(new ConfEntry("colorSpread", colorSpread));
    section->setEntry(new ConfEntry("rimLight", rimLight));
    section->setEntry(new ConfEntry("specularity", specularity));
    section->setEntry(new ConfEntry("specularExponent", specularExponent));
    section->setEntry(new ConfEntry("light", light));

    section->setEntry(new ConfEntry("backgroundColor", backgroundColor));
    section->setEntry(new ConfEntry("diffuseColor", diffuseColor));
    section->setEntry(new ConfEntry("ambientColor", ambientColor));
    section->setEntry(new ConfEntry("lightColor", lightColor));

    section->setEntry(new ConfEntry("maxIterations", maxIterations));
    section->setEntry(new ConfEntry("stepLimit", stepLimit));
    section->setEntry(new ConfEntry("epsilonScale", epsilonScale));
    section->setEntry(new ConfEntry("aoSteps", aoSteps));

    section->setEntry(new ConfEntry("fogDistance", fogDistance));
    section->setEntry(new ConfEntry("glowDepth", glowDepth));
    section->setEntry(new ConfEntry("glowMulti", glowMulti));
    section->setEntry(new ConfEntry("glowColour", glowColour));

    section->setEntry(new ConfEntry("backgroundGradient", backgroundGradient));
    section->setEntry(new ConfEntry("fov", fov));
    section->setEntry(new ConfEntry("speed", speed));
    section->setEntry(new ConfEntry("constantSpeed", constantSpeed));

    conf.setSection(section);

    if(saverec) {
        campath.save(conf);
    }

    conf.save();

    setMessage("Wrote " + conf.getFilename());
}

bool MandelbulbViewer::readConfig() {

    if(conf.getFilename().size()==0) return false;

    if(!conf.load()) return false;

    ConfSection* settings = conf.getSection("mandelbulb");

    if(settings == 0) return true;

    if(settings->hasValue("animated"))
        animated = settings->getBool("animated");

    if(settings->hasValue("juliaset"))
        juliaset = settings->getBool("juliaset");

    if(settings->hasValue("julia_c"))
        julia_c = settings->getVec3("julia_c");

    if(settings->hasValue("radiolaria"))
        radiolaria = settings->getBool("radiolaria");

    if(settings->hasValue("radiolariaFactor"))
        radiolariaFactor = settings->getFloat("radiolariaFactor");

    if(settings->hasValue("power"))
        power = settings->getFloat("power");

    if(settings->hasValue("bounding"))
        bounding = settings->getFloat("bounding");

    if(settings->hasValue("bailout"))
        bailout = settings->getFloat("bailout");

    if(settings->hasValue("antialiasing"))
        antialiasing = settings->getInt("antialiasing");

    if(settings->hasValue("phong"))
        phong = settings->getBool("phong");

    if(settings->hasValue("fogDistance"))
        fogDistance = settings->getFloat("fogDistance");

    if(settings->hasValue("shadows"))
        shadows = settings->getFloat("shadows");

    if(settings->hasValue("ambientOcclusion"))
        ambientOcclusion = settings->getFloat("ambientOcclusion");

    if(settings->hasValue("ambientOcclusionEmphasis"))
        ambientOcclusionEmphasis = settings->getFloat("ambientOcclusionEmphasis");

    if(settings->hasValue("colorSpread"))
        colorSpread = settings->getFloat("colorSpread");

    if(settings->hasValue("rimLight"))
        rimLight = settings->getFloat("rimLight");

    if(settings->hasValue("specularity"))
        specularity = settings->getFloat("specularity");

    if(settings->hasValue("specularExponent"))
        specularExponent = settings->getFloat("specularExponent");

    if(settings->hasValue("light"))
        light = settings->getVec3("light");

    if(settings->hasValue("backgroundColor"))
        backgroundColor = settings->getVec4("backgroundColor");

    if(settings->hasValue("diffuseColor"))
        diffuseColor = settings->getVec4("diffuseColor");

    if(settings->hasValue("ambientColor"))
        ambientColor = settings->getVec4("ambientColor");

    if(settings->hasValue("lightColor"))
        lightColor = settings->getVec4("lightColor");

    if(settings->hasValue("maxIterations"))
        maxIterations = settings->getInt("maxIterations");

    if(settings->hasValue("stepLimit"))
        stepLimit = settings->getInt("stepLimit");

    if(settings->hasValue("epsilonScale"))
        epsilonScale = settings->getFloat("epsilonScale");

    if(settings->hasValue("backgroundGradient"))
        backgroundGradient = settings->getBool("backgroundGradient");

    if(settings->hasValue("fov"))
        fov = settings->getFloat("fov");

    if(settings->hasValue("aoSteps"))
        aoSteps = settings->getFloat("aoSteps");

    if(settings->hasValue("glowDepth"))
        glowDepth = settings->getFloat("glowDepth");

    if(settings->hasValue("glowMulti"))
        glowMulti = settings->getFloat("glowMulti");

    if(settings->hasValue("glowColour"))
        glowColour = settings->getVec3("glowColour");

    if(settings->hasValue("constantSpeed"))
        constantSpeed = settings->getBool("constantSpeed");

    if(settings->hasValue("beat"))
        beat = settings->getFloat("beat");

    return true;
}


void MandelbulbViewer::toggleRecord() {
    if(play) return;

    record = !record;
    record_frame_delta = 0.0;

    //start new recording
    if(record) {
        campath.clear();
    } else {
        saveConfig(true);
    }
}

void MandelbulbViewer::togglePlay() {
    if(record) return;

    play = !play;
    campath.reset();
}

void MandelbulbViewer::addWaypoint(float duration) {
    if(campath.size()==0) duration = 0.0;

    ViewCameraEvent* e = new ViewCameraEvent(view, duration);
    campath.addEvent(e);
}
void MandelbulbViewer::mouseMove(SDL_MouseMotionEvent *e) {

    //debugLog("mouseMove %d %d\n", e->xrel, e->yrel);
    if(mouselook) {
        if(roll) {
            view.rotateZ(-(e->xrel / 10.0f) * DEGREES_TO_RADIANS);
            view.rotateX((e->yrel / 10.0f) * DEGREES_TO_RADIANS);
        } else {
            view.rotateY((e->xrel / 10.0f) * DEGREES_TO_RADIANS);
            view.rotateX((e->yrel / 10.0f) * DEGREES_TO_RADIANS);
        }
   }
}

void MandelbulbViewer::mouseClick(SDL_MouseButtonEvent *e) {

    if(e->state == SDL_PRESSED) {
        if(e->button == SDL_BUTTON_RIGHT) {
            //save mouse position
            mousepos = vec2f(e->x, e->y);

            mouselook=true;
            SDL_ShowCursor(false);
            SDL_WM_GrabInput(SDL_GRAB_ON);
        }

        if(e->button == SDL_BUTTON_LEFT && mouselook) {
            roll = true;
        }

        if(e->button == SDL_BUTTON_WHEELUP) {
            speed *= 2.0;
        }

        if(e->button == SDL_BUTTON_WHEELDOWN) {
            speed /= 2.0;
        }

    }

    if(e->state == SDL_RELEASED) {
        if(e->button == SDL_BUTTON_LEFT && mouselook) {
            roll = false;
        }

        if(e->button == SDL_BUTTON_RIGHT) {
            mouselook=false;
            SDL_ShowCursor(true);
            SDL_WM_GrabInput(SDL_GRAB_OFF);

            //warp to last position
            SDL_WarpMouse(mousepos.x, mousepos.y);
        }
    }

}

void MandelbulbViewer::moveCam(float dt) {

    mat3f camRotation = view.getRotationMatrix();

    vec3f campos = view.getPos();

    float cam_distance = campos.length2();
    cam_distance *= cam_distance;

    float amount = constantSpeed ?
        speed * dt : speed * std::min(1.0f, cam_distance) * dt;

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

    dt *= timescale;

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

    frame_count++;
}

void MandelbulbViewer::logic(float t, float dt) {
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

    //roll doesnt make any sense unless mouselook
    //is enabled
    if(!mouselook) roll = false;

    if(record) {
        record_frame_delta += dt;

        if(frame_count % record_frame_skip == 0) {
            addWaypoint(record_frame_delta);
            record_frame_delta = 0.0;
        }
    }

    //update beat
    if(beat>0.0) {
        beatTimer += dt;
        if(beatTimer>beat*2.0) {
            beatTimer=0.0;
            beatCount++;

            if(beatCount % beatPeriod == 0) {
                glowColour = vec3f(rand() % 100, rand() % 100, rand() % 100).normal();
            }
        }

        float beatpc = beatTimer/beat;
        if(beatpc>1.0) beatpc = 2.0-beatpc;

        beatGlowDepth = glowDepth * 0.5 + 0.5 * glowDepth * beatpc;
        beatGlowMulti = glowMulti * 0.5 + 0.5 * glowMulti * beatpc;
    }

    //update julia seed
    _julia_c = julia_c;

    if(animated) {
        _julia_c = julia_c + vec3f(sinf(time_elapsed), sinf(time_elapsed), atan(time_elapsed)) * 0.1;
    }

    //to avoid a visible sphere we need to set the bounding
    //sphere to be greater than the camera's distance from the
    //origin
    if(backgroundGradient) {
        bounding = std::max(bounding, view.getPos().length2());
    }

//    float amount = 90 * dt;
//    mandelbulb.rotateY(dt * 90.0f * DEGREES_TO_RADIANS);

    viewRotation = view.getRotationMatrix();
    frame_count++;

    if(record) {
        vec3f red(1.0, 0.0, 0.0);

        char msgbuff[256];
        snprintf(msgbuff, 256, "Recording %d", campath.size());
        setMessage(std::string(msgbuff), red);
    }

    if(play) {
        vec3f green(0.0, 1.0, 0.0);

        char msgbuff[256];
        snprintf(msgbuff, 256, "Playing %d / %d", campath.getIndex(), campath.size());
        setMessage(std::string(msgbuff), green);
    }

    if(message_timer>0.0) message_timer -= dt;
}


void MandelbulbViewer::drawMandelbulb() {

    int vwidth = this->vwidth;
    int vheight = this->vheight;

    if(mouselook) {
        vwidth  *= 0.25;
        vheight *= 0.25;
    }

    vec3f campos = view.getPos();

    display.mode2D();

    shader->use();
    shader->setFloat("width",  vwidth);
    shader->setFloat("height", vheight);

    shader->setVec3("camera",         campos);
    shader->setVec3("cameraFine",     vec3f(0.0f, 0.0f, 0.0f));
    shader->setFloat("cameraZoom",    cameraZoom);

    shader->setInteger("julia", juliaset);
    shader->setVec3("julia_c", _julia_c);

    shader->setInteger("radiolaria", radiolaria);
    shader->setFloat("radiolariaFactor", radiolariaFactor);

    shader->setFloat("power", power);
    shader->setFloat("bounding", bounding );
    shader->setFloat("bailout", bailout );

    shader->setInteger("antialiasing", antialiasing);
    shader->setInteger("phong", phong);
    shader->setFloat("shadows", shadows);
    shader->setFloat("ambientOcclusion", ambientOcclusion);
    shader->setFloat("ambientOcclusionEmphasis", ambientOcclusionEmphasis);
    shader->setFloat("colorSpread",      colorSpread);
    shader->setFloat("rimLight",         rimLight);
    shader->setFloat("specularity",      specularity);
    shader->setFloat("specularExponent", specularExponent);

    shader->setVec3("light", light);

    shader->setVec4("backgroundColor", backgroundColor);
    shader->setVec4("diffuseColor",    diffuseColor);
    shader->setVec4("ambientColor",    ambientColor);
    shader->setVec4("lightColor",      lightColor);

    shader->setMat3("viewRotation", viewRotation);
    shader->setMat3("objRotation",  mandelbulb.getRotationMatrix());

    shader->setInteger("maxIterations", maxIterations);
    shader->setInteger("stepLimit",     stepLimit);
    shader->setFloat("epsilonScale",    epsilonScale);

    shader->setFloat("aoSteps", aoSteps);
    shader->setFloat("fogDistance", fogDistance);


    if(beat>0.0) {
        shader->setFloat("glowDepth", beatGlowDepth);
        shader->setFloat("glowMulti", beatGlowMulti);
    } else {
        shader->setFloat("glowDepth", glowDepth);
        shader->setFloat("glowMulti", glowMulti);
    }

    shader->setVec3("glowColour", glowColour);


    shader->setInteger("backgroundGradient", backgroundGradient);
    shader->setFloat("fov",  fov);

    drawAlignedQuad(vwidth, vheight);

    glUseProgramObjectARB(0);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    if(display.width != vwidth && display.height != vheight) {

        glBindTexture(GL_TEXTURE_2D, rendertex);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, display.height - vheight, vwidth, vheight, 0);

        display.mode2D();

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

void MandelbulbViewer::draw(float t, float dt) {
    if(appFinished) return;

    display.clear();

    if(!paused) {
        time_elapsed += dt;
    }

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    drawMandelbulb();

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
        font.print(0, 40, "camera: %.2f,%.2f,%.2f %.2f", campos.x, campos.y, campos.z, speed);
        font.print(0, 60, "power: %.2f", power);
        font.print(0, 80, "maxIterations: %d", maxIterations);
        font.print(0, 100, "epsilonScale: %.5f", epsilonScale);
        font.print(0, 120,"aoSteps: %.5f", aoSteps);
        font.print(0, 140,"dt: %.5f", dt);

        if(juliaset) {
            font.print(0, 140, "julia_c: %.2f,%.2f,%.2f", _julia_c.x, _julia_c.y, _julia_c.z);
        }
    }

}
