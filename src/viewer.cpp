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


#ifdef _WIN32
HWND consoleWindow = 0;

void createWindowsConsole() {
    if(consoleWindow !=0) return;

    //create a console on Windows so users can see messages

    //find an available name for our window
    int console_suffix = 0;
    char consoleTitle[512];
    sprintf(consoleTitle, "%s", "Mandelbulb Console");

    while(FindWindow(0, consoleTitle)) {
        sprintf(consoleTitle, "Mandelbulb Console %d", ++console_suffix);
    }

    AllocConsole();
    SetConsoleTitle(consoleTitle);

    //redirect streams to console
    freopen("conin$", "r", stdin);
    freopen("conout$","w", stdout);
    freopen("conout$","w", stderr);

    consoleWindow = 0;

    //wait for our console window
    while(consoleWindow==0) {
        consoleWindow = FindWindow(0, consoleTitle);
        SDL_Delay(100);
    }

    //disable the close button so the user cant crash gource
    HMENU hm = GetSystemMenu(consoleWindow, false);
    DeleteMenu(hm, SC_CLOSE, MF_BYCOMMAND);
}
#endif

//info message
void mandelbulb_info(std::string msg) {
#ifdef _WIN32
    createWindowsConsole();
#endif

    printf("%s\n", msg.c_str());

#ifdef _WIN32
    printf("\nPress Enter\n");
    getchar();
#endif

    exit(0);
}

//display error only
void mandelbulb_quit(std::string error) {
    SDL_Quit();

#ifdef _WIN32
    createWindowsConsole();
#endif

    printf("Error: %s\n\n", error.c_str());

#ifdef _WIN32
    printf("Press Enter\n");
    getchar();
#endif

    exit(1);
}

//display help message + error (optional)
void mandelbulb_help(std::string error) {

#ifdef _WIN32
    createWindowsConsole();

    //resize window to fit help message
    if(consoleWindow !=0) {
        RECT windowRect;
        if(GetWindowRect(consoleWindow, &windowRect)) {
            float width = windowRect.right - windowRect.left;
            MoveWindow(consoleWindow,windowRect.left,windowRect.top,width,400,true);
        }
    }
#endif

    printf("Mandelbulb Viewer v%s\n", MANDELBULB_VIEWER_VERSION);

    if(error.size()) {
        printf("Error: %s\n\n", error.c_str());
    }

    printf("Usage: mandelbulb [OPTIONS] [FILE]\n");
    printf("\nOptions:\n");
    printf("  -h, --help                       Help\n\n");
    printf("  -WIDTHxHEIGHT                    Set window size\n");
    printf("  -f                               Fullscreen\n\n");

    printf("  --multi-sampling         Enable multi-sampling\n\n");

    printf("  --output-ppm-stream FILE Write frames as PPM to a file ('-' for STDOUT)\n");
    printf("  --output-framerate FPS   Framerate of output (25,30,60)\n\n");

    printf("FILE may be a Mandelbulb conf file or a recording file.\n\n");

#ifdef _WIN32
    printf("Press Enter\n");
    getchar();
#endif

    //check if we should use an error code
    if(error.size()) {
        exit(1);
    } else {
        exit(0);
    }
}

int main(int argc, char *argv[]) {

    int width  = 1024;
    int height = 768;
    bool fullscreen=false;
    bool multisample=false;

    std::string conffile = "mandelbulb.conf";

    std::string ppm_file_name;
    int video_framerate = 60;

    std::vector<std::string> arguments;

    SDLAppParseArgs(argc, argv, &width, &height, &fullscreen, &arguments);

    for(int i=0;i<arguments.size();i++) {
        std::string args = arguments[i];

        if(args == "-h" || args == "-?" || args == "--help") {
            mandelbulb_help("");
        }

        if(args == "--output-ppm-stream") {

            if((i+1)>=arguments.size()) {
                mandelbulb_help("specify ppm output file or '-' for stdout");
            }

            ppm_file_name = arguments[++i];

#ifdef _WIN32
            if(ppm_file_name == "-") {
                mandelbulb_help("stdout PPM mode not supported on Windows");
            }
#endif
            continue;
        }

        if(args == "--output-framerate") {

            if((i+1)>=arguments.size()) {
                mandelbulb_help("specify framerate (25,30,60)");
            }

            video_framerate = atoi(arguments[++i].c_str());

            if(   video_framerate != 25
               && video_framerate != 30
               && video_framerate != 60) {
                mandelbulb_help("supported framerates are 25,30,60");
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

        mandelbulb_help(arg_error);

    }

    display.enableShaders(true);

    if(multisample) {
        display.multiSample(4);
    }

    display.enableVsync(true);

    display.init("Mandelbulb Viewer", width, height, fullscreen);

    if(multisample) glEnable(GL_MULTISAMPLE_ARB);

    MandelbulbViewer* viewer = new MandelbulbViewer(conffile);

    if(ppm_file_name.size()) {
        viewer->createVideo(ppm_file_name, video_framerate);
    }

    viewer->run();

    delete viewer;

    display.quit();

    return 0;
}

MandelbulbViewer::MandelbulbViewer(std::string conffile) : SDLApp() {
    shaderfile = "MandelbulbQuick";

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


    runtime = 0.0;
    frame_skip = 0;
    frame_count = 0;
    fixed_tick_rate = 0.0;

    frameExporter = 0;
    record_frame_skip  = 1;
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
}

MandelbulbViewer::~MandelbulbViewer() {
    if(shader != 0) delete shader;
    if(frameExporter != 0) delete frameExporter;
}

void MandelbulbViewer::createVideo(std::string filename, int video_framerate) {
    if(campath.size()==0) {
        mandelbulb_quit("nothing to record");
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

        if (e->keysym.sym == SDLK_F5) {
            readConfig();
        }

        if (e->keysym.sym == SDLK_F6) {
            saveConfig(false);
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
            epsilonScale = std::max( epsilonScale / 1.1, 0.00001);
        }

        if (e->keysym.sym ==  SDLK_RIGHTBRACKET) {
            epsilonScale = std::min( epsilonScale * 1.1, 2.0);
        }

        if (e->keysym.sym ==  SDLK_COMMA) {
            lod = std::max( lod / 1.1, 0.00001);
        }

        if (e->keysym.sym ==  SDLK_PERIOD) {
            lod = std::min( lod * 1.1, 1.0);
        }

        if (e->keysym.sym ==  SDLK_F1) {
            if(maxIterations>1) maxIterations--;
        }

        if (e->keysym.sym ==  SDLK_F2) {
            maxIterations++;
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

    power = 8.0;
    bounding = 3.0f;
    bailout = 4.0f;

    stepLimit = 110;
    maxIterations = 6;
    epsilonScale = 1.0;
    lod = 1.0;

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
    section->setEntry(new ConfEntry("lod", lod));

    section->setEntry(new ConfEntry("backgroundGradient", backgroundGradient));
    section->setEntry(new ConfEntry("fov", fov));
    section->setEntry(new ConfEntry("speed", speed));

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

    animated         = settings->getBool("animated");
    juliaset         = settings->getBool("juliaset");
    julia_c          = settings->getVec3("julia_c");
    radiolaria       = settings->getBool("radiolaria");
    radiolariaFactor = settings->getFloat("radiolariaFactor");

    power            = settings->getFloat("power");
    bounding         = settings->getFloat("bounding");
    bailout          = settings->getFloat("bailout");

    antialiasing     = settings->getInt("antialiasing");
    phong            = settings->getBool("phong");
    shadows          = settings->getFloat("shadows");
    ambientOcclusion = settings->getFloat("ambientOcclusion");
    ambientOcclusionEmphasis = settings->getFloat("ambientOcclusionEmphasis");
    colorSpread      = settings->getFloat("colorSpread");
    rimLight         = settings->getFloat("rimLight");
    specularity      = settings->getFloat("specularity");
    specularExponent = settings->getFloat("specularExponent");
    light            = settings->getVec3("light");
    backgroundColor  = settings->getVec4("backgroundColor");
    diffuseColor     = settings->getVec4("diffuseColor");
    ambientColor     = settings->getVec4("ambientColor");
    lightColor       = settings->getVec4("lightColor");

    maxIterations    = settings->getInt("maxIterations");
    stepLimit        = settings->getInt("stepLimit");
    epsilonScale     = settings->getFloat("epsilonScale");

    lod              = settings->getFloat("lod");
    backgroundGradient = settings->getBool("backgroundGradient");

    fov              = settings->getFloat("fov");

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

    float amount = speed * std::min(1.0f, cam_distance) * dt;

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
    //dt = std::max(dt, 1.0f/25.0f);

    //if exporting a video use a fixed tick rate rather than time based
    if(frameExporter != 0) dt = fixed_tick_rate;

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

void MandelbulbViewer::draw(float t, float dt) {
    if(appFinished) return;

    display.clear();

    if(!paused) {
        time_elapsed += dt;
    }

    vec3f _julia_c = julia_c;

    if(animated) {
        _julia_c = julia_c + vec3f(sinf(time_elapsed), sinf(time_elapsed), atan(time_elapsed)) * 0.1;
    }

    vec3f campos = view.getPos();

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
    shader->setFloat("lod", lod);

    shader->setInteger("backgroundGradient", backgroundGradient);
    shader->setFloat("fov",  fov);

    drawAlignedQuad();

    glUseProgramObjectARB(0);
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
        font.print(0, 20, "fps: %.2f", fps);
        font.print(0, 40, "camera: %.2f,%.2f,%.2f %.2f", campos.x, campos.y, campos.z, speed);
        font.print(0, 60, "power: %.2f", power);
        font.print(0, 80, "maxIterations: %d", maxIterations);
        font.print(0, 100, "epsilonScale: %.5f", epsilonScale);
        font.print(0, 120,"lod: %.5f", lod);
        font.print(0, 140,"dt: %.5f", dt);

        if(juliaset) {
            font.print(0, 140, "julia_c: %.2f,%.2f,%.2f", _julia_c.x, _julia_c.y, _julia_c.z);
        }
    }

}
