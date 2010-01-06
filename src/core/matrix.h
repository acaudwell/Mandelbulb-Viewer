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

#ifndef MATRIX_H
#define MATRIX_H

#include "vectors.h"

#include <cmath>

template<class T> class mat3 {
public:
    T matrix[3][3];

    mat3<T>::mat3<T>() {
        for(int r=0;r<3;r++) {
            for(int c=0;c<3;c++) {
                matrix[r][c] = 0;
            }
        }
    }

    mat3<T>::mat3<T>(T a1, T b1, T c1,
                     T a2, T b2, T c2,
                     T a3, T b3, T c3) {

        matrix[0][0] = a1; matrix[0][1] = b1; matrix[0][2] = c1;
        matrix[1][0] = a2; matrix[1][1] = b2; matrix[1][2] = c2;
        matrix[2][0] = a3; matrix[2][1] = b3; matrix[2][2] = c3;
    }

    vec3<T> operator* (const vec3<T>& vec) {

        vec3<T> v;

        // multiply each row of A
        // by the column of vector B

        v.x =  matrix[0][0] * vec.x
             + matrix[0][1] * vec.y
             + matrix[0][2] * vec.z;

        v.y =  matrix[1][0] * vec.x
             + matrix[1][1] * vec.y
             + matrix[1][2] * vec.z;

        v.z =  matrix[2][0] * vec.x
             + matrix[2][1] * vec.y
             + matrix[2][2] * vec.z;

        return v;
    }

    mat3<T> operator* (const mat3<T>& mat) {

        mat3<T> m;

        // multiply each row of A
        // by each column of B

        for(int r=0;r<3;r++) {
            for(int c=0;c<3;c++) {
                m.matrix[r][c] =  matrix[r][0] * mat.matrix[0][c]
                                + matrix[r][1] * mat.matrix[1][c]
                                + matrix[r][2] * mat.matrix[2][c];
            }
        }

        return m;
    }

    operator T*() const {
            return (T*) &matrix;
    }

    vec3<T> X() {
        return vec3<T>(matrix[0][0], matrix[1][0], matrix[2][0]);
    }

    vec3<T> Y() {
        return vec3<T>(matrix[0][1], matrix[1][1], matrix[2][1]);
    }

    vec3<T> Z() {
        return vec3<T>(matrix[0][2], matrix[1][2], matrix[2][2]);
    }

/*
    vec3<T> X() {
        return vec3<T>(matrix[0][0], matrix[0][1], matrix[0][2]);
    }

    vec3<T> Y() {
        return vec3<T>(matrix[1][0], matrix[1][1], matrix[1][2]);
    }

    vec3<T> Z() {
        return vec3<T>(matrix[2][0], matrix[2][1], matrix[2][2]);
    }
*/
};

typedef mat3<float> mat3f;

#endif
