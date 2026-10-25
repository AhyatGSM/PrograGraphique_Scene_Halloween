//-----------------------------------------------------------------------------
// scene.frag - Fragment shader de MyScene
//
// Base : lighting_dir_point_spot.frag du cours (modele de Blinn-Phong avec
// lumiere directionnelle + lumieres ponctuelles + spot). Ajouts :
//   1. jusqu'a 20 lumieres ponctuelles (au lieu de 3), chacune avec sa couleur
//   2. une lumiere ambiante globale (dans le shader du cours, l'ambiance
//      venait uniquement de la lampe torche)
//   3. transparence "tout ou rien" : les pixels transparents de la texture
//      (feuilles des arbres, anneaux de Saturne) ne sont pas dessines
//   4. eclairage des deux faces (feuilles et anneaux sont des surfaces fines)
//   5. objets "non eclaires" (ciel, Saturne, citrouilles, feu follet) :
//      affiches avec leurs vraies couleurs, comme s'ils brillaient
//   6. brouillard : les objets lointains se fondent dans la couleur du ciel
//-----------------------------------------------------------------------------
#version 330 core

struct Material
{
	sampler2D diffuseMap;
	vec3 specular;
	float shininess;
};

struct DirectionalLight
{
	vec3 direction;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight
{
	vec3 position;
	vec3 color;
	float linear;
	float exponent;
};

struct SpotLight
{
	vec3 position;
	vec3 direction;
	float cosInnerCone;
	float cosOuterCone;
	vec3 diffuse;
	vec3 specular;
	int on;
	float linear;
	float exponent;
};

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

#define MAX_POINT_LIGHTS 20

uniform DirectionalLight sunLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numPointLights;		// nombre de lumieres ponctuelles utilisees
uniform SpotLight spotLight;
uniform Material material;
uniform vec3 viewPos;

uniform vec3 ambientLight;		// lumiere ambiante de toute la scene
uniform vec3 fogColor;			// couleur du brouillard
uniform float fogDensity;		// epaisseur du brouillard
uniform int unlit;				// 1 = objet non eclaire (il brille)
uniform float unlitBrightness;	// intensite d'un objet non eclaire (1 = couleurs normales)
uniform int isSky;				// 1 = c'est le ciel etoile (brouillard a l'horizon)

out vec4 frag_color;

//-----------------------------------------------------------------------------
// Diffus + speculaire (Blinn-Phong) pour une lumiere venant de lightDir
//-----------------------------------------------------------------------------
vec3 blinnPhong(vec3 lightDir, vec3 lightColor, vec3 specColor, vec3 normal, vec3 viewDir, vec3 baseColor)
{
	// Diffus
	float NdotL = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = lightColor * NdotL * baseColor;

	// Speculaire (seulement si la face est eclairee)
	vec3 halfDir = normalize(lightDir + viewDir);
	float NdotH = max(dot(normal, halfDir), 0.0);
	vec3 specular = specColor * material.specular * pow(NdotH, material.shininess);
	if (NdotL <= 0.0)
		specular = vec3(0.0);

	return diffuse + specular;
}

void main()
{
	vec4 texel = texture(material.diffuseMap, TexCoord);

	// 3. Transparence : pixel transparent -> on ne dessine rien
	if (texel.a < 0.5)
		discard;

	vec3 baseColor = texel.rgb;

	// 5. Objet non eclaire : couleur de la texture telle quelle, sans brouillard
	if (unlit == 1)
	{
		vec3 c = baseColor * unlitBrightness;

		// Ciel : pres de l'horizon, il se fond dans la couleur du brouillard
		// (sinon on verrait une coupure nette entre les arbres dans la brume et le ciel)
		if (isSky == 1)
		{
			float h = normalize(FragPos - viewPos).y;	// 0 = horizon, 1 = au-dessus de nous
			c = mix(fogColor, c, smoothstep(0.0, 0.35, h));
		}

		frag_color = vec4(c, 1.0);
		return;
	}

	// 4. Eclairage des deux faces : si on voit l'arriere d'une face, on retourne la normale
	vec3 normal = normalize(Normal);
	if (!gl_FrontFacing)
		normal = -normal;

	vec3 viewDir = normalize(viewPos - FragPos);

	// 2. Ambiance
	vec3 color = ambientLight * baseColor;

	// Lumiere directionnelle (Saturne)
	color += blinnPhong(normalize(-sunLight.direction), sunLight.diffuse, sunLight.specular, normal, viewDir, baseColor);

	// 1. Lumieres ponctuelles avec attenuation 1 / (1 + Kl.d + Kq.d^2)
	for (int i = 0; i < numPointLights; i++)
	{
		vec3 toLight = pointLights[i].position - FragPos;
		float d = length(toLight);
		float attenuation = 1.0 / (1.0 + pointLights[i].linear * d + pointLights[i].exponent * d * d);
		color += attenuation * blinnPhong(toLight / d, pointLights[i].color, pointLights[i].color, normal, viewDir, baseColor);
	}

	// Spot (lampe torche) avec cone adouci
	if (spotLight.on == 1)
	{
		vec3 toLight = spotLight.position - FragPos;
		float d = length(toLight);
		vec3 lightDir = toLight / d;
		float cosDir = dot(-lightDir, normalize(spotLight.direction));
		float spotIntensity = smoothstep(spotLight.cosOuterCone, spotLight.cosInnerCone, cosDir);
		float attenuation = 1.0 / (1.0 + spotLight.linear * d + spotLight.exponent * d * d);
		color += attenuation * spotIntensity * blinnPhong(lightDir, spotLight.diffuse, spotLight.specular, normal, viewDir, baseColor);
	}

	// 6. Brouillard exponentiel : plus l'objet est loin, plus il prend la couleur du ciel
	float dist = length(viewPos - FragPos);
	float visibility = exp(-pow(fogDensity * dist, 2.0));
	color = mix(fogColor, color, clamp(visibility, 0.0, 1.0));

	frag_color = vec4(color, 1.0);
}
