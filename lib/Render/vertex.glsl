// Copyright (C) 2012 PiB <pixelbound@gmail.com>
//  
// EQuilibre is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.attribute vec3 a_position;

attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec3 a_texCoords;
attribute vec4 a_color;

uniform mat4 u_modelViewMatrix;
uniform mat4 u_projectionMatrix;

uniform int u_mapMaterialSlots;
uniform vec3 u_materialSlotMap[256];

const int NO_LIGHTING = 0;
const int BAKED_LIGHTING = 1;
const int DEBUG_VERTEX_COLOR = 2;
const int DEBUG_TEX_FACTOR = 3;
const int DEBUG_DIFFUSE = 4;
uniform int u_lightingMode;

uniform float u_fogStart;
uniform float u_fogEnd;
uniform float u_fogDensity;

varying vec3 v_color;
varying float v_texFactor;
varying vec3 v_texCoords;
varying float v_fogFactor;

void main()
{
    vec4 viewPos = u_modelViewMatrix * vec4(a_position, 1.0);
    gl_Position = u_projectionMatrix * viewPos;
    
    // Transform texture coordinates if using the material map.
    float baseTex = a_texCoords.z - 1.0;
    vec2 baseTexCoords = a_texCoords.xy;
    vec3 matInfo = u_materialSlotMap[int(baseTex)];
    vec2 mappedTexCoords = baseTexCoords * matInfo.xy;
    float mappedTex = matInfo.z - 1.0;
    float finalTex = (u_mapMaterialSlots > 0) ? mappedTex : baseTex;
    vec2 finalTexCoords = (u_mapMaterialSlots > 0) ? mappedTexCoords : baseTexCoords;
    v_texCoords = vec3(finalTexCoords, finalTex);
    
    if(u_lightingMode == DEBUG_VERTEX_COLOR)
    {
        v_color = a_color.xyz;
        v_texFactor = 0.0;
    }
    else if(u_lightingMode == DEBUG_TEX_FACTOR)
    {
        v_color = a_color.www;
        v_texFactor = 0.0;
    }
    else
    {
        v_color = vec3(0.0, 0.0, 0.0);
        v_texFactor = 1.0;
    }
    
    // Compute the fog factor.
    const float LOG2 = 1.442695;
    float fogDist = max(length(viewPos) - u_fogStart, 0.0);
    v_fogFactor = clamp(exp2(-pow(u_fogDensity * fogDist, 2.0) * LOG2), 0.0, 1.0);
}
