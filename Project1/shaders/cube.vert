#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec2 aEmissionCoords;   // 
out vec2 TexCoords;
out vec2 EmissionCoords;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

//the normal matrix - transforming the normal vector for a vertex from local to world space 
//(no translation and avoiding the effects of non-uniform scale)
uniform mat3 normalMat;

void main()
{
TexCoords = aTexCoords;
EmissionCoords = aEmissionCoords; //
	//the fragment position in world space (multiplied only with the model matrix and without w component)
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = normalMat * aNormal;  
    
    gl_Position = proj * view * vec4(FragPos, 1.0);
}