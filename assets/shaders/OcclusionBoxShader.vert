#version 460

layout(location = 0) in vec3 a_Position;

uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_Model;

uniform vec3 u_BoundsCenterMS;
uniform vec3 u_BoundsHalfExtentsMS;

void main()
{
    // a_Position is the unit cube vertex in [-1, +1] on each axis
    vec3 positionMS = u_BoundsCenterMS + a_Position * u_BoundsHalfExtentsMS;

    vec4 positionWS = u_Model * vec4(positionMS, 1.0);
    gl_Position = u_Projection * u_View * positionWS;
}