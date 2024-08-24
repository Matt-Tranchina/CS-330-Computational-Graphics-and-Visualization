///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;


	bReturn = CreateGLTexture(
		"../../Utilities/MY_textures/FrontCover.jpg",
		"Front Cover");

	bReturn = CreateGLTexture(
		"../../Utilities/MY_textures/Glass.jpg",
		"Glass");

	bReturn = CreateGLTexture(
		"../../Utilities/MY_textures/Glass2.jpg",
		"Glass2");

	bReturn = CreateGLTexture(
		"../../Utilities/MY_textures/Wood.jpg",
		"Wood");

	bReturn = CreateGLTexture(
		"../../Utilities/MY_textures/Black.jpg",
		"Black");

	bReturn = CreateGLTexture(
		"../../Utilities/MY_textures/Chrome.jpg",
		"Chrome");

	bReturn = CreateGLTexture(
		"../../Utilities/MY_Textures/Grey.jpg",
		"Grey");

	bReturn = CreateGLTexture(
		"../../Utilities/MY_textures/BackCover.jpg",
		"Back Cover");

	bReturn = CreateGLTexture(
		"../../Utilities/MY_Textures/Modern.jpg",
		"Modern");

	bReturn = CreateGLTexture(
		"../../Utilities/MY_Textures/Brass.jpg",
		"Brass");

	BindGLTextures();
}

/***********************************************************
*	DefineObjectMaterials()
*	
*	This method is used for configuring the various material
*	settings for all of the objects in the 3D scene.
************************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL woodMaterial;
	
	woodMaterial.ambientColor = glm::vec3(0.0f, 0.0f, 0.0f);
	woodMaterial.ambientStrength = 0.5f;
	woodMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.01f, 0.01f, 0.01f);
	woodMaterial.shininess = 10.0;
	woodMaterial.tag = "Wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	
	glassMaterial.ambientColor = glm::vec3(0.01f, 0.01f, 0.01f);
	glassMaterial.ambientStrength = 0.01f;
	glassMaterial.diffuseColor = glm::vec3(0.01f, 0.01f, 0.01f);
	glassMaterial.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	glassMaterial.shininess = 75.0;
	glassMaterial.tag = "Glass";
	
	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL glass2Material;

	glass2Material.ambientColor = glm::vec3(0.01f, 0.01f, 0.01f);
	glass2Material.ambientStrength = 0.01f;
	glass2Material.diffuseColor = glm::vec3(0.01f, 0.01f, 0.01f);
	glass2Material.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	glass2Material.shininess = 10.0;
	glass2Material.tag = "Glass2";

	m_objectMaterials.push_back(glass2Material);

	OBJECT_MATERIAL glossyMaterial;

	glossyMaterial.ambientColor = glm::vec3(0.01f, 0.01f, 0.01f);
	glossyMaterial.ambientStrength = 0.3f;
	glossyMaterial.diffuseColor = glm::vec3(0.01f, 0.01f, 0.01f);
	glossyMaterial.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	glossyMaterial.shininess = 80.0;
	glossyMaterial.tag = "Glossy";

	m_objectMaterials.push_back(glossyMaterial);

	OBJECT_MATERIAL glossy2Material;

	glossy2Material.ambientColor = glm::vec3(0.01f, 0.01f, 0.01f);
	glossy2Material.ambientStrength = 0.5f;
	glossy2Material.diffuseColor = glm::vec3(0.01f, 0.01f, 0.01f);
	glossy2Material.specularColor = glm::vec3(0.08f, 0.08f, 0.08f);
	glossy2Material.shininess = 90.0;
	glossy2Material.tag = "Glossy2";

	m_objectMaterials.push_back(glossy2Material);

	OBJECT_MATERIAL superGlossyMaterial;

	superGlossyMaterial.ambientColor = glm::vec3(0.0f, 0.0f, 0.0f);
	superGlossyMaterial.ambientStrength = 0.5f;
	superGlossyMaterial.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
	superGlossyMaterial.specularColor = glm::vec3(0.15f, 0.15f, 0.15f);
	superGlossyMaterial.shininess = 90.0;
	superGlossyMaterial.tag = "Super Glossy";

	m_objectMaterials.push_back(superGlossyMaterial);

	OBJECT_MATERIAL noGlossMaterial;

	noGlossMaterial.ambientColor = glm::vec3(0.0f, 0.0f, 0.0f);
	noGlossMaterial.ambientStrength = 0.0f;
	noGlossMaterial.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
	noGlossMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	noGlossMaterial.shininess = 0.0;
	noGlossMaterial.tag = "No Gloss";

	m_objectMaterials.push_back(noGlossMaterial);
}

/***********************************************************
*	SetupSceneLights()
*
*	This method is used to add and configure the various
*	light sources that will add more realism to the 3D scene
************************************************************/
void SceneManager::SetupSceneLights()
{


	m_pShaderManager->setVec3Value("lightSources[0].position", 12.0f, 15.0f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	m_pShaderManager->setVec3Value("lightSources[1].position", 6.0f, 5.0f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.5f);

	m_pShaderManager->setVec3Value("lightSources[2].position", 0.0f, 15.0f, 20.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.05f);

	m_pShaderManager->setVec3Value("lightSources[3].position", 1.0f, 4.0f, -5.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 6.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.8f);

	m_pShaderManager->setBoolValue("bUseLighting", true);
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the textures for the 3D scene
	LoadSceneTextures();

	DefineObjectMaterials();
	
	SetupSceneLights();
	
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	/**********                 Wooden Table                 **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14.0f, 1.0f, 7.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, -0.499f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Wood");
	SetShaderMaterial("Wood");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	/**********               Base of turntable            **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14.0f, 0.5f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	SetShaderTexture("Black");
	SetShaderMaterial("Glossy2");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/**********              Front left foot              **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.7f, 0.4f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, -0.48f, -1.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	SetShaderTexture("Black");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********              Back left foot                **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.7f, 0.4f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, -0.48f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	SetShaderTexture("Black");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********              Front right foot              **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.7f, 0.4f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.6f, -0.48f, -1.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	SetShaderTexture("Black");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********              Back right foot               **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.7f, 0.4f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.6f, -0.48f, -8.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.1, 0.1, 0.1, 1);
	SetShaderTexture("Black");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********                 Platter                    **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.7f, 0.2f, 4.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.9f, 0.5f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.6, 0.75, 0.7, 0.4);
	SetShaderTexture("Glass");
	SetShaderMaterial("Glass");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/
	/**********             Motor under platter            **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.4f, 0.2f, 1.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.9f, 0.3f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.2, 0.2, 0.2, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********                Spindle base                **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.1f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.9f, 0.65f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 0, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********                  Spindle                   **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.3f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-1.9f, 0.65f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.7, 0.7, 0.7, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********            Motor speed button [base]       **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.11f, 0.02f, 0.11f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.4f, 0.25f, -0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.359, 0.359, 0.359, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********              Motor speed button            **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 0.05f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.4f, 0.25f, -0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.3, 0.3, 0.3, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********                 Tone arm                   **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 7.6f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.8f, 1.0f, -8.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.3, 0.3, 0.3, 1);
	
	SetShaderMaterial("Glossy2");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********             Tone arm base [lower]          **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.8f, 0.1f, 0.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.8f, 0.3f, -6.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.3, 0.3, 0.3, 1);
	SetShaderMaterial("Glossy2");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********              Tone arm base [upper]         **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.45f, 0.15f, 0.7f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.8f, 0.4f, -6.9f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.35, 0.35, 0.35, 1);
	SetShaderMaterial("Glossy2");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********            Tone arm support [rear]         **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 0.4f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.8f, 0.3f, -7.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.4, 0.4, 1);
	SetShaderMaterial("Glossy2");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********             Tone arm support [near]        **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 0.6f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.8f, 0.3f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.4, 0.4, 1);
	SetShaderMaterial("Glossy2");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********               Tone weight [near]           **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.7f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.8f, 1.0f, -7.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.4, 0.4, 1);
	SetShaderMaterial("Glossy");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********               Tone weight [rear]           **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 0.45f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.8f, 0.9f, -8.35f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.4, 0.4, 1);
	SetShaderMaterial("Glossy");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********           Tone arm rest [horizontal]       **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 2.0f, 0.035);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.8f, 0.45f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.35, 0.35, 0.35, 1);
	SetShaderMaterial("Glossy2");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********           Tone arm rest [vertical]         **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 0.45f, 0.035);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.8f, 0.417f, -4.52f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.35, 0.35, 0.35, 1);
	SetShaderMaterial("Glossy2");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********           Cuing lever [horizontal]         **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.06f, 0.38f, 0.06);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.5f, 0.45f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.385, 0.385, 0.385, 1);
	SetShaderMaterial("Glossy");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********             Cuing lever [angle]            **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.02f, 0.7f, 0.02);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -35.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.35f, 0.45f, -6.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.4, 0.4, 1);
	SetShaderMaterial("Glossy");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********             Cuing lever [handle]            **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.06f, 0.2f, 0.06);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 60.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -35.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.67f, 0.68f, -6.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.385, 0.385, 0.385, 1);
	SetShaderMaterial("Glossy");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********                 Cartridge                  **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.6f, 0.15f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.8f, 1.0f, -1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.3, 0.3, 0.3, 1);
	SetShaderMaterial("Glossy");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/**********              Cartridge handle              **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 0.9f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -40.0f;
	ZrotationDegrees = -90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.66f, 1.1f, -1.1f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.4, 0.4, 1);
	SetShaderMaterial("Glossy");
	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/
	/**********               Handle rivet [left]          **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.04f, 0.04f, 0.04f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.72f, 1.145f, -1.05f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.9, 0.9, 0.9, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/
	/**********              Handle rivet [right]          **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.04f, 0.04f, 0.04f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.92f, 1.13f, -0.89f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.9, 0.9, 0.9, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	/****************************************************************/
	/**********                 Styulis                    **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 0.4f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = -83.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -22.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.7f, 0.88f, -0.6f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.4, 0.4, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/
	/**********                 Needle                     **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.03f, 0.1f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(3.71f, 0.87f, -0.64f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.4, 0.4, 0.4, 1);

	// draw the mesh with transformation values
	m_basicMeshes->DrawConeMesh();
	/****************************************************************/
	/**********                Lid cover [rear]            **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(14.0f, 0.1f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 5.5f, -11.72f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 0.55);
	SetShaderTexture("Glass2");
	SetShaderMaterial("Glass2");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/**********                Lid [left panel]            **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 1.7f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.0f, 5.5f, -10.92f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 0.55);
	SetShaderTexture("Glass2");
	SetShaderMaterial("Glass2");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/**********                Lid [right panel]           **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 1.7f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.0f, 5.5f, -10.92f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 0.55);
	SetShaderTexture("Glass2");
	SetShaderMaterial("Glass2");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/**********                Lid [top panel]             **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 1.7f, 14.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 10.5f, -10.92f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 0.55);
	SetShaderTexture("Glass2");
	SetShaderMaterial("Glass2");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/**********                Lid [bottom panel]          **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 1.7f, 14.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.49f, -10.92f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 0.55);
	SetShaderTexture("Glass2");
	SetShaderMaterial("Glass2");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/**********          Left lid handle [horizontal]      **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.8f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.37f, 0.34f, -10.01f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Chrome");
	SetShaderMaterial("Glossy");
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********           Left lid handle [Vertical]       **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.8f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-6.1f, 0.34f, -10.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Chrome");
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********          Right lid handle [horizontal]     **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.8f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.08f, 0.34f, -10.01f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Chrome");
	SetShaderMaterial("Glossy");
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********           Right lid handle [Vertical]      **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.08f, 0.8f, 0.08f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(6.0f, 0.34f, -10.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Chrome");
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/****************************************************************/
	/****************************************************************/
	/**********                                            **********/
	/**********            RUN THE JEWLES ALBUM            **********/
	/**********                                            **********/
	/****************************************************************/
	/****************************************************************/
	/**********                 Album shape                **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(9.5f, 9.5f, 0.2f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = -10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.1f, 4.94f, -10.60f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0, 0, 0, 1);
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
	/**********                Front Album Cover           **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.7f, 3.4f, 4.7f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 80.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.1f, 4.94f, -10.47f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Front Cover");
	SetShaderMaterial("Glossy");
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	/**********                Rear Album Cover            **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(4.7f, 3.4f, 4.7f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 80.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-0.1f, 4.94f, -10.71f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Back Cover");
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	/****************************************************************/
	/****************************************************************/
	/**********                                            **********/
	/**********                    LAMP                    **********/
	/**********                                            **********/
	/****************************************************************/
	/****************************************************************/
	/**********                 Lamp base                  **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(2.0f, 0.7f, 2.0f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.5f, -0.4f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Brass");
	SetShaderMaterial("Glossy");
	m_basicMeshes->DrawHalfSphereMesh();
	/****************************************************************/
	/**********               Lamp stand [lower]           **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 4.6f, 0.15f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -20.0f;
	ZrotationDegrees = 15.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-10.5f, 0.0f, -9.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Brass");
	SetShaderMaterial("Glossy");
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********              Lamp joint [lower]            **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.4f, 0.2f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -35.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.6f, 4.7f, -9.39f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Brass");
	SetShaderMaterial("Glossy");
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
	/**********              Lamp stand [upper]            **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.15f, 4.6f, 0.15f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 20.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -20.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-11.6f, 4.8f, -9.38f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Brass");
	SetShaderMaterial("Glossy");
	m_basicMeshes->DrawCylinderMesh();
	/****************************************************************/
	/**********                 Lamp shade                 **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 3.1f, 1.5f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 10.0f;
	YrotationDegrees = -19.0f;
	ZrotationDegrees = 48.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-7.9f, 8.2f, -7.2f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("Modern");
	SetShaderMaterial("Glossy");
	m_basicMeshes->DrawHalfSphereMesh();
	/****************************************************************/
	/**********               Lamp joint [upper]           **********/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.4f, 0.2f);
	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = -30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-9.9f, 9.2f, -7.8f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("Brass");
	SetShaderTexture("Glossy3");
	m_basicMeshes->DrawSphereMesh();
	/****************************************************************/
}
