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

#include "vcamera.h"

Object3D::Object3D() {
    side    = vec3f(1.0, 0.0, 0.0);
    up      = vec3f(0.0, 1.0, 0.0);
    forward = vec3f(0.0, 0.0, 1.0);
}

Object3D::Object3D(vec3f pos, vec3f up, vec3f side, vec3f forward) {
    this->pos     = pos;
    this->up      = up;
    this->side    = side;
    this->forward = forward;
}

void Object3D::setPos(vec3f pos) {
    this->pos = pos;
}

void Object3D::setUp(vec3f up) {
    this->up = up;
}

void Object3D::setSide(vec3f side) {
    this->side = side;
}

void Object3D::setForward(vec3f forward) {
    this->forward = forward;
}

vec3f Object3D::getPos() {
    return pos;
}

vec3f Object3D::getUp() {
    return up;
}

vec3f Object3D::getSide() {
    return side;
}

vec3f Object3D::getForward() {
    return forward;
}

void Object3D::rotateX(float radians) {

    vec3f _forward = forward;
    vec3f _up      = up;

    float c = cosf(radians);
    float s = sinf(radians);

    forward = c * _forward + s * _up;
    up      = c * _up      - s * _forward;

    forward.normalize();
    up.normalize();
}

void Object3D::rotateY(float radians) {

    vec3f _forward = forward;
    vec3f _side   = side;

    float c = cosf(radians);
    float s = sinf(radians);

    forward = c * _forward + s * _side;
    side   = c * _side   - s * _forward;

    forward.normalize();
    side.normalize();
}

void Object3D::rotateZ(float radians) {

    vec3f _up    = up;
    vec3f _side = side;

    float c = cosf(radians);
    float s = sinf(radians);

    up    = c * _up    + s * _side;
    side  = c * _side - s * _up;

    up.normalize();
    side.normalize();
}

Object3D Object3D::interpolate(Object3D& obj, float dt) {

    Object3D result;

    result.setPos( pos + (obj.getPos() - pos) * dt );
    result.setUp( up + (obj.getUp() - up) * dt );
    result.setSide( side + (obj.getSide() - side) * dt );
    result.setForward( forward + (obj.getForward() - forward) * dt );

    return result;
}

mat3f Object3D::getRotationMatrix() {

    return mat3f( -side.x, up.x, -forward.x,
                  -side.y, up.y, -forward.y,
                  -side.z, up.z, -forward.z);
}

// ViewCamera

ViewCamera::ViewCamera() {
}

ViewCamera::ViewCamera(vec3f pos, vec3f up, vec3f side, vec3f forward)
    : Object3D(pos, up, side, forward) {
}

ViewCamera ViewCamera::interpolate(ViewCamera& obj, float dt) {

    ViewCamera result;

    result.setPos( pos + (obj.getPos() - pos) * dt );
    result.setUp( up + (obj.getUp() - up) * dt );
    result.setSide( side + (obj.getSide() - side) * dt );
    result.setForward( forward + (obj.getForward() - forward) * dt );

    return result;
}

// ViewCameraEvent

ViewCameraEvent::ViewCameraEvent() {
    finished=false;
}

bool ViewCameraEvent::isFinished() {
    return finished;
}

ViewCameraEvent::ViewCameraEvent(const ViewCamera& cam, float duration) {
    this->start  = cam;
    this->finish = cam;
    this->duration = duration;
    elapsed = 0.0;
    finished = false;
}

void ViewCameraEvent::prepare(ViewCameraEvent& prev) {
    this->start = prev.getCamera();
    finished=false;
    elapsed = 0.0;
}

float ViewCameraEvent::getDuration() {
    return duration;
}

void ViewCameraEvent::setDuration(float duration) {
    this->duration = duration;
}

ViewCamera ViewCameraEvent::getCamera() {
    return finish;
}

void ViewCameraEvent::logic(float dt, ViewCamera* cam) {
    elapsed += dt;

    if(elapsed >= duration || duration <= 0.0f) {
        *cam = finish;
        elapsed = duration;
        finished=true;
        return;
    }

    float pc = elapsed/duration;

    *cam = (ViewCamera) start.interpolate(finish, pc);
}

//ViewCameraPath

ViewCameraPath::ViewCameraPath(bool loop) {
    this->loop = loop;
    this->units_per_second = -1.0;
    reset();
}

ViewCameraPath::~ViewCameraPath() {
    clear();
}

void ViewCameraPath::setUnitsPerSecond(float units_per_second) {
    this->units_per_second = units_per_second;
}

void ViewCameraPath::load(ConfFile& conf) {

    clear();

    ConfSectionList* cameralist = conf.getSections("camera");

    if(cameralist==0) return;

    for(ConfSectionList::iterator it = cameralist->begin();
          it != cameralist->end(); it++) {

        ConfSection* section = *it;

        ViewCamera cam(section->getVec3("pos"),
                       section->getVec3("up"),
                       section->getVec3("side"),
                       section->getVec3("forward"));

        ViewCameraEvent* e =
            new ViewCameraEvent(cam, section->getFloat("duration"));

        addEvent(e);
    }
}

void ViewCameraPath::save(ConfFile& conf) {

    for(std::vector<ViewCameraEvent*>::iterator it = events.begin(); it != events.end(); it++) {

        ViewCameraEvent* event = *it;

        ConfSection* section = new ConfSection("camera");

        ViewCamera cam = event->getCamera();

        section->setEntry(new ConfEntry("pos",     cam.getPos()));
        section->setEntry(new ConfEntry("up",      cam.getUp()));
        section->setEntry(new ConfEntry("side",    cam.getSide()));
        section->setEntry(new ConfEntry("forward", cam.getForward()));
        section->setEntry(new ConfEntry("duration", event->getDuration()));

        conf.addSection(section);
    }
}

bool ViewCameraPath::isFinished() {
    return finished;
}

void ViewCameraPath::reset() {
    finished=false;
    current_index = -1;
    current = 0;
}

void ViewCameraPath::clear() {
    for(std::vector<ViewCameraEvent*>::iterator it = events.begin(); it != events.end(); it++) {
        delete *it;
    }
    events.clear();
    reset();
}

bool ViewCameraPath::getLastCamera(ViewCamera& cam) {
    if(events.size()==0) return false;
    ViewCameraEvent* last = events.back();
    cam = last->getCamera();
    return true;
}

void ViewCameraPath::deleteLast() {
    if(events.size() > 0) {
        ViewCameraEvent* last = events.back();
        events.pop_back();
        delete last;
    }
}


void ViewCameraPath::addEvent(ViewCameraEvent* ce) {

    if(units_per_second>0.0 && events.size()>0) {
        ViewCameraEvent* last = events[events.size()-1];

        float dist = (ce->getCamera().getPos() - last->getCamera().getPos()).length();

        float duration = (1.0/units_per_second) * dist;

        ce->setDuration(duration);
    }

    events.push_back(ce);
}

size_t ViewCameraPath::size() {
    return events.size();
}

int ViewCameraPath::getIndex() {
    return current_index;
}

void ViewCameraPath::logic(float dt, ViewCamera* cam) {
    if(finished) return;

    if(events.size() == 0) {
        current = 0;
        finished = true;
    }

    if(current == 0) {
        if(loop) {
            current_index = (current_index + 1) % events.size();
        } else {
            current_index++;

            if(current_index >= events.size()) {
                finished=true;
                return;
            }
        }

        current = events[current_index];

        if(current_index>0) {
            current->prepare(*events[current_index-1]);
        }
    }

    current->logic(dt, cam);

    if(current->isFinished()) {
        current = 0;
    }
}

