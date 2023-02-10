/** 
 * @file pbropaqueV.glsl
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */


#ifndef IS_HUD

//deferred opaque implementation

#ifdef HAS_SKIN
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
mat4 getObjectSkinnedTransform();
#else
uniform mat3 normal_matrix;
uniform mat4 modelview_projection_matrix;
#endif
uniform mat4 texture_matrix0;

uniform mat3 texture_basecolor_matrix;
uniform mat3 texture_normal_matrix;
uniform mat3 texture_metallic_roughness_matrix;
uniform mat3 texture_emissive_matrix;

in vec3 position;
in vec4 diffuse_color;
in vec3 normal;
in vec4 tangent;
in vec2 texcoord0;

out vec2 basecolor_texcoord;
out vec2 normal_texcoord;
out vec2 metallic_roughness_texcoord;
out vec2 emissive_texcoord;
 
out vec4 vertex_color;

out vec3 vary_tangent;
flat out float vary_sign;
out vec3 vary_normal;

void main()
{
#ifdef HAS_SKIN
	mat4 mat = getObjectSkinnedTransform();

	mat = modelview_matrix * mat;

	vec3 pos = (mat*vec4(position.xyz,1.0)).xyz;

	gl_Position = projection_matrix*vec4(pos,1.0);

#else
	//transform vertex
	gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0); 
#endif
	
	basecolor_texcoord = (texture_matrix0 * vec4(texture_basecolor_matrix * vec3(texcoord0,1), 1)).xy;
	normal_texcoord = (texture_matrix0 * vec4(texture_normal_matrix * vec3(texcoord0,1), 1)).xy;
	metallic_roughness_texcoord = (texture_matrix0 * vec4(texture_metallic_roughness_matrix * vec3(texcoord0,1), 1)).xy;
	emissive_texcoord = (texture_matrix0 * vec4(texture_emissive_matrix * vec3(texcoord0,1), 1)).xy;

#ifdef HAS_SKIN
	vec3 n = (mat*vec4(normal.xyz+position.xyz,1.0)).xyz-pos.xyz;
	vec3 t = (mat*vec4(tangent.xyz+position.xyz,1.0)).xyz-pos.xyz;
#else //HAS_SKIN
	vec3 n = normal_matrix * normal;
	vec3 t = normal_matrix * tangent.xyz;
#endif

    vary_tangent = normalize(t);
    vary_sign = tangent.w;
    vary_normal = normalize(n);
	
	vertex_color = diffuse_color;
}

#else 

// fullbright HUD implementation

uniform mat4 modelview_projection_matrix;

uniform mat4 texture_matrix0;

uniform mat3 texture_basecolor_matrix;
uniform mat3 texture_normal_matrix;
uniform mat3 texture_metallic_roughness_matrix;
uniform mat3 texture_emissive_matrix;

in vec3 position;
in vec4 diffuse_color;
in vec2 texcoord0;

out vec2 basecolor_texcoord;
out vec2 emissive_texcoord;
 
out vec4 vertex_color;

void main()
{
    //transform vertex
    gl_Position = modelview_projection_matrix * vec4(position.xyz, 1.0); 

    basecolor_texcoord = (texture_matrix0 * vec4(texture_basecolor_matrix * vec3(texcoord0,1), 1)).xy;
    emissive_texcoord = (texture_matrix0 * vec4(texture_emissive_matrix * vec3(texcoord0,1), 1)).xy;

    vertex_color = diffuse_color;
}

#endif


