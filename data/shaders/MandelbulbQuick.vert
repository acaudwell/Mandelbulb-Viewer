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
varying vec3 Position;

void main()
{
   vec4 pos = gl_Vertex;
   pos.xy = sign(pos.xy);
   Position = vec3(pos.x, pos.y, 0.0);
   gl_Position = vec4(pos.xy, 0, 1);

   float ratio = width / height;

//	gl_Position = ftransform();
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
//	Position = vec3(gl_MultiTexCoord0);
}
