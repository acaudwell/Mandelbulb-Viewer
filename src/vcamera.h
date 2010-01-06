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

#ifndef VIEW_CAMERA
#define VIEW_CAMERA

#include "core/matrix.h"

#include <vector>

class Object3D {
protected:
    vec3f up, side, forward;
    vec3f pos;
public:
    Object3D();
    void setPos(vec3f pos);

    void setSide(vec3f side);
    void setUp(vec3f up);
    void setForward(vec3f forward);

    vec3f getPos();
    vec3f getSide();
    vec3f getForward();
    vec3f getUp();

    void rotateX(float angle);
    void rotateY(float angle);
    void rotateZ(float angle);

    Object3D interpolate(Object3D& obj, float dt);

    mat3f getRotationMatrix();
};

class ViewCamera : public Object3D {
public:
    ViewCamera();

    ViewCamera interpolate(ViewCamera& obj, float dt);

};


class ViewCameraEvent {
protected:
    bool finished;
public:
    ViewCameraEvent();
    bool isFinished();

    virtual ViewCamera getCamera() { return ViewCamera(); };

    virtual void prepare(ViewCameraEvent& prev) { finished = false;};
    virtual void logic(float dt, ViewCamera* cam) {};
};

class ViewCameraMoveEvent : public ViewCameraEvent {

    float duration;
    float elapsed;

    ViewCamera start;
    ViewCamera finish;

public:
    ViewCameraMoveEvent(const ViewCamera& cam, float duration = 0.0);

    ViewCamera getCamera();

    void prepare(ViewCameraEvent& prev);
    void logic(float dt, ViewCamera* cam);
};

class ViewCameraPath {
    ViewCameraEvent* current;
    int current_index;

    bool loop;
    bool finished;

    std::vector<ViewCameraEvent*> events;
public:
    ViewCameraPath(bool loop = false);
    ~ViewCameraPath();

    size_t size();

    int getIndex();

    void addEvent(ViewCameraEvent* ce);
    void clear();
    void reset();
    bool isFinished();

    void logic(float dt, ViewCamera* cam);
};


#endif
