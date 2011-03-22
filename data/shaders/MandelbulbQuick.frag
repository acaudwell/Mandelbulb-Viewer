/* MandelbulbQuick.pbk
 * Last update: 14 December 2009
 *
 * Changelog:
 *		1.0     - Initial release
 *		1.0.1	- Fixed a missing asymmetry thanks to Chris King (http://www.dhushara.com)
 *              - Refinements in the colouring
 *      1.0.2   - Added radiolaria option for a funky hair-like effect
 *              - Incorporated the scalar derivative method as described here:
 *              - http://www.fractalforums.com/mandelbulb-implementation/realtime-renderingoptimisations/
 *		1.0.3	- Created a quick version of the script as using a boolean flag to determine
 *                which distance estimation method created long compilation times.
 *      1.0.4   - Fixed issue with older graphic cards and the specular highlights
 *      1.0.4-1 - (fork) Moved rotation matrix code out of shader, added fov controls.
 *                Andrew Caudwell (acaudwell@gmail.com)
 *
 * Copyright (c) 2009 Tom Beddard
 * http://www.subblue.com
 *
 * For more Flash and PixelBender based generative graphics experiments see:
 * http://www.subblue.com/blog
 *
 * Licensed under the MIT License:
 * http://www.opensource.org/licenses/mit-license.php
 *
 *
 * Credits and references
 * ======================
 * For the story behind the 3D Mandelbrot see the following page:
 * http://www.skytopia.com/project/fractal/mandelbulb.html
 *
 * The original forum disussion with many implementation details can be found here:
 * http://www.fractalforums.com/3d-fractal-generation/true-3d-mandlebrot-type-fractal/
 *
 * This implementation references the 4D Quaternion GPU Raytracer by Keenan Crane:
 * http://www.devmaster.net/forums/showthread.php?t=4448
 *
 * and the NVIDIA CUDA/OptiX implementation by cbuchner1:
 * http://forums.nvidia.com/index.php?showtopic=150985
 *
 */


uniform float width;
uniform float height;
uniform float pixelSize;
uniform int  antialiasing;
uniform bool  phong;
uniform bool  julia;
uniform bool  radiolaria;

uniform float shadows;
uniform float radiolariaFactor;
uniform float ambientOcclusion;
uniform float ambientOcclusionEmphasis;
uniform float bounding;
uniform float bailout;
uniform float power;
uniform vec3  julia_c;
uniform vec3  camera;
uniform vec3  cameraFine;
//uniform vec3  cameraRotation;
uniform float cameraZoom;
uniform vec3  light;
uniform vec4  backgroundColor;
uniform vec4  diffuseColor;
uniform vec4  ambientColor;
uniform vec4  lightColor;
uniform float colorSpread;
uniform float rimLight;
uniform float specularity;
uniform float specularExponent;
//uniform vec3  rotation;
uniform int   maxIterations;
uniform int   stepLimit;
uniform float epsilonScale;

uniform float aoSteps;
uniform float fogDistance;
uniform float glowDepth;
uniform float glowMulti;
uniform vec3  glowColour;

uniform bool  Rave;
uniform float Pulse;
uniform float PulseScale;

uniform mat3 viewRotation;
uniform mat3 objRotation;

uniform bool backgroundGradient;
uniform float fov;
#define PI 3.141592653
#define MIN_EPSILON 3e-7

uniform sampler2D texture;
varying vec3 Position;

vec2 texelSize = vec2(1.0/width, 1.0/height);
float aspectRatio = width / height;

//eye = float3(0, 0, camera.w) * viewRotation;
//lightSource = light * viewRotation * 100.0;
vec3 eye = (camera + cameraFine) * objRotation;
//if (eye == float3(0, 0, 0)) eye = float3(0, 0.0001, 0);


// Super sampling
float sampleStep = 1.0 / float(antialiasing + 1);
float sampleContribution = 1.0 / pow(float(antialiasing + 1), 2.0);
float pixel_scale = 1.0 / max(width, height);




// Scalar derivative approach by Enforcer:
// http://www.fractalforums.com/mandelbulb-implementation/realtime-renderingoptimisations/
void powN(inout vec3 z, float zr0, inout float dr)
{
	float zo0 = asin(z.z / zr0);
	float zi0 = atan(z.y, z.x);
	float zr = pow(zr0, power - 1.0);
	float zo = (zo0) * power;
	float zi = (zi0) * power;
	float czo = cos(zo);

	dr = zr * dr * power + 1.0;
	zr *= zr0;

    z = zr * vec3(czo*cos(zi), czo*sin(zi), -sin(zo));
}

// The fractal calculation
//
// Calculate the closest distance to the fractal boundary and use this
// distance as the size of the step to take in the ray marching.
//
// Fractal formula:
//	  z' = z^p + c
//
// For each iteration we also calculate the derivative so we can estimate
// the distance to the nearest point in the fractal set, which then sets the
// maxiumum step we can move the ray forward before having to repeat the calculation.
//
//	 dz' = p * z^(p-1)
//
float DE(vec3 z0, inout float min_dist)
{
	vec3 c = julia ? julia_c : z0; // Julia set has fixed c, Mandelbrot c changes with location
	vec3 z = z0;

	float dr = 1.0;
	float r	 = length(z);
	if (r < min_dist) min_dist = r;

	for (int n = 0; n < maxIterations; n++) {
		powN(z, r, dr);

		z += c;
        if(Pulse>0.0) z *= sin(Pulse*0.5+0.5)*PulseScale;

		if (radiolaria && z.y > radiolariaFactor) z.y = radiolariaFactor;


		r = length(z);
		if (r < min_dist) min_dist = r;
		if (r > bailout) break;
	}

	return 0.5 * log(r) * r / dr;
}

// Intersect bounding sphere
//
// If we intersect then set the tmin and tmax values to set the start and
// end distances the ray should traverse.
bool intersectBoundingSphere(vec3 origin,
							 vec3 direction,
							 out float tmin,
							 out float tmax)
{
	bool hit = false;

	float b = dot(origin, direction);
	float c = dot(origin, origin) - bounding;
	float disc = b*b - c;			// discriminant
	tmin = tmax = 0.0;

	if (disc > 0.0) {
		// Real root of disc, so intersection
		float sdisc = sqrt(disc);
		float t0 = -b - sdisc;			// closest intersection distance
		float t1 = -b + sdisc;			// furthest intersection distance

		if (t0 >= 0.0) {
			// Ray intersects front of sphere
			float min_dist;
			vec3 z = origin + t0 * direction;
			tmin = DE(z, min_dist);
			tmax = t0 + t1;
		} else if (t0 < 0.0) {
			// Ray starts inside sphere
			float min_dist;
			vec3 z = origin;
			tmin = DE(z, min_dist);
			tmax = t1;
		}
		hit = true;
	}

	return hit;
}


// Calculate the gradient in each dimension from the intersection point
vec3 estimate_normal(vec3 z, float e)
{
	float min_dst;	// Not actually used in this particular case
	vec3 z1 = z + vec3(e, 0, 0);
	vec3 z2 = z - vec3(e, 0, 0);
	vec3 z3 = z + vec3(0, e, 0);
	vec3 z4 = z - vec3(0, e, 0);
	vec3 z5 = z + vec3(0, 0, e);
	vec3 z6 = z - vec3(0, 0, e);

	float dx = DE(z1, min_dst) - DE(z2, min_dst);
	float dy = DE(z3, min_dst) - DE(z4, min_dst);
	float dz = DE(z5, min_dst) - DE(z6, min_dst);

	return normalize(vec3(dx, dy, dz) / (2.0*e));
}


// Computes the direct illumination for point pt with normal N due to
// a point light at light and a viewer at eye.
vec3 Phong(vec3 pt, vec3 N, out float specular)
{
	vec3 diffuse	= vec3(0);			// Diffuse contribution
	vec3 color	= vec3(0);
	specular = 0.0;

	vec3 L = normalize(light * objRotation - pt); // find the vector to the light
	float  NdotL = dot(N, L);			// find the cosine of the angle between light and normal

	if (NdotL > 0.0) {
		// Diffuse shading
		diffuse = diffuseColor.rgb + abs(N) * colorSpread;
		diffuse *= lightColor.rgb * NdotL;

		// Phong highlight
		vec3 E = normalize(eye - pt);		// find the vector to the eye
		vec3 R = L - 2.0 * NdotL * N;		// find the reflected vector
		float  RdE = dot(R,E);

		if (RdE <= 0.0) {
			specular = specularity * pow(abs(RdE), specularExponent);
		}
	} else {
		diffuse = diffuseColor.rgb * abs(NdotL) * rimLight;
	}

	return (ambientColor.rgb * ambientColor.a) + diffuse;
}


// Define the ray direction from the pixel coordinates
vec3 rayDirection(vec2 p)
{

    float fov_multi = tan(fov * 0.017453292 * 0.5);

    vec3 direction = vec3(
        p.x * fov_multi * aspectRatio,
        p.y * fov_multi,
        exp(cameraZoom)
    );


//	vec3 direction = vec3( 2.0 * aspectRatio * p.x / float(size.x) - aspectRatio,
//					      -2.0 * p.y / float(size.y) + 1.0,
//						  -2.0 * exp(cameraZoom));

	return normalize(direction * viewRotation * objRotation);
}


// Calculate the output colour for each input pixel
vec4 renderPixel(vec2 pixel)
{
	float tmin, tmax;
	vec3 ray_direction = rayDirection(pixel);
	vec4 pixel_color = backgroundColor;

    float aoScale = aoSteps / epsilonScale;

	if(intersectBoundingSphere(eye, ray_direction, tmin, tmax)) {

        vec3 ray = eye + tmin * ray_direction;

        float dist, ao;
        float min_dist = 4.0;
        float ray_length = tmin;
        float eps = MIN_EPSILON;

        // number of raymarching steps scales inversely with factor
        int max_steps = int(float(stepLimit) / epsilonScale);

        int i;
        float f;

        for (i = 0; i < max_steps; ++i) {
            dist = DE(ray, min_dist);

            // March ray forward
            f = epsilonScale * dist;
            ray += f * ray_direction;
            ray_length += f;

            // Are we within the intersection threshold or completely missed the fractal
            if (dist < eps || ray_length > tmax) {
                break;
            }

            // Set the intersection threshold as a function of the ray length away from the camera
            //eps = max(max(MIN_EPSILON, eps_start), pixel_scale * pow(ray_length, epsilonScale));
            eps = max(MIN_EPSILON, pixel_scale * ray_length);
        }


        ao	= 1.0 - clamp(1.0 - min_dist * min_dist, 0.0, 1.0) * ambientOcclusion;

        // Found intersection?
        if (dist < eps) {

            if (phong) {
                vec3 normal = estimate_normal(ray, eps/2.0);
                float specular = 0.0;
                pixel_color.rgb = Phong(ray, normal, specular);

                if (shadows > 0.0) {
                    // The shadow ray will start at the intersection point and go
                    // towards the point light. We initially move the ray origin
                    // a little bit along this direction so that we don't mistakenly
                    // find an intersection with the same point again.
                    vec3 light_direction = normalize((light - ray) * objRotation);
                    ray += normal * eps * 2.0;

                    float min_dist2;
                    dist = 4.0;

                    for (int j = 0; j < max_steps; ++j) {
                        dist = DE(ray, min_dist2);

                        // March ray forward
                        f = epsilonScale * dist;
                        ray += f * light_direction;

                        // Are we within the intersection threshold or completely missed the fractal
                        if (dist < eps || dot(ray, ray) > bounding * bounding) break;
                    }

                    // Again, if our estimate of the distance to the set is small, we say
                    // that there was a hit and so the source point must be in shadow.
                    if (dist < eps) {
                        pixel_color.rgb *= 1.0 - shadows;
                    } else {
                        // Only add specular component when there is no shadow
                        pixel_color.rgb += specular;
                    }
                } else {
                    pixel_color.rgb += specular;
                }
            } else {
                // Just use the base colour
                pixel_color.rgb = diffuseColor.rgb;
            }

            ao *= 1.0 - min(1.0, float(i) / aoScale) * ambientOcclusionEmphasis * 2.0;

            pixel_color.rgb *= ao;
            pixel_color.a = 1.0;

        } else {
            if(backgroundGradient) {
                pixel_color.rgb = backgroundColor.rgb * (1.0-min(1.0, float(i) / aoScale));
                pixel_color.a = backgroundColor.a;
            }
        }

        if(fogDistance>0.0) {
            float fog_alpha = min(ray_length*ray_length,fogDistance)/fogDistance;
            pixel_color.rgb = backgroundColor.xyz * fog_alpha + pixel_color.rgb * (1.0 - fog_alpha);
        }

        if(glowDepth>0.0) {
            float glow_alpha = min(min_dist,glowDepth)/glowDepth;
            if(Rave) glow_alpha += ao;

            glow_alpha*=glow_alpha;

            //colour distance from centre
            pixel_color.rgb = pixel_color.rgb * glow_alpha + glowMulti * glowColour.xyz * (1.0-glow_alpha);
        }


	}

	return pixel_color;
}


// The main loop
void main()
{
	vec4 c = vec4(0, 0, 0, 1.0);
	vec2 p = vec2(Position);// * size;

	if (antialiasing > 0) {
		// Average detailSuperSample^2 points per pixel
		for (float i = 0.0; i < 1.0; i += sampleStep)
			for (float j = 0.0; j < 1.0; j += sampleStep)
				c += sampleContribution * renderPixel(p + vec2(i, j) * texelSize);
	} else {
		c = renderPixel(p);
	}

	//if (c.a <= 0.0) discard;

	// Return the final color which is still the background color if we didn't hit anything.
	gl_FragColor = c;
}
