/*
    Copyright (C) 2010 Andrew Caudwell (acaudwell@gmail.com)

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

#include "viewer_settings.h"

MandelbulbViewerSettings gViewerSettings;

void MandelbulbViewerSettings::help() {

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

MandelbulbViewerSettings::MandelbulbViewerSettings() {
    setViewerDefaults();

    default_section_name = "mandelbulb";

    //command line only options
    conf_sections["help"]      = "command-line";

    //boolean args
    arg_types["help"]             = "bool";

    arg_types["viewscale"]        = "float";
    arg_types["timescale"]        = "float";
    arg_types["speed"]            = "float";
    arg_types["fov"]              = "float";
    arg_types["cameraZoom"]       = "float";
    arg_types["animated"]         = "bool";
    arg_types["juliaset"]         = "bool";
    arg_types["julia_c"]          = "vec3";
    arg_types["radiolaria"]       = "bool";
    arg_types["radiolariaFactor"] = "float";
    arg_types["power"]            = "float";
    arg_types["bounding"]         = "float";
    arg_types["bailout"]          = "float";
    arg_types["antialiasing"]     = "int";
    arg_types["phong"]            = "bool";
    arg_types["fogDistance"]      = "float";
    arg_types["shadows"]          = "float";
    arg_types["ambientOcclusion"] = "float";
    arg_types["ambientOcclusionEmphasis"] = "float";
    arg_types["colorSpread"]      = "float";
    arg_types["rimLight"]         = "float";
    arg_types["specularity"]      = "float";
    arg_types["specularExponent"] = "float";
    arg_types["light"]            = "vec3";
    arg_types["backgroundColor"]  = "vec4";
    arg_types["diffuseColor"]     = "vec4";
    arg_types["ambientColor"]     = "vec4";
    arg_types["lightColor"]       = "vec4";
    arg_types["maxIterations"]    = "int";
    arg_types["stepLimit"]        = "int";
    arg_types["epsilonScale"]     = "float";
    arg_types["backgroundGradient"] = "bool";
    arg_types["aoSteps"]          = "float";
    arg_types["glowDepth"]        = "float";
    arg_types["glowMulti"]        = "float";
    arg_types["glowColour"]       = "vec3";
    arg_types["constantSpeed"]    = "bool";
    arg_types["beat"]             = "float";
}

void MandelbulbViewerSettings::commandLineOption(const std::string& name, const std::string& value) {

    if(name == "help") {
        help();
    }

}

void MandelbulbViewerSettings::setViewerDefaults() {

    viewscale = 1.0;
    timescale = 1.0;

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
    beatPeriod = 8;
}

void MandelbulbViewerSettings::importViewerSettings(ConfFile& conf) {

    ConfSection* settings = conf.getSection("mandelbulb");

    if(settings==0) return;

    if(settings->hasValue("cameraZoom"))
        cameraZoom = settings->getFloat("cameraZoom");

    if(settings->hasValue("speed"))
        speed = settings->getFloat("speed");

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

    if(settings->hasValue("timescale"))
        timescale = settings->getFloat("timescale");

    if(settings->hasValue("viewscale"))
        viewscale = settings->getFloat("viewscale");
}

void MandelbulbViewerSettings::exportViewerSettings(ConfFile& conf) {

    ConfSection* section = new ConfSection("mandelbulb");

    //export current settings

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
}
