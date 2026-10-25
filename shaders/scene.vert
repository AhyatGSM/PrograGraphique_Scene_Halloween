//-----------------------------------------------------------------------------
// scene.vert - Vertex shader de MyScene
//
// Identique dans l'esprit a celui du cours : transforme chaque sommet avec
// les matrices model / view / projection et transmet au fragment shader
// la position dans le monde, la normale et les coordonnees de texture.
//-----------------------------------------------------------------------------
#version 330 core

layout (location = 0) in vec3 pos;			// position (attribut 0 du Mesh)
layout (location = 1) in vec3 normal;		// normale  (attribut 1 du Mesh)
layout (location = 2) in vec2 texCoord;		// UV       (attribut 2 du Mesh)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;	// position du fragment dans le monde
out vec3 Normal;	// normale dans le monde
out vec2 TexCoord;

void main()
{
	FragPos  = vec3(model * vec4(pos, 1.0));

	// Matrice des normales : garde les normales correctes meme si l'objet
	// est agrandi differemment selon les axes
	Normal   = mat3(transpose(inverse(model))) * normal;

	TexCoord = texCoord;

	gl_Position = projection * view * vec4(FragPos, 1.0);
}
