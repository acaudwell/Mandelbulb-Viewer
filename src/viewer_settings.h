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

#ifndef MANDELBULB_VIEWER_SETTINGS_H
#define MANDELBULB_VIEWER_SETTINGS_H

#include "core/settings.h"

#define MANDELBULB_VIEWER_VERSION "0.2"

class MandelbulbViewerSettings : public SDLAppSettings {
    void commandLineOption(const std::string& name, const std::string& value);
public:

    float timescale;
    float viewscale;

    bool backgroundGradient;
    bool juliaset;
    vec3f julia_c;

    float cameraZoom;
    float fov;
    float speed;

    bool constantSpeed;
    bool animated;
    bool phong;

    int antialiasing;

    float shadows;

    float specularity;
    float specularExponent;

    float ambientOcclusion;
    float ambientOcclusionEmphasis;

    float power;
    float bounding;
    float bailout;
    float epsilonScale;
    int maxIterations;
    int stepLimit;

    float fogDistance;
    float aoSteps;

    float glowDepth;
    float glowMulti;
    vec3f glowColour;

    bool  rave;
    bool  pulsate;
    float pulseScale;
    bool  pulsateFov;
    float  pulseFovScale;

    vec3f rotation;

    float beat;
    int beatPeriod;

    bool radiolaria;
    float radiolariaFactor;

    float colorSpread;
    float rimLight;

    vec3f light;

    vec4f  backgroundColor;
    vec4f  diffuseColor;
    vec4f  ambientColor;
    vec4f  lightColor;

    MandelbulbViewerSettings();

    void setDisplayDefaults();
    void setViewerDefaults();

    void importViewerSettings(ConfFile& conf);
    void exportViewerSettings(ConfFile& conf);

    void help();
};

extern MandelbulbViewerSettings gViewerSettings;

#endif
