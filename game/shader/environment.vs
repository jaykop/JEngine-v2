#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

////////////////////////////
// uniform variables
////////////////////////////
uniform mat4 m4_translate;
uniform mat4 m4_scale;
uniform mat4 m4_rotate;
uniform mat4 m4_viewport;
uniform mat4 m4_projection;

////////////////////////////
// out variables
////////////////////////////
out	vec3 v3_outNormal;
out	vec3 v3_outFragmentPosition;
	
////////////////////////////
// entry point
////////////////////////////
void main()
{
	//mat4 model = m4_scale * m4_rotate * m4_translate;
	//mat4 mvp = transpose(m4_projection) * transpose(m4_viewport)* transpose(model);
	//vec4 v4_position = vec4(position, 1);
	//gl_Position = mvp * v4_position;
	
    //v3_outFragmentPosition = vec3(transpose(model) * v4_position);
	//// v3_outNormal = normal;
	//v3_outNormal = mat3(transpose(inverse(transpose(model)))) * normal;

	mat4 model = m4_scale * m4_rotate * m4_translate;
	v3_outNormal = normal;
	v3_outFragmentPosition = mat3(transpose(model)) * position;
	
	vec4 v4_position = vec4(position, 1);
	gl_Position = transpose(m4_projection) * transpose(m4_viewport)* transpose(model) * v4_position;
}
