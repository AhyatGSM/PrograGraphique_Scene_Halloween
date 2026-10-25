//-----------------------------------------------------------------------------
// This code demonstrates how to implement the Blinn-Phong lighting model for
// multiple light sources using OpenGL 3.3 or a forward compatible
// OpenGL 3.0 rendering context.
//-----------------------------------------------------------------------------
// Scene du devoir : une nuit d'Halloween. Un petit garcon eclaire la porte d'une
// maison hantee pendant que des monstres se reunissent autour d'un chaudron ;
// trois fantomes tournent autour du domaine, entoure d'une cloture et d'une foret,
// sous un ciel etoile ou brille Saturne.
// Ce programme reprend Lighting_Multiple.cpp 


// Touches : C visite automatique, clic gauche + souris pour regarder autour,
//           W A S D Z X se deplacer, F lampe torche, F1 fil de fer, Echap quitter.
//-----------------------------------------------------------------------------

#include <iostream>
#include <sstream>
#include <string>
#include <vector>	// listes d'objets et de lumieres
#include <map>		// pour ne charger chaque modele qu'une fois
#include <cmath>	// sinf, cosf (animations)
#define GLEW_STATIC
#include "GL/glew.h"	// Important - this header must come before glfw3 header
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "ShaderProgram.h"
#include "Texture2D.h"
#include "Camera.h"
#include "Mesh.h"


// Global Variables
const char* APP_TITLE = "Ma scene virtuelle";
int gWindowWidth = 1600;
int gWindowHeight = 900;
GLFWwindow* gWindow = NULL;
bool gWireframe = false;
bool gFlashlightOn = true;
glm::vec4 gClearColor(0.05f, 0.06f, 0.11f, 1.0f);	// bleu nuit, aussi utilise pour le brouillard

FPSCamera fpsCamera(glm::vec3(-1.5f, 1.7f, 9.0f));	// depart au bout du chemin, face a la maison
const double ZOOM_SENSITIVITY = -3.0;
const float MOVE_SPEED = 5.0; // units per second
const float MOUSE_SENSITIVITY = 0.1f;

// Camera automatique (touche C) et derniere position de la souris
bool   gAutoCamera = false;
float  gCamTime = 0.0f;
double gLastX = 0.0, gLastY = 0.0;

// Materiaux : intensite du reflet (speculaire) et brillance, selon la matiere
struct Material { glm::vec3 specular; float shininess; };
const Material MAT_MATTE = { glm::vec3(0.03f), 4.0f }, MAT_STANDARD = { glm::vec3(0.25f), 32.0f },
               MAT_STONE = { glm::vec3(0.15f), 12.0f }, MAT_METAL    = { glm::vec3(0.90f), 96.0f };

// Un objet de la scene : { .obj, texture, position, taille, rotation, materiau, lumineux, inclinaison }
struct SceneObject
{
	std::string obj, texture;
	glm::vec3 position, scale;
	float rotation;
	Material material = MAT_STANDARD;
	bool unlit = false;		// true = l'objet brille (pas d'eclairage)
	float tilt = 0.0f;		// inclinaison autour de l'axe Z
};

// positions principales de la scene (en metres)
const glm::vec3 SATURN_POS(15.0f, 32.0f, -65.0f), GATE_POS(-1.5f, 0.02f, 5.8f), CAULDRON_POS(-1.5f, 0.0f, -1.0f);
const glm::vec3 LANTERN_L = GATE_POS + glm::vec3(-2.6f, -0.02f, 1.8f), LANTERN_R = GATE_POS + glm::vec3(2.6f, -0.02f, 1.8f);
const glm::vec3 LAMP_L = LANTERN_L + glm::vec3(0.45f, 2.75f, 0.0f), LAMP_R = LANTERN_R + glm::vec3(-0.45f, 2.75f, 0.0f);	// cages des lanternes
const glm::vec3 TORCH_POS(-1.25f, 1.05f, -8.45f), TORCH_TARGET(-1.5f, 2.6f, -13.5f);	// lampe du garcon -> porte

// objets places a la main (les objets repetes sont ajoutes par buildScene)
std::vector<SceneObject> gObjects = {
	{ "models/sol.obj",             "textures/grass_texture.jpg",       { 0.0f, 0.0f, -10.0f },  glm::vec3(1.0f),  0.0f, MAT_MATTE },
	{ "models/maison_couleurs.obj", "textures/maison_palette.png",      { -1.5f, 0.0f, -17.8f }, glm::vec3(0.01f), 0.0f },
	{ "models/lampPost.obj",        "textures/lamp_post_diffuse.png",   { -6.5f, 0.1f, -9.0f },  glm::vec3(1.0f),  0.0f, MAT_METAL },
	{ "models/lampPost.obj",        "textures/lamp_post_diffuse.png",   {  3.3f, 0.1f, -8.8f },  glm::vec3(1.0f),  0.0f, MAT_METAL },
	{ "models/perso2.obj",          "textures/perso2_baseColor.png",    { -1.5f, 0.9f, -8.2f },  glm::vec3(1.0f), 180.0f },
	// Monstres en cercle autour du chaudron
	{ "models/wendigo_pose1.obj", "textures/wendigo_Material.001_baseColor.png", {  0.76f, 0.00f, -0.18f }, glm::vec3(0.90f), 246.0f },
	{ "models/snatcher.obj",      "textures/snatcher_baseColor.png",             {  0.47f, 0.03f, -2.38f }, glm::vec3(2.50f), 310.0f },
	{ "models/wendigo_pose2.obj", "textures/wendigo_Material.001_baseColor.png", { -1.50f, 0.00f, -3.40f }, glm::vec3(0.80f),   3.0f },
	{ "models/snatcher.obj",      "textures/snatcher_baseColor.png",             { -3.47f, 0.03f, -2.38f }, glm::vec3(2.30f),  50.0f },
	{ "models/wendigo_pose3.obj", "textures/wendigo_Material.001_baseColor.png", { -3.76f, 0.00f, -0.18f }, glm::vec3(0.85f), 114.0f },
	{ "models/araignee_couleurs.obj", "textures/araignee_palette.png", { -7.3f, 0.10f, -11.4f }, glm::vec3(0.5f),   0.0f },
	{ "models/feu_couleurs.obj",      "textures/feu_palette.png",      {  4.2f, 0.12f,  -2.5f }, glm::vec3(1.5f),   0.0f, MAT_MATTE },
	{ "models/feu_couleurs.obj",      "textures/feu_palette.png",      { -7.2f, 0.12f,  -2.5f }, glm::vec3(1.5f), 180.0f, MAT_MATTE },
	// Lanterne, bougies, petit cimetiere
	{ "models/SM_Lantern.obj",         "textures/T_HorrorProps_Palette.png", {  0.0f, 0.10f,  -9.1f }, glm::vec3(1.0f),   0.0f, MAT_METAL },
	{ "models/SM_Candle.obj",          "textures/T_HorrorProps_Palette.png", { -3.0f, 1.19f, -12.0f }, glm::vec3(1.0f),   0.0f },
	{ "models/SM_Candle.obj",          "textures/T_HorrorProps_Palette.png", {  0.0f, 1.19f, -12.0f }, glm::vec3(1.0f),   0.0f },
	{ "models/SM_Tombstone_Round.obj", "textures/T_HorrorProps_Palette.png", {  6.0f, 0.10f,  -8.4f }, glm::vec3(1.0f), -15.0f, MAT_STONE },
	{ "models/SM_Tombstone_Round.obj", "textures/T_HorrorProps_Palette.png", {  7.6f, 0.10f,  -7.4f }, glm::vec3(1.0f),  10.0f, MAT_STONE },
	{ "models/SM_Tombstone_Round.obj", "textures/T_HorrorProps_Palette.png", {  4.7f, 0.10f,  -7.5f }, glm::vec3(1.0f),   5.0f, MAT_STONE },
	{ "models/SM_Skull.obj",           "textures/T_HorrorProps_Palette.png", {  5.6f, 0.10f,  -7.0f }, glm::vec3(1.0f),  30.0f, MAT_STONE },
	// Lanternes du portail et une lueur dans chaque cage
	{ "models/lanterne2.obj", "textures/lanterne_part1.png",  LANTERN_L, glm::vec3(1.0f),   0.0f, MAT_METAL },
	{ "models/lanterne2.obj", "textures/lanterne_part1.png",  LANTERN_R, glm::vec3(1.0f), 180.0f, MAT_METAL },
	{ "models/light.obj",     "textures/lueur_lanterne.png", LAMP_L,    glm::vec3(0.12f),  0.0f, MAT_STANDARD, true },
	{ "models/light.obj",     "textures/lueur_lanterne.png", LAMP_R,    glm::vec3(0.12f),  0.0f, MAT_STANDARD, true },
};

// Lumieres : { position, couleur, Kl, Kq, effet (0 fixe, 1 flamme, 2 neon casse) }
struct PointLightDef { glm::vec3 position, color; float linear, exponent; int effect; };
std::vector<PointLightDef> gPointLights = {
	{ {  4.2f, 0.8f, -2.5f },   { 1.8f, 0.85f, 0.30f }, 0.22f, 0.20f, 1 },	// feux de camp
	{ { -7.2f, 0.8f, -2.5f },   { 1.8f, 0.85f, 0.30f }, 0.22f, 0.20f, 1 },
	{ { -6.0f, 3.9f, -9.0f },   { 0.9f, 0.85f, 0.70f }, 0.22f, 0.20f, 0 },	// lampadaires 
	{ {  3.8f, 3.9f, -8.8f },   { 0.8f, 0.90f, 1.00f }, 0.22f, 0.20f, 2 },
	{ { -1.44f, 4.25f, -12.6f },{ 1.3f, 0.80f, 0.40f }, 0.35f, 0.44f, 1 },	// lanterne de la porte
	{ { -1.5f, 6.4f, -12.9f },  { 1.2f, 0.65f, 0.25f }, 0.35f, 0.44f, 1 },	// fenetres 
	{ {  2.3f, 1.8f, -14.6f },  { 1.2f, 0.65f, 0.25f }, 0.35f, 0.44f, 1 },
	{ {  5.5f, 5.7f, -15.5f },  { 0.4f, 1.10f, 0.40f }, 0.35f, 0.44f, 1 },	// tour (vert)
	{ { -1.5f, 0.5f, 1.5f },    { 1.2f, 0.55f, 0.10f }, 0.35f, 0.44f, 1 },	// citrouilles du chemin
	{ { -1.5f, 0.5f, -8.5f },   { 1.2f, 0.55f, 0.10f }, 0.35f, 0.44f, 1 },
	{ {  0.0f, 0.4f, -9.1f },   { 1.0f, 0.75f, 0.40f }, 0.70f, 1.80f, 1 },	// lanterne du garcon
	{ CAULDRON_POS + glm::vec3(0.0f, 1.9f, 0.0f), { 0.5f, 1.4f, 0.3f }, 0.35f, 0.44f, 1 },	// potion
	{ GATE_POS + glm::vec3(0.0f, 3.2f, 0.6f), { 1.1f, 0.75f, 0.40f }, 0.35f, 0.44f, 1 },	// portail
	{ LAMP_L, { 1.2f, 0.85f, 0.45f }, 0.35f, 0.44f, 1 },	// lanternes du portail
	{ LAMP_R, { 1.2f, 0.85f, 0.45f }, 0.35f, 0.44f, 1 },
};

// Fantomes, seuls objets mobiles : 
struct Ghost { std::string obj; float height, startAngle; glm::vec3 lightColor; };
std::vector<Ghost> gGhosts = {
	{ "models/fantome1.obj", 3.0f,   0.0f, { 0.55f, 0.85f, 1.30f } },
	{ "models/fantome2.obj", 3.8f, 120.0f, { 0.60f, 1.30f, 0.80f } },
	{ "models/fantome3.obj", 2.6f, 240.0f, { 0.95f, 0.70f, 1.30f } },
};

// Visite automatique : positions de la camera et points regardes (7 s entre deux points)
std::vector<glm::vec3> CAM_POS = { { -1.5f, 3.0f, 16.0f }, { -1.5f, 2.2f, 4.5f }, { -9.0f, 2.6f, -6.5f },
	{ 0.6f, 2.6f, -12.0f }, { -1.1f, 1.35f, -10.4f }, { -1.4f, 1.2f, -9.5f }, { 6.5f, 2.4f, -4.0f },
	{ 2.0f, 7.0f, 6.0f }, { -1.5f, 3.0f, 16.0f } };
std::vector<glm::vec3> CAM_TARGET = { { -1.5f, 1.5f, -3.0f }, { -1.5f, 1.2f, -1.0f }, { -3.0f, 1.5f, -4.0f },
	{ -1.5f, 1.2f, -1.0f }, { -1.5f, 1.1f, -8.2f }, { -1.5f, 1.1f, -8.2f }, { 2.0f, 1.5f, -10.0f },
	{ -1.5f, 3.0f, -12.0f }, { -1.5f, 1.5f, -5.0f } };


// Function prototypes
void glfw_onKey(GLFWwindow* window, int key, int scancode, int action, int mode);
void glfw_onFramebufferSize(GLFWwindow* window, int width, int height);
void glfw_onMouseScroll(GLFWwindow* window, double deltaX, double deltaY);
void glfw_onMouseMove(GLFWwindow* window, double posX, double posY);
void update(double elapsedTime);
void showFPS(GLFWwindow* window);
bool initOpenGL();
void buildScene();
float lightEffect(int effect, float time, int index);

//-----------------------------------------------------------------------------
// Main Application Entry Point
//-----------------------------------------------------------------------------
int main()
{
	if (!initOpenGL())
	{
		// An error occured
		std::cerr << "GLFW initialization failed" << std::endl;
		return -1;
	}

	ShaderProgram lightingShader;
	lightingShader.loadShaders("shaders/scene.vert", "shaders/scene.frag");	// shaders de la scene (brouillard, transparence, objets lumineux)

	// Load meshes and textures
	// Un fichier utilise plusieurs fois (arbres, citrouilles...) n'est charge qu'une seule fois
	buildScene();
	std::map<std::string, Mesh*> meshes;
	std::map<std::string, Texture2D*> textures;
	gObjects.push_back({ "models/ciel.obj", "textures/Milkyway_Swan_Panorama.jpg", glm::vec3(0.0f), glm::vec3(0.0f), 0.0f });	// pour charger le ciel
	gObjects.push_back({ "models/fantome1.obj", "textures/fantome_part1.png", glm::vec3(0.0f), glm::vec3(0.0f), 0.0f });		// et les fantomes
	gObjects.push_back({ "models/fantome2.obj", "textures/fantome_part1.png", glm::vec3(0.0f), glm::vec3(0.0f), 0.0f });
	gObjects.push_back({ "models/fantome3.obj", "textures/fantome_part1.png", glm::vec3(0.0f), glm::vec3(0.0f), 0.0f });
	for (const SceneObject& o : gObjects)
	{
		if (!meshes[o.obj])       { meshes[o.obj] = new Mesh();          meshes[o.obj]->loadOBJ(o.obj); }
		if (!textures[o.texture]) { textures[o.texture] = new Texture2D(); textures[o.texture]->loadTexture(o.texture, true); }
	}
	gObjects.resize(gObjects.size() - 4);	// ciel et fantomes sont dessines a part

	double lastTime = glfwGetTime();

	// Rendering loop
	while (!glfwWindowShouldClose(gWindow))
	{
		showFPS(gWindow);

		double currentTime = glfwGetTime();
		double deltaTime = currentTime - lastTime;
		float time = (float)currentTime;	// temps pour les animations

		// Poll for and process events
		glfwPollEvents();
		update(deltaTime);

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 model(1.0), view(1.0), projection(1.0);

		// Create the View matrix
		view = fpsCamera.getViewMatrix();

		// update the view (camera) position
		glm::vec3 viewPos = fpsCamera.getPosition();

		// Visite automatique : la camera va d'un point au suivant (interpolation avec glm::mix)
		if (gAutoCamera)
		{
			gCamTime += (float)deltaTime;
			float t = fmodf(gCamTime / 7.0f, (float)(CAM_POS.size() - 1));
			int k = (int)t;
			float f = (t - k) * (t - k) * (3.0f - 2.0f * (t - k));	// lissage : ralentit pres des points
			viewPos = glm::mix(CAM_POS[k], CAM_POS[k + 1], f);
			view = glm::lookAt(viewPos, glm::mix(CAM_TARGET[k], CAM_TARGET[k + 1], f), glm::vec3(0.0f, 1.0f, 0.0f));
		}

		// Create the projection matrix
		projection = glm::perspective(glm::radians(fpsCamera.getFOV()), (float)gWindowWidth / (float)gWindowHeight, 0.1f, 200.0f);

		// Must be called BEFORE setting uniforms because setting uniforms is done
		// on the currently active shader program.
		lightingShader.use();
		lightingShader.setUniform("view", view);
		lightingShader.setUniform("projection", projection);
		lightingShader.setUniform("viewPos", viewPos);

		// Lumiere ambiante et brouillard
		lightingShader.setUniform("ambientLight", glm::vec3(0.12f, 0.12f, 0.17f));
		lightingShader.setUniform("fogColor", glm::vec3(gClearColor));
		lightingShader.setUniform("fogDensity", 0.03f);

		// Directional light
		// la lumiere vient de Saturne et eclaire le centre de la scene
		lightingShader.setUniform("sunLight.direction", glm::normalize(glm::vec3(0.0f, 0.0f, -8.0f) - SATURN_POS));
		lightingShader.setUniform("sunLight.diffuse", glm::vec3(0.45f, 0.42f, 0.55f));
		lightingShader.setUniform("sunLight.specular", glm::vec3(0.22f, 0.21f, 0.27f));

		// Fantomes : ronde sur une ellipse autour de la scene, en ondulant ; chacun porte une lumiere
		std::vector<glm::vec3> ghostPos;
		std::vector<float> ghostRot;
		std::vector<PointLightDef> lights = gPointLights;
		for (int g = 0; g < (int)gGhosts.size(); g++)
		{
			float a = glm::radians(gGhosts[g].startAngle + 14.0f * time);
			ghostPos.push_back(glm::vec3(-1.5f + 10.0f * sinf(a), gGhosts[g].height + 0.35f * sinf(time * 1.4f + g * 2.0f), -4.5f + 7.5f * cosf(a)));
			ghostRot.push_back(glm::degrees(atan2f(10.0f * cosf(a), -7.5f * sinf(a))));	// sens du deplacement
			lights.push_back({ ghostPos[g] + glm::vec3(0.0f, 0.6f, 0.0f), gGhosts[g].lightColor, 0.35f, 0.44f, 0 });	// lumiere du fantome
		}

		// Point lights : toutes les lumieres de la liste, avec leur effet (20 au maximum)
		lightingShader.setUniform("numPointLights", (int)lights.size());
		for (int i = 0; i < (int)lights.size(); i++)
		{
			std::string n = "pointLights[" + std::to_string(i) + "].";
			lightingShader.setUniform((n + "position").c_str(), lights[i].position);
			lightingShader.setUniform((n + "color").c_str(), lights[i].color * lightEffect(lights[i].effect, time, i));
			lightingShader.setUniform((n + "linear").c_str(), lights[i].linear);
			lightingShader.setUniform((n + "exponent").c_str(), lights[i].exponent);
		}

		// Spot light
		// la lampe est dans la main du garcon et eclaire la porte
		lightingShader.setUniform("spotLight.diffuse", glm::vec3(1.3f, 1.25f, 1.1f));
		lightingShader.setUniform("spotLight.specular", glm::vec3(0.6f, 0.6f, 0.6f));
		lightingShader.setUniform("spotLight.position", TORCH_POS);
		lightingShader.setUniform("spotLight.direction", glm::normalize(TORCH_TARGET - TORCH_POS));
		lightingShader.setUniform("spotLight.cosInnerCone", glm::cos(glm::radians(15.0f)));
		lightingShader.setUniform("spotLight.cosOuterCone", glm::cos(glm::radians(22.0f)));
		lightingShader.setUniform("spotLight.linear", 0.07f);
		lightingShader.setUniform("spotLight.exponent", 0.017f);
		lightingShader.setUniform("spotLight.on", gFlashlightOn ? 1 : 0);
		lightingShader.setUniformSampler("material.diffuseMap", 0);

		// Ciel etoile : grande sphere qui suit la camera, dessinee derriere tout le reste
		glDepthMask(GL_FALSE);
		lightingShader.setUniform("model", glm::translate(glm::mat4(1.0), viewPos) * glm::scale(glm::mat4(1.0), glm::vec3(150.0f)));
		lightingShader.setUniform("unlit", 1);
		lightingShader.setUniform("isSky", 1);
		lightingShader.setUniform("unlitBrightness", 0.8f);
		textures["textures/Milkyway_Swan_Panorama.jpg"]->bind(0);
		meshes["models/ciel.obj"]->draw();
		lightingShader.setUniform("isSky", 0);
		lightingShader.setUniform("unlitBrightness", 1.0f);
		glDepthMask(GL_TRUE);

		// Render the scene
		for (const SceneObject& o : gObjects)
		{
			model = glm::translate(glm::mat4(1.0), o.position)
				  * glm::rotate(glm::mat4(1.0), glm::radians(o.rotation), glm::vec3(0.0f, 1.0f, 0.0f))	// rotation et inclinaison de l'objet
				  * glm::rotate(glm::mat4(1.0), glm::radians(o.tilt), glm::vec3(0.0f, 0.0f, 1.0f))
				  * glm::scale(glm::mat4(1.0), o.scale);
			lightingShader.setUniform("model", model);
			lightingShader.setUniform("unlit", o.unlit ? 1 : 0);	// objet lumineux ou eclaire

			// Set material properties
			lightingShader.setUniform("material.specular", o.material.specular);	// materiau propre a chaque objet
			lightingShader.setUniform("material.shininess", o.material.shininess);

			textures[o.texture]->bind(0);		// set the texture before drawing.  Our simple OBJ mesh loader does not do materials yet.
			meshes[o.obj]->draw();				// Render the OBJ mesh
			textures[o.texture]->unbind(0);
		}

		// Fantomes lumineux et ampoule de la lampe du garcon
		lightingShader.setUniform("unlit", 1);
		lightingShader.setUniform("unlitBrightness", 1.5f);
		textures["textures/fantome_part1.png"]->bind(0);
		for (int g = 0; g < (int)gGhosts.size(); g++)
		{
			lightingShader.setUniform("model", glm::translate(glm::mat4(1.0), ghostPos[g])
				* glm::rotate(glm::mat4(1.0), glm::radians(ghostRot[g]), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0), glm::vec3(1.4f)));
			meshes[gGhosts[g].obj]->draw();
		}
		lightingShader.setUniform("unlitBrightness", 1.0f);
		if (gFlashlightOn)
		{
			lightingShader.setUniform("model", glm::translate(glm::mat4(1.0), TORCH_POS) * glm::scale(glm::mat4(1.0), glm::vec3(0.06f)));
			textures["textures/lueur_lanterne.png"]->bind(0);
			meshes["models/light.obj"]->draw();
		}

		// Swap front and back buffers
		glfwSwapBuffers(gWindow);

		lastTime = currentTime;
	}

	glfwTerminate();

	return 0;
}

//-----------------------------------------------------------------------------
// Ajoute tous les morceaux d'un modele decoupé (un .obj par texture),
// avec la meme position, taille et rotation : models/NOM_partN.obj + textures/NOM_partN<suffixe>
//-----------------------------------------------------------------------------
void addParts(std::string name, std::vector<std::string> suffixes, glm::vec3 pos, glm::vec3 scale,
			  float rotation, Material material, bool unlit = false, float tilt = 0.0f)
{
	for (int p = 1; p <= (int)suffixes.size(); p++)
	{
		std::string part = name + "_part" + std::to_string(p);
		gObjects.push_back({ "models/" + part + ".obj", "textures/" + part + suffixes[p - 1], pos, scale, rotation, material, unlit, tilt });
	}
}

//-----------------------------------------------------------------------------
// Nombre "aleatoire" entre a et b, identique a chaque lancement
//-----------------------------------------------------------------------------
float randomFloat(float a, float b)
{
	static unsigned int seed = 12345;
	seed = seed * 1103515245u + 12345u;
	return a + (float)((seed >> 16) & 0x7FFF) / 32767.0f * (b - a);
}

//-----------------------------------------------------------------------------
// Ajoute les modeles en morceaux, le chemin de citrouilles, la cloture et la foret
//-----------------------------------------------------------------------------
void buildScene()
{
	// Modeles decoupes en plusieurs morceaux
	addParts("chat",     { ".jpeg", ".png", ".jpeg", ".png", ".jpeg" }, { -0.75f, 1.81f, -13.05f }, glm::vec3(0.10f), 0.0f, MAT_STANDARD);
	addParts("chaudron", { ".jpeg", ".jpeg", ".jpeg", ".jpeg" }, CAULDRON_POS + glm::vec3(0.0f, 0.671f, 0.0f), glm::vec3(0.011f), 0.0f, MAT_METAL);
	addParts("saturne",  { ".png", ".jpeg", ".png", "_palette.png" }, SATURN_POS, glm::vec3(2.5f), 20.0f, MAT_STANDARD, true, 25.0f);
	addParts("portail",  { ".png", ".png", ".png", ".png", "_palette.png", ".png", ".png" }, GATE_POS, glm::vec3(1.0f), 0.0f, MAT_STONE);

	// Chemin : deux rangees de citrouilles lumineuses (x = -5.4 et 2.4), tournees vers le chemin
	for (float z = 3.4f; z >= -9.61f; z -= 2.6f)
	{
		float y = (z < -5.0f) ? 0.1f : 0.0f;	// le socle du decor est 10 cm plus haut
		gObjects.push_back({ "models/SM_Pumpkin_JackOLantern.obj", "textures/T_HorrorProps_Palette.png", { -5.4f, y, z }, glm::vec3(1.0f),  90.0f, MAT_STANDARD, true });
		gObjects.push_back({ "models/SM_Pumpkin_JackOLantern.obj", "textures/T_HorrorProps_Palette.png", {  2.4f, y, z }, glm::vec3(1.0f), -90.0f, MAT_STANDARD, true });
	}

	// Cloture : rectangle x de -13.5 a 10.5, z de 5.8 (portail) a -27, murets de 2,7 m bout a bout
	float L = -13.5f, R = 10.5f, F = GATE_POS.z, B = -27.0f;
	glm::vec3 lines[5][2] = { { { -6.44f, 0, F }, { L, 0, F } }, { { 3.44f, 0, F }, { R, 0, F } },	// de chaque cote du portail
							  { { L, 0, F }, { L, 0, B } }, { { R, 0, F }, { R, 0, B } }, { { L, 0, B }, { R, 0, B } } };
	for (int s = 0; s < 5; s++)
	{
		glm::vec3 a = lines[s][0], b = lines[s][1];
		int count = (int)ceilf(glm::length(b - a) / 2.7f);
		for (int i = 0; i < count; i++)
			gObjects.push_back({ "models/muret.obj", "textures/portail_part7.png", a + (b - a) * ((i + 0.5f) / count),
								 glm::vec3(1.0f), (a.x == b.x) ? 90.0f : 0.0f, MAT_STONE });
	}
	float pillars[9][2] = { { -6.44f, F }, { 3.44f, F }, { L, F }, { R, F }, { L, B }, { R, B }, { L, (F + B) / 2 }, { R, (F + B) / 2 }, { (L + R) / 2, B } };
	for (int p = 0; p < 9; p++)
		gObjects.push_back({ "models/pilier.obj", "textures/portail_part6.png", { pillars[p][0], 0.0f, pillars[p][1] }, glm::vec3(1.0f), 0.0f, MAT_STONE });

	// Foret : 4 cercles d'arbres autour de (0, -10), hors de la cloture, du chemin et du trajet de la camera
	for (int ring = 0; ring < 4; ring++)
	{
		float radius = 17.0f + ring * 6.5f;
		int nbTrees = (int)(2.0f * glm::pi<float>() * radius / 6.0f);
		float offset = randomFloat(0.0f, 1.0f);
		for (int i = 0; i < nbTrees; i++)
		{
			float angle = 2.0f * glm::pi<float>() * (i + offset + randomFloat(-0.3f, 0.3f)) / nbTrees;
			float r = radius + randomFloat(-2.0f, 2.0f);
			float x = r * sinf(angle), z = -10.0f + r * cosf(angle);
			int type = (randomFloat(0.0f, 1.0f) < 0.5f) ? 1 : 2;
			float size = randomFloat(0.85f, 1.25f), rot = randomFloat(0.0f, 360.0f);

			bool forbidden = (x > L - 2 && x < R + 2 && z > B - 2 && z < F + 2) || (fabsf(x + 1.5f) < 6.4f && z > -6.0f);
			for (const glm::vec3& p : CAM_POS)
				if (glm::length(glm::vec2(x - p.x, z - p.z)) < 3.5f)
					forbidden = true;
			if (forbidden)
				continue;

			if (type == 1)	// modeles en centimetres, racines sous le sol
				addParts("arbre1", { ".png", ".png", ".png", ".png", ".png" }, { x, 0.5f * size, z }, glm::vec3(0.02f * size), rot, MAT_MATTE);
			else
				addParts("arbre2", { ".png", ".png" }, { x, 1.4f * size, z }, glm::vec3(0.012f * size), rot, MAT_MATTE);
		}
	}
}

//-----------------------------------------------------------------------------
// Intensite d'une lumiere selon son effet : 1 = flamme qui vacille, 2 = neon casse
//-----------------------------------------------------------------------------
float lightEffect(int effect, float time, int index)
{
	float phase = index * 1.7f;
	if (effect == 1)
		return 1.0f + 0.12f * sinf(time * 11.0f + phase) + 0.08f * sinf(time * 17.3f + 2 * phase) + 0.05f * sinf(time * 29.0f + 3 * phase);
	if (effect == 2)
	{
		float s = sinf(time * 1.3f) + sinf(time * 2.7f + 1.0f) + sinf(time * 4.1f + 2.0f);
		if (s > 1.6f) return 0.05f;										// coupure
		if (s > 1.3f) return (sinf(time * 90.0f) > 0.0f) ? 1.0f : 0.1f;	// clignotement
		return 0.9f + 0.1f * sinf(time * 60.0f);						// bourdonnement
	}
	return 1.0f;
}

//-----------------------------------------------------------------------------
// Initialize GLFW and OpenGL
//-----------------------------------------------------------------------------
bool initOpenGL()
{
	// Intialize GLFW
	// GLFW is configured.  Must be called before calling any GLFW functions
	if (!glfwInit())
	{
		// An error occured
		std::cerr << "GLFW initialization failed" << std::endl;
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);	// forward compatible with newer versions of OpenGL as they become available but not backward compatible (it will not run on devices that do not support OpenGL 3.3


	// Create an OpenGL 3.3 core, forward compatible context window
	gWindow = glfwCreateWindow(gWindowWidth, gWindowHeight, APP_TITLE, NULL, NULL);
	if (gWindow == NULL)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	// Make the window's context the current one
	glfwMakeContextCurrent(gWindow);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Failed to initialize GLEW" << std::endl;
		return false;
	}

	// Set the required callback functions
	glfwSetKeyCallback(gWindow, glfw_onKey);
	glfwSetFramebufferSizeCallback(gWindow, glfw_onFramebufferSize);
	glfwSetScrollCallback(gWindow, glfw_onMouseScroll);
	glfwSetCursorPosCallback(gWindow, glfw_onMouseMove);	// rotation de la camera a la souris

	// Le curseur n'est ni cache ni recentre (cela posait probleme sous WSL) : on tourne au clic gauche
	fpsCamera.rotate(0.0f, 0.0f);

	glClearColor(gClearColor.r, gClearColor.g, gClearColor.b, gClearColor.a);

    // Define the viewport dimensions
    int w, h;
    glfwGetFramebufferSize( gWindow, &w, &h); // For retina display
    glViewport(0, 0, w, h);

    //    glViewport(0, 0, gWindowWidth, gWindowHeight);

    glEnable(GL_DEPTH_TEST);

	return true;
}

//-----------------------------------------------------------------------------
// Is called whenever a key is pressed/released via GLFW
//-----------------------------------------------------------------------------
void glfw_onKey(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
	{
		gWireframe = !gWireframe;
		if (gWireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		// toggle the flashlight
		gFlashlightOn = !gFlashlightOn;
	}

	if (key == GLFW_KEY_C && action == GLFW_PRESS)	// lancer / arreter la visite automatique
	{
		gAutoCamera = !gAutoCamera;
		gCamTime = 0.0f;
	}
}

//-----------------------------------------------------------------------------
// Is called when the window is resized
//-----------------------------------------------------------------------------
void glfw_onFramebufferSize(GLFWwindow* window, int width, int height)
{
	gWindowWidth = width;
	gWindowHeight = height;

    // Define the viewport dimensions
    int w, h;
    glfwGetFramebufferSize( gWindow, &w, &h); // For retina display
    glViewport(0, 0, w, h);

    //    glViewport(0, 0, gWindowWidth, gWindowHeight);

}

//-----------------------------------------------------------------------------
// Called by GLFW when the mouse wheel is rotated
//-----------------------------------------------------------------------------
void glfw_onMouseScroll(GLFWwindow* window, double deltaX, double deltaY)
{
	double fov = fpsCamera.getFOV() + deltaY * ZOOM_SENSITIVITY;

	fov = glm::clamp(fov, 1.0, 120.0);

	fpsCamera.setFOV((float)fov);
}

//-----------------------------------------------------------------------------
// Souris : la camera tourne quand le clic gauche est enfonce
//-----------------------------------------------------------------------------
void glfw_onMouseMove(GLFWwindow* window, double posX, double posY)
{
	if (!gAutoCamera && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		fpsCamera.rotate((float)(gLastX - posX) * MOUSE_SENSITIVITY, (float)(gLastY - posY) * MOUSE_SENSITIVITY);
	gLastX = posX;
	gLastY = posY;
}

//-----------------------------------------------------------------------------
// Update stuff every frame
//-----------------------------------------------------------------------------
void update(double elapsedTime)
{
	// La rotation a la souris se fait dans glfw_onMouseMove ; pas de deplacement pendant la visite
	if (gAutoCamera)
		return;

	// Camera FPS movement

	// Forward/backward
	if (glfwGetKey(gWindow, GLFW_KEY_W) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getLook());
	else if (glfwGetKey(gWindow, GLFW_KEY_S) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getLook());

	// Strafe left/right
	if (glfwGetKey(gWindow, GLFW_KEY_A) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -fpsCamera.getRight());
	else if (glfwGetKey(gWindow, GLFW_KEY_D) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * fpsCamera.getRight());

	// Up/down
	if (glfwGetKey(gWindow, GLFW_KEY_Z) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * glm::vec3(0.0f, 1.0f, 0.0f));
	else if (glfwGetKey(gWindow, GLFW_KEY_X) == GLFW_PRESS)
		fpsCamera.move(MOVE_SPEED * (float)elapsedTime * -glm::vec3(0.0f, 1.0f, 0.0f));
}

//-----------------------------------------------------------------------------
// Code computes the average frames per second, and also the average time it takes
// to render one frame.  These stats are appended to the window caption bar.
//-----------------------------------------------------------------------------
void showFPS(GLFWwindow* window)
{
	static double previousSeconds = 0.0;
	static int frameCount = 0;
	double elapsedSeconds;
	double currentSeconds = glfwGetTime(); // returns number of seconds since GLFW started, as double float

	elapsedSeconds = currentSeconds - previousSeconds;

	// Limit text updates to 4 times per second
	if (elapsedSeconds > 0.25)
	{
		previousSeconds = currentSeconds;
		double fps = (double)frameCount / elapsedSeconds;
		double msPerFrame = 1000.0 / fps;

		// The C++ way of setting the window title
		std::ostringstream outs;
		outs.precision(3);	// decimal places
		outs << std::fixed
			<< APP_TITLE << "    "
			<< "FPS: " << fps << "    "
			<< "Frame Time: " << msPerFrame << " (ms)";
		glfwSetWindowTitle(window, outs.str().c_str());

		// Reset for next average.
		frameCount = 0;
	}

	frameCount++;
}