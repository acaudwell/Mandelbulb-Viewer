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

ViewCamera::ViewCamera() : Object3D() {
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

// ViewCameraMoveEvent

ViewCameraMoveEvent::ViewCameraMoveEvent(const ViewCamera& cam, float duration) : ViewCameraEvent() {
    this->start  = cam;
    this->finish = cam;
    this->duration = duration;
    elapsed = 0.0;
    finished = false;
}

void ViewCameraMoveEvent::prepare(ViewCameraEvent& prev) {
    this->start = prev.getCamera();
    finished=false;
    elapsed = 0.0;
}

ViewCamera ViewCameraMoveEvent::getCamera() {
    return finish;
}

void ViewCameraMoveEvent::logic(float dt, ViewCamera* cam) {
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
    reset();
}

ViewCameraPath::~ViewCameraPath() {
    clear();
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

void ViewCameraPath::addEvent(ViewCameraEvent* ce) {
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

