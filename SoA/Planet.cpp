#include "stdafx.h"
#include "Planet.h"

#include <glm\gtc\type_ptr.hpp>
#include <glm\gtc\quaternion.hpp>
#include <glm\gtx\quaternion.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include "FileSystem.h"
#include "GameManager.h"
#include "InputManager.h"
#include "Inputs.h"
#include "ObjectLoader.h"
#include "OpenglManager.h"
#include "Options.h"
#include "Rendering.h"
#include "TerrainGenerator.h"
#include "TerrainPatch.h"
#include "shader.h"

ObjectLoader objectLoader;

Planet::Planet() : generator(NULL)
{
    bindex = 0;
    stormNoiseFunction = NULL;
    sunnyCloudyNoiseFunction = NULL;
    cumulusNoiseFunction = NULL;
    rotationTheta = 0;
    rotationMatrix = glm::mat4(1.0);
    invRotationMatrix = glm::inverse(rotationMatrix);
    axialZTilt = 0.4;
    baseTemperature = 100;
    baseRainfall = 100;
    minHumidity = 0.0f;
    maxHumidity = 100.0f;
    minCelsius = -20.0f;
    maxCelsius = 60.0f;
    solarX = solarY = solarZ = 0;
    scaledRadius = 0;
    gravityConstant = 1.0;
    density = 1.0;
}

Planet::~Planet()
{
    destroy();
}

void Planet::clearMeshes()
{
    TerrainBuffers *tb;
    for (int k = 0; k < 6; k++){
        for (size_t i = 0; i < drawList[k].size(); i++){
            tb = drawList[k][i];
            if (tb->vaoID != 0) glDeleteVertexArrays(1, &(tb->vaoID));
            if (tb->vboID != 0) glDeleteBuffers(1, &(tb->vboID));
            if (tb->treeVboID != 0) glDeleteBuffers(1, &(tb->treeVboID));
            if (tb->vboIndexID != 0) glDeleteBuffers(1, &(tb->vboIndexID));
            delete tb;
        }

        drawList[k].clear();
    }


    for (size_t i = 0; i < 6U; i++){
        for (size_t j = 0; j < faces[i].size(); j++){
            for (size_t k = 0; k < faces[i][j].size(); k++){
                faces[i][j][k]->NullifyBuffers();
            }
        }
    }
}

void Planet::initialize(string filePath)
{
    if (!generator) generator = new TerrainGenerator;
    if (filePath == "Worlds/(Empty Planet)/"){
        radius = 1000000;
        scaledRadius = (radius - radius%CHUNK_WIDTH) / planetScale;
        int width = scaledRadius / TerrainPatchWidth * 2;
        if (width % 2 == 0){ // must be odd
            width++;
        }
        scaledRadius = (width*TerrainPatchWidth) / 2;
        radius = (int)(scaledRadius*planetScale);

        for (int i = 0; i < 256; i++){
            for (int j = 0; j < 256; j++){
                ColorMap[i][j][0] = (int)0; //converts bgr to rgb
                ColorMap[i][j][1] = (int)255;
                ColorMap[i][j][2] = (int)0;
            }
        }

        for (int i = 0; i < 256; i++){
            for (int j = 0; j < 256; j++){
                waterColorMap[i][j][0] = (int)0; //converts bgr to rgb
                waterColorMap[i][j][1] = (int)0;
                waterColorMap[i][j][2] = (int)255;
            }
        }

        for (int i = 0; i < 256; i++){
            for (int j = 0; j < 256; j++){
                int hexcode = 0;
                generator->BiomeMap[i][j] = hexcode;
            }
        }

        atmosphere.initialize("", scaledRadius);
    }
    else{
        loadData(filePath, 0);


        atmosphere.initialize(filePath + "Sky/properties.ini", scaledRadius);
    }
    dirName = filePath;

    rotationTheta = 0;
    glm::vec3 EulerAngles(0, rotationTheta, axialZTilt);
    rotateQuaternion = glm::quat(EulerAngles);
    rotationMatrix = glm::toMat4(rotateQuaternion);

    double mrad = (double)radius / 2.0;
    volume = (4.0 / 3.0)*M_PI*mrad*mrad*mrad;
    mass = volume*density;

    facecsGridWidth = (radius * 2) / CHUNK_WIDTH;
}

void Planet::initializeTerrain(const glm::dvec3 &startPosition)
{
    int lodx, lody, lodz;

    int wx = (int)startPosition.x, wy = (int)startPosition.y, wz = (int)startPosition.z;
    int width = scaledRadius/TerrainPatchWidth*2;
    if (width%2 == 0){ // must be odd
        width++;
    }
    cout << "RADIUS " << radius << " Width " << width << endl;
    cout << "XYZ " << solarX << " " << solarY << " " << solarZ << endl;

    int centerX = 0;
    int centerY = 0 + scaledRadius;
    int centerZ = 0;
    double magnitude;

    faces[P_TOP].resize(width);
    for (size_t i = 0; i < faces[P_TOP].size(); i++){
        faces[P_TOP][i].resize(width);
        for (size_t j = 0; j < faces[P_TOP][i].size(); j++){
            faces[P_TOP][i][j] = new TerrainPatch(TerrainPatchWidth);

            lodx = 0 + (j - width/2) * TerrainPatchWidth - TerrainPatchWidth/2;
            lody = centerY;
            lodz = 0 + (i - width/2) * TerrainPatchWidth - TerrainPatchWidth/2;
            magnitude = sqrt(lodx*lodx + lody*lody + lodz*lodz);

            faces[P_TOP][i][j]->Initialize(lodx, lody, lodz, wx, wy, wz, radius, P_TOP);
            while(!(faces[P_TOP][i][j]->CreateMesh())); //load all of the quadtrees
        }
    }

    centerX = 0 - scaledRadius;
    centerY = 0;
    centerZ = 0;
    faces[P_LEFT].resize(width);
    for (size_t i = 0; i < faces[P_LEFT].size(); i++){
        faces[P_LEFT][i].resize(width);
        for (size_t j = 0; j < faces[P_LEFT][i].size(); j++){
            faces[P_LEFT][i][j] = new TerrainPatch(TerrainPatchWidth);

            lodx = centerX;
            lody = 0 + (i - width/2) * TerrainPatchWidth - TerrainPatchWidth/2;
            lodz = 0 + (j - width/2) * TerrainPatchWidth - TerrainPatchWidth/2;
            magnitude = sqrt(lodx*lodx + lody*lody + lodz*lodz);

            faces[P_LEFT][i][j]->Initialize(lodx, lody, lodz, wx, wy, wz, radius, P_LEFT);
            while(!(faces[P_LEFT][i][j]->CreateMesh())); //load all of the quadtrees
        }
    }

    centerX = 0 + scaledRadius;
    centerY = 0;
    centerZ = 0;
    faces[P_RIGHT].resize(width);
    for (size_t i = 0; i < faces[P_RIGHT].size(); i++){
        faces[P_RIGHT][i].resize(width);
        for (size_t j = 0; j < faces[P_RIGHT][i].size(); j++){
            faces[P_RIGHT][i][j] = new TerrainPatch(TerrainPatchWidth);

            lodx = centerX;
            lody = 0 + (i - width/2) * TerrainPatchWidth - TerrainPatchWidth/2;
            lodz = 0 + (j - width/2) * TerrainPatchWidth - TerrainPatchWidth/2;
            magnitude = sqrt(lodx*lodx + lody*lody + lodz*lodz);

            faces[P_RIGHT][i][j]->Initialize(lodx, lody, lodz, wx, wy, wz, radius, P_RIGHT);

            while(!(faces[P_RIGHT][i][j]->CreateMesh())); //load all of the quadtrees
        }
    }

    centerX = 0;
    centerY = 0;
    centerZ = 0 + scaledRadius;
    faces[P_FRONT].resize(width);
    for (size_t i = 0; i < faces[P_FRONT].size(); i++){
        faces[P_FRONT][i].resize(width);
        for (size_t j = 0; j < faces[P_FRONT][i].size(); j++){
            faces[P_FRONT][i][j] = new TerrainPatch(TerrainPatchWidth);

            lodx = 0 + (j - width/2) * TerrainPatchWidth - TerrainPatchWidth/2;
            lody = 0 + (i - width/2) * TerrainPatchWidth - TerrainPatchWidth/2;
            lodz = centerZ;
            magnitude = sqrt(lodx*lodx + lody*lody + lodz*lodz);


            //TopFace[i][j]->Initialize(((double)lodx/magnitude)*radius, ((double)lody/magnitude)*radius, ((double)lodz/magnitude)*radius, centerX, centerY, centerZ);
            faces[P_FRONT][i][j]->Initialize(lodx, lody, lodz, wx, wy, wz, radius, P_FRONT);

            while(!(faces[P_FRONT][i][j]->CreateMesh())); //load all of the quadtrees
        }
    }

    centerX = 0;
    centerY = 0;
    centerZ = 0 - scaledRadius;
    faces[P_BACK].resize(width);
    for (size_t i = 0; i < faces[P_BACK].size(); i++){
        faces[P_BACK][i].resize(width);
        for (size_t j = 0; j < faces[P_BACK][i].size(); j++){
            faces[P_BACK][i][j] = new TerrainPatch(TerrainPatchWidth);

            lodx = 0 + (j - width/2) * TerrainPatchWidth - TerrainPatchWidth/2;
            lody = 0 + (i - width/2) * TerrainPatchWidth - TerrainPatchWidth/2;
            lodz = centerZ;
            magnitude = sqrt(lodx*lodx + lody*lody + lodz*lodz);

            faces[P_BACK][i][j]->Initialize(lodx, lody, lodz, wx, wy, wz, radius, P_BACK);

            while(!(faces[P_BACK][i][j]->CreateMesh())); //load all of the quadtrees
        }
    }

    centerX = 0;
    centerY = 0 - scaledRadius;
    centerZ = 0;

    faces[P_BOTTOM].resize(width);
    for (size_t i = 0; i < faces[P_BOTTOM].size(); i++){
        printf("%d ", i);
        faces[P_BOTTOM][i].resize(width);
        for (size_t j = 0; j < faces[P_BOTTOM][i].size(); j++){
            faces[P_BOTTOM][i][j] = new TerrainPatch(TerrainPatchWidth);

            lodx = 0 + (j - width/2) * TerrainPatchWidth - TerrainPatchWidth/2;
            lody = centerY;
            lodz = 0 + (i - width/2) * TerrainPatchWidth - TerrainPatchWidth/2;
            magnitude = sqrt(lodx*lodx + lody*lody + lodz*lodz);

            faces[P_BOTTOM][i][j]->Initialize(lodx, lody, lodz, wx, wy, wz, radius, P_BOTTOM);

            while(!(faces[P_BOTTOM][i][j]->CreateMesh())); //load all of the quadtrees
        }
    }
}

void Planet::loadData(string filePath, bool ignoreBiomes)
{    

    loadProperties(filePath + "properties.ini");
    saveProperties(filePath + "properties.ini"); //save em to update them

    currTerrainGenerator = generator;
    GameManager::planet = this;
    if (!ignoreBiomes){
        if (fileManager.loadNoiseFunctions((filePath + "Noise/TerrainNoise.SOANOISE").c_str(), 0, this)) {
            pError("Failed to load terrain noise functions!");
            glToGame.enqueue(Message(GL_M_QUIT, nullptr));
            exit(75);
        }
        if (fileManager.loadNoiseFunctions((filePath + "Noise/MandatoryNoise.SOANOISE").c_str(), 1, this)) {
            pError("Failed to load mandatory noise functions!");
            glToGame.enqueue(Message(GL_M_QUIT, nullptr));
            exit(76);
        }
    }
    
    if (fileManager.loadFloraData(this, filePath)) {
        pError("Failed to load flora data!");
        glToGame.enqueue(Message(GL_M_QUIT, nullptr));
        exit(77);
    }
    if (fileManager.loadAllTreeData(this, filePath)) {
        pError("Failed to load tree data");
        glToGame.enqueue(Message(GL_M_QUIT, nullptr));
        exit(78);
    }
    if (!ignoreBiomes){
        if (fileManager.loadBiomeData(this, filePath)) {
            pError("Failed to load biome data!");
            glToGame.enqueue(Message(GL_M_QUIT, nullptr));
            exit(79);
        }
    }

    //Uncomment to save all trees after loading, for file conversions
     /*for (size_t i = 0; i < treeTypeVec.size(); i++){
        fileManager.SaveTreeData(treeTypeVec[i]);
    }*/

    fileManager.saveBiomeData(this, filePath);
    
    loadPNG(sunColorMapTexture, (filePath + "/Sky/sunColor.png").c_str(), PNGLoadInfo(textureSamplers, 2)); 

    GLubyte buffer[256][256][3];
    if (!ignoreBiomes){
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, GameManager::planet->biomeMapTexture.ID);
        
        glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, buffer);

        //convert rgb values into a hex int
        for (int i = 0; i < 256; i++){
            for (int j = 0; j < 256; j++){
                int hexcode = 0;
                hexcode |= (((int)buffer[i][j][2]) << 16); //converts bgr to rgb
                hexcode |= (((int)buffer[i][j][1]) << 8);
                hexcode |= ((int)buffer[i][j][0]);
                generator->BiomeMap[i][j] = hexcode;
            }
        }
    }
    //color map!
    glBindTexture(GL_TEXTURE_2D, GameManager::planet->colorMapTexture.ID);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, buffer);
    for (int i = 0; i < 256; i++){
        for (int j = 0; j < 256; j++){
            ColorMap[i][j][0] = (int)buffer[i][j][2]; //converts bgr to rgb
            ColorMap[i][j][1] = (int)buffer[i][j][1];
            ColorMap[i][j][2] = (int)buffer[i][j][0];
        }
    }

    glBindTexture(GL_TEXTURE_2D, GameManager::planet->waterColorMapTexture.ID);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, buffer);
    for (int i = 0; i < 256; i++){
        for (int j = 0; j < 256; j++){
            waterColorMap[i][j][0] = (int)buffer[i][j][2]; //converts bgr to rgb
            waterColorMap[i][j][1] = (int)buffer[i][j][1];
            waterColorMap[i][j][2] = (int)buffer[i][j][0];
        }
    }
    
}

void Planet::saveProperties(string filePath)
{
    vector <vector <IniValue> > iniValues;
    vector <string> iniSections;

    iniSections.push_back("");
    iniValues.push_back(vector<IniValue>());
    iniSections.push_back("Properties");
    iniValues.push_back(vector<IniValue>());
    iniValues.back().push_back(IniValue("x", 0));
    iniValues.back().push_back(IniValue("y", 0));
    iniValues.back().push_back(IniValue("z", 0));
    iniValues.back().push_back(IniValue("radius", radius));
    iniValues.back().push_back(IniValue("gravityConstant", gravityConstant));
    iniValues.back().push_back(IniValue("density", density));
    iniValues.back().push_back(IniValue("axialTilt", axialZTilt));
    iniSections.push_back("Climate");
    iniValues.push_back(vector<IniValue>());
    iniValues.back().push_back(IniValue("baseTemperature", baseTemperature));
    iniValues.back().push_back(IniValue("minCelsius", minCelsius));
    iniValues.back().push_back(IniValue("maxCelsius", maxCelsius));
    iniValues.back().push_back(IniValue("baseHumidity", baseRainfall));
    iniValues.back().push_back(IniValue("minHumidity", minHumidity));
    iniValues.back().push_back(IniValue("maxHumidity", maxHumidity));
    
    fileManager.saveIniFile(filePath, iniValues, iniSections);
}

void Planet::loadProperties(string filePath)
{
    vector < vector <IniValue> > iniValues;
    vector <string> iniSections;
    fileManager.loadIniFile(filePath, iniValues, iniSections);

    int iVal;
    IniValue *iniVal;
    for (size_t i = 0; i < iniSections.size(); i++){
        for (size_t j = 0; j < iniValues[i].size(); j++){
            iniVal = &(iniValues[i][j]);

            iVal = fileManager.getIniVal(iniVal->key);
            switch (iVal){
            case INI_X:
                solarX = iniVal->getInt();
                break;
            case INI_Y:
                solarY = iniVal->getInt();
                break;
            case INI_Z:
                solarZ = iniVal->getInt();
                break;
            case INI_DENSITY:
                density = iniVal->getFloat();
                break;
            case INI_GRAVITYCONSTANT:
                gravityConstant = iniVal->getFloat();
                break;
            case INI_BASETEMPERATURE:
                baseTemperature = iniVal->getInt();
                break;
            case INI_BASEHUMIDITY:
                baseRainfall = iniVal->getInt();
                break;
            case INI_AXIALTILT:
                axialZTilt = iniVal->getFloat();
                break;
            case INI_MINCELSIUS:
                minCelsius = iniVal->getFloat();
                break;
            case INI_MAXCELSIUS:
                maxCelsius = iniVal->getFloat();
                break;
            case INI_MINHUMIDITY:
                minHumidity = iniVal->getFloat();
                break;
            case INI_MAXHUMIDITY:
                maxHumidity = iniVal->getFloat();
                break;
            case INI_RADIUS:
                radius = iniVal->getFloat();
                scaledRadius = (radius - radius%CHUNK_WIDTH) / planetScale;
                int width = scaledRadius / TerrainPatchWidth * 2;
                if (width % 2 == 0){ // must be odd
                    width++;
                }
                scaledRadius = (width*TerrainPatchWidth) / 2;
                radius = (int)(scaledRadius*planetScale);
                break;
            }
        }
    }
}

void Planet::saveData()
{
    fileManager.saveBiomeData(this, "World/");
    fileManager.saveAllBiomes(this);
}

void Planet::rotationUpdate()
{
    float rotationInput = GameManager::inputManager->getAxis(INPUT_PLANET_ROTATION);
    if (std::abs(rotationInput) > 0) {
        rotationTheta += rotationInput * 0.01 * physSpeedFactor;
    }else{
        rotationTheta += 0.00002*physSpeedFactor;//0.00001
    }
    if (rotationTheta > 3.14){
        rotationTheta = -3.14;
    }else if (rotationTheta < -3.14){
        rotationTheta = 3.14;
    }

    glm::vec3 EulerAngles(0, rotationTheta, axialZTilt);
    rotateQuaternion = glm::quat(EulerAngles);
    gameToGl.enqueue(Message(GL_M_UPDATEPLANET, (void *)(new PlanetUpdateMessage(glm::toMat4(rotateQuaternion)))));
    //rotationMatrix = glm::toMat4(rotateQuaternion);
    //invRotationMatrix = glm::inverse(rotationMatrix);
}

void Planet::draw(float theta, const glm::mat4 &VP, const glm::mat4 &V, glm::vec3 lightPos, glm::dvec3 &PlayerPos, GLfloat sunVal, float fadeDistance, bool connectedToPlanet)
{    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, terrainTexture.ID);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, waterNormalTexture.ID);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, biomeMapTexture.ID);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, waterColorMapTexture.ID);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, waterNoiseTexture.ID);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, sunColorMapTexture.ID);

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, colorMapTexture.ID);

    closestTerrainPatchDistance = 999999999999.0;

    glm::dvec3 rotPlayerPos;
    glm::dvec3 nPlayerPos;
    glm::vec3 rotLightPos;
    rotLightPos = glm::vec3((GameManager::planet->invRotationMatrix) * glm::vec4(lightPos, 1.0));
    bool onPlanet;
    if (connectedToPlanet){
        rotPlayerPos = PlayerPos;
        nPlayerPos = PlayerPos;
        onPlanet = 1;
    }else{
        rotPlayerPos = glm::dvec3((glm::dmat4(GameManager::planet->invRotationMatrix)) * glm::dvec4(PlayerPos, 1.0));
        nPlayerPos = PlayerPos;
        onPlanet = 0;
    }

    if (glm::length(PlayerPos) > atmosphere.radius){
        drawGroundFromSpace(theta, VP, rotLightPos, nPlayerPos, rotPlayerPos, onPlanet);
    }else{
        drawGroundFromAtmosphere(theta, VP, rotLightPos, nPlayerPos, rotPlayerPos, fadeDistance, onPlanet);
    }
}

void Planet::drawTrees(glm::mat4 &VP, const glm::dvec3 &PlayerPos, GLfloat sunVal)
{
    glm::vec3 worldUp = glm::vec3(glm::normalize(PlayerPos));
//    glDisable(GL_CULL_FACE);
    treeShader.Bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, treeTrunkTexture1.ID);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalLeavesTexture.ID);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, pineLeavesTexture.ID);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mushroomCapTexture.ID);

    glUniform3f(treeShader.worldUpID, worldUp.x, worldUp.y, worldUp.z);
    glUniform1f(treeShader.fadeDistanceID, (GLfloat)((csGridWidth/2) * CHUNK_WIDTH)*invPlanetScale);
    glUniform1f(treeShader.sunValID, sunVal);

    for (size_t f = 0; f < 6; f++){
        for (size_t i = 0; i < drawList[f].size(); i++){
            if (drawList[f][i]->treeIndexSize) TerrainPatch::DrawTrees(drawList[f][i], PlayerPos, VP);
        }
    }

    
    treeShader.UnBind();

}

void Planet::drawGroundFromAtmosphere(float theta, const glm::mat4 &VP, glm::vec3 lightPos, const glm::dvec3 &PlayerPos, const glm::dvec3 &rotPlayerPos, float fadeDistance, bool onPlanet)
{
    atmosphereToGroundShader.Bind();

    float m_Kr4PI = atmosphere.m_Kr*4.0f*M_PI;
    float m_Km4PI = atmosphere.m_Km*4.0f*M_PI;
    float m_fScale = 1.0 / (atmosphere.radius - scaledRadius);

    glUniform3f(atmosphereToGroundShader.cameraPosID, (float)rotPlayerPos.x, (float)rotPlayerPos.y, (float)rotPlayerPos.z);

    glUniform3f(atmosphereToGroundShader.lightPosID, lightPos.x, lightPos.y, lightPos.z);
//    glUniform1f(atmosphereToGroundShader.gammaID, gamma);
    glUniform3f(atmosphereToGroundShader.invWavelengthID, 1/atmosphere.m_fWavelength4[0], 1/atmosphere.m_fWavelength4[1], 1/atmosphere.m_fWavelength4[2]);
    float height = glm::length(rotPlayerPos);
    if (height < scaledRadius+1) height = scaledRadius+1;
    glUniform1f(atmosphereToGroundShader.cameraHeightID, height);

    glUniform1f(atmosphereToGroundShader.dtID, bdt);

    glUniform1f(atmosphereToGroundShader.specularExponentID, graphicsOptions.specularExponent);
    glUniform1f(atmosphereToGroundShader.specularIntensityID, graphicsOptions.specularIntensity);

    glUniform1f(atmosphereToGroundShader.freezeTempID, FREEZETEMP/255.0f);
    
//    glUniform1f(atmosphereToGroundShader.cameraHeight2ID, glm::length(ppos)*glm::length(ppos));
//    glUniform1f(atmosphereToGroundShader.outerRadiusID, atmosphere.radius);
//    glUniform1f(atmosphereToGroundShader.outerRadius2ID, outerRad*outerRad);
    glUniform1f(atmosphereToGroundShader.innerRadiusID, scaledRadius);
//    glUniform1f(atmosphereToGroundShader.innerRadius2ID, innerRad*innerRad);
    glUniform1f(atmosphereToGroundShader.KrESunID, atmosphere.m_Kr*atmosphere.m_ESun);
    glUniform1f(atmosphereToGroundShader.KmESunID, atmosphere.m_Km*atmosphere.m_ESun);
    glUniform1f(atmosphereToGroundShader.Kr4PIID, m_Kr4PI);
    glUniform1f(atmosphereToGroundShader.Km4PIID, m_Km4PI);
    glUniform1f(atmosphereToGroundShader.scaleID, m_fScale);
    glUniform1f(atmosphereToGroundShader.scaleDepthID, atmosphere.m_fRayleighScaleDepth);
    glUniform1f(atmosphereToGroundShader.scaleOverScaleDepthID, m_fScale/atmosphere.m_fRayleighScaleDepth);
    glUniform1f(atmosphereToGroundShader.fSamplesID, atmosphere.fSamples);
    glUniform1i(atmosphereToGroundShader.nSamplesID, atmosphere.nSamples);

    glUniform1f(atmosphereToGroundShader.secColorMultID, graphicsOptions.secColorMult);

    glUniform1f(atmosphereToGroundShader.fadeDistanceID, fadeDistance);
//    glUniform1f(atmosphereToGroundShader.gID, atmosphere.m_g);
//    glUniform1f(atmosphereToGroundShader.g2ID, atmosphere.m_g*atmosphere.m_g);
//    debugVarc;

    for (size_t i = 0; i < drawList[0].size(); i++){
        TerrainPatch::Draw(drawList[0][i], PlayerPos, rotPlayerPos, VP, atmosphereToGroundShader.mvpID, atmosphereToGroundShader.worldOffsetID, onPlanet);
    }
    for (size_t i = 0; i < drawList[2].size(); i++){
        TerrainPatch::Draw(drawList[2][i], PlayerPos, rotPlayerPos, VP, atmosphereToGroundShader.mvpID, atmosphereToGroundShader.worldOffsetID, onPlanet);
    }
    for (size_t i = 0; i < drawList[4].size(); i++){
        TerrainPatch::Draw(drawList[4][i], PlayerPos, rotPlayerPos, VP, atmosphereToGroundShader.mvpID, atmosphereToGroundShader.worldOffsetID, onPlanet);
    }
    glFrontFace(GL_CW);
    for (size_t i = 0; i < drawList[5].size(); i++){
        TerrainPatch::Draw(drawList[5][i], PlayerPos, rotPlayerPos, VP, atmosphereToGroundShader.mvpID, atmosphereToGroundShader.worldOffsetID, onPlanet);
    }
    for (size_t i = 0; i < drawList[1].size(); i++){
        TerrainPatch::Draw(drawList[1][i], PlayerPos, rotPlayerPos, VP, atmosphereToGroundShader.mvpID, atmosphereToGroundShader.worldOffsetID, onPlanet);
    }
    for (size_t i = 0; i < drawList[3].size(); i++){
        TerrainPatch::Draw(drawList[3][i], PlayerPos, rotPlayerPos, VP, atmosphereToGroundShader.mvpID, atmosphereToGroundShader.worldOffsetID, onPlanet);
    }
    glFrontFace(GL_CCW);

    /*for (int i = 0; i < faces[P_TOP].size(); i++){
        for (int j = 0; j < faces[P_TOP][0].size(); j++){
            faces[P_TOP][i][j]->Draw(PlayerPos, rotPlayerPos, VP, atmosphereToGroundShader.mvpID, atmosphereToGroundShader.worldOffsetID, onPlanet);
        }
    }
    for (int i = 0; i < faces[P_RIGHT].size(); i++){
        for (int j = 0; j < faces[P_RIGHT][0].size(); j++){
            faces[P_RIGHT][i][j]->Draw(PlayerPos, rotPlayerPos, VP, atmosphereToGroundShader.mvpID, atmosphereToGroundShader.worldOffsetID, onPlanet);
        }
    }
    for (int i = 0; i < faces[P_BACK].size(); i++){
        for (int j = 0; j < faces[P_BACK][0].size(); j++){
            faces[P_BACK][i][j]->Draw(PlayerPos, rotPlayerPos, VP, atmosphereToGroundShader.mvpID, atmosphereToGroundShader.worldOffsetID, onPlanet);
        }
    }
    glFrontFace(GL_CW);
    for (int i = 0; i < faces[P_BOTTOM].size(); i++){
        for (int j = 0; j < faces[P_BOTTOM][0].size(); j++){
            faces[P_BOTTOM][i][j]->Draw(PlayerPos, rotPlayerPos, VP, atmosphereToGroundShader.mvpID, atmosphereToGroundShader.worldOffsetID, onPlanet);
        }
    }
    for (int i = 0; i < faces[P_LEFT].size(); i++){
        for (int j = 0; j < faces[P_LEFT][0].size(); j++){
            faces[P_LEFT][i][j]->Draw(PlayerPos, rotPlayerPos, VP, atmosphereToGroundShader.mvpID, atmosphereToGroundShader.worldOffsetID, onPlanet);
        }
    }
    for (int i = 0; i < faces[P_FRONT].size(); i++){
        for (int j = 0; j < faces[P_FRONT][0].size(); j++){
            faces[P_FRONT][i][j]->Draw(PlayerPos, rotPlayerPos, VP, atmosphereToGroundShader.mvpID, atmosphereToGroundShader.worldOffsetID, onPlanet);
        }
    }
    glFrontFace(GL_CCW);*/

    atmosphereToGroundShader.UnBind();
}

void Planet::drawGroundFromSpace(float theta, const glm::mat4 &VP, glm::vec3 lightPos, const glm::dvec3 &PlayerPos, const glm::dvec3 &rotPlayerPos, bool onPlanet)
{
    spaceToGroundShader.Bind();

    float m_Kr4PI = atmosphere.m_Kr*4.0f*M_PI;
    float m_Km4PI = atmosphere.m_Km*4.0f*M_PI;
    float m_fScale = 1 / (atmosphere.radius - scaledRadius);

    glUniform3f(spaceToGroundShader.cameraPosID, (float)rotPlayerPos.x, (float)rotPlayerPos.y, (float)rotPlayerPos.z);

    glUniform3f(spaceToGroundShader.lightPosID, lightPos.x, lightPos.y, lightPos.z);
//    glUniform1f(spaceToGroundShader.gammaID, gamma);
    glUniform3f(spaceToGroundShader.invWavelengthID, 1/atmosphere.m_fWavelength4[0], 1/atmosphere.m_fWavelength4[1], 1/atmosphere.m_fWavelength4[2]);
//glUniform1f(spaceToGroundShader.cameraHeightID, glm::length(PlayerPos));

    glUniform1f(spaceToGroundShader.specularExponentID, graphicsOptions.specularExponent);
    glUniform1f(spaceToGroundShader.specularIntensityID, graphicsOptions.specularIntensity);
    
    glUniform1f(spaceToGroundShader.freezeTempID, FREEZETEMP/255.0f);

    glUniform1f(spaceToGroundShader.cameraHeight2ID, glm::length(PlayerPos)*glm::length(PlayerPos));
    glUniform1f(spaceToGroundShader.outerRadiusID, atmosphere.radius);
    glUniform1f(spaceToGroundShader.outerRadius2ID, atmosphere.radius*atmosphere.radius);
    glUniform1f(spaceToGroundShader.innerRadiusID, scaledRadius);
//    glUniform1f(spaceToGroundShader.innerRadius2ID, innerRad*innerRad);
    glUniform1f(spaceToGroundShader.KrESunID, atmosphere.m_Kr*atmosphere.m_ESun);
    glUniform1f(spaceToGroundShader.KmESunID, atmosphere.m_Km*atmosphere.m_ESun);
    glUniform1f(spaceToGroundShader.Kr4PIID, m_Kr4PI);
    glUniform1f(spaceToGroundShader.Km4PIID, m_Km4PI);
    glUniform1f(spaceToGroundShader.scaleID, m_fScale);
    glUniform1f(spaceToGroundShader.scaleDepthID, atmosphere.m_fRayleighScaleDepth);
    glUniform1f(spaceToGroundShader.scaleOverScaleDepthID, m_fScale/atmosphere.m_fRayleighScaleDepth);
    glUniform1f(spaceToGroundShader.fSamplesID, atmosphere.fSamples);
    glUniform1f(spaceToGroundShader.drawModeID, planetDrawMode);
    glUniform1i(spaceToGroundShader.nSamplesID, atmosphere.nSamples);

    glUniform1f(spaceToGroundShader.dtID, bdt);
//    glUniform1f(spaceToGroundShader.gID, atmosphere.m_g);
//    glUniform1f(spaceToGroundShader.g2ID, atmosphere.m_g*atmosphere.m_g);

    glUniform1f(spaceToGroundShader.secColorMultID, graphicsOptions.secColorMult);

//    glDisable(GL_CULL_FACE);
    
//    glEnable(GL_CULL_FACE);

    for (size_t i = 0; i < drawList[0].size(); i++){
        TerrainPatch::Draw(drawList[0][i], PlayerPos, rotPlayerPos, VP, spaceToGroundShader.mvpID, spaceToGroundShader.worldOffsetID, onPlanet);
    }
    for (size_t i = 0; i < drawList[2].size(); i++){
        TerrainPatch::Draw(drawList[2][i], PlayerPos, rotPlayerPos, VP, spaceToGroundShader.mvpID, spaceToGroundShader.worldOffsetID, onPlanet);
    }
    for (size_t i = 0; i < drawList[4].size(); i++){
        TerrainPatch::Draw(drawList[4][i], PlayerPos, rotPlayerPos, VP, spaceToGroundShader.mvpID, spaceToGroundShader.worldOffsetID, onPlanet);
    }
    glFrontFace(GL_CW);
    for (size_t i = 0; i < drawList[5].size(); i++){
        TerrainPatch::Draw(drawList[5][i], PlayerPos, rotPlayerPos, VP, spaceToGroundShader.mvpID, spaceToGroundShader.worldOffsetID, onPlanet);
    }
    for (size_t i = 0; i < drawList[1].size(); i++){
        TerrainPatch::Draw(drawList[1][i], PlayerPos, rotPlayerPos, VP, spaceToGroundShader.mvpID, spaceToGroundShader.worldOffsetID, onPlanet);
    }
    for (size_t i = 0; i < drawList[3].size(); i++){
        TerrainPatch::Draw(drawList[3][i], PlayerPos, rotPlayerPos, VP, spaceToGroundShader.mvpID, spaceToGroundShader.worldOffsetID, onPlanet);
    }
    glFrontFace(GL_CCW);

    spaceToGroundShader.UnBind();
}

void Planet::updateLODs(glm::dvec3 &worldPosition, GLuint maxTicks)
{
    static int k = 0;

    glm::dvec3 rWorldPosition = worldPosition;
    //glm::dvec3 rWorldPosition = glm::dvec3((glm::dmat4(GameManager::planet->invRotationMatrix)) * glm::dvec4(worldPosition, 1.0));

    int mx = (int)rWorldPosition.x, my = (int)rWorldPosition.y, mz = (int)rWorldPosition.z;
    for (int n = 0; n < 6; n++){
        for (unsigned int a = 0; a < faces[n].size(); a++){
            for (unsigned int b = 0; b < faces[n][a].size(); b++){
                if (faces[n][a][b]->update(mx, my, mz)){
                    if (faces[n][a][b]->updateVecIndex == -1){
                        faces[n][a][b]->updateVecIndex = LODUpdateList.size();
                        LODUpdateList.push_back(faces[n][a][b]);
                    }
                }
            }
        }
    }

    GLuint startTicks = SDL_GetTicks();

    if (LODUpdateList.empty()) return;
    //cout << LODUpdateList.size() << " ";
//    GLuint starttime = SDL_GetTicks();
    TerrainPatch *lod;

    int wx = (0+CHUNK_WIDTH*csGridWidth/2);
    int wz = (0+CHUNK_WIDTH*csGridWidth/2);
    int size = LODUpdateList.size()-1;

    sortUpdateList(); //sorts every update, but insertion sort is fast

    for (int i = size; LODUpdateList.size(); i--)
    {
        lod = LODUpdateList.back();

        if (lod->CreateMesh()){
            LODUpdateList.pop_back();
            lod->updateVecIndex = -1;
        }
        if (SDL_GetTicks() - startTicks > maxTicks){ return; }
    }
    
    k++;
//    cout << SDL_GetTicks() - starttime << "     L\n";
}

void RecursiveFlagLOD(TerrainPatch *lod){
    lod->updateCode = 1;
    if (lod->lods[0]){
        for(int i = 0; i < 4; i++){
            RecursiveFlagLOD(lod->lods[i]);
        }
    }
}

void Planet::flagTerrainForRebuild()
{
    for (int n = 0; n < 6; n++){
        for (unsigned int a = 0; a < faces[n].size(); a++){
            for (unsigned int b = 0; b < faces[n][a].size(); b++){
                RecursiveFlagLOD(faces[n][a][b]);
                if (faces[n][a][b]->updateVecIndex == -1){
                    faces[n][a][b]->updateVecIndex = LODUpdateList.size();
                    LODUpdateList.push_back(faces[n][a][b]);
                }
                
            }
        }
    }
}

void Planet::sortUpdateList()
{
    TerrainPatch *temp;
    int j;
    for (unsigned int i = 1; i < LODUpdateList.size(); i++)
    {
        temp = LODUpdateList[i];

        for (j = i - 1; (j >= 0) && (temp->distance > LODUpdateList[j]->distance); j-- ){
            LODUpdateList[j+1] = LODUpdateList[j];
            LODUpdateList[j+1]->vecIndex = j+1;
        }

        LODUpdateList[j+1] = temp;
        LODUpdateList[j+1]->vecIndex = j+1;
    }
}

void Planet::destroy()
{
    for (size_t i = 0; i < 6; i++){
        for (size_t j = 0; j < faces[i].size(); j++){
            for (size_t k = 0; k < faces[i][j].size(); k++){
                delete faces[i][j][k];
            }
        }
        faces[i].clear();
    }
    for (size_t i = 0; i < allBiomesLookupVector.size(); i++){
        delete allBiomesLookupVector[i];
    }
    allBiomesLookupVector.clear();
    if (generator) delete generator;
    generator = NULL;
    currTerrainGenerator = NULL;

    for (size_t i = 0; i < floraNoiseFunctions.size(); i++){
        delete floraNoiseFunctions[i];
    }
    floraNoiseFunctions.clear();
    for (size_t i = 0; i < treeTypeVec.size(); i++){
        delete treeTypeVec[i];
    }
    treeTypeVec.clear();
    for (size_t i = 0; i < floraTypeVec.size(); i++){
        delete floraTypeVec[i];
    }
    floraTypeVec.clear();

    biomeMapTexture.freeTexture();
    colorMapTexture.freeTexture();
    sunColorMapTexture.freeTexture();
    waterColorMapTexture.freeTexture();
}

void Planet::clearBiomes() //MEMORY LEAKS ARE ON PURPOSE. NOT MEANT FOR FINAL GAME
{
    bindex = 0;
    allBiomesLookupVector.clear();
    mainBiomesVector.clear();
    childBiomesVector.clear();
    baseBiomesLookupMap.clear();
}

void Planet::addBaseBiome(Biome *baseBiome, int mapColor)
{
    baseBiome->vecIndex = bindex++;
    baseBiomesLookupMap.insert(make_pair(mapColor, baseBiome));
    allBiomesLookupVector.push_back(baseBiome);

    //biome = new Biome; //freed in chunkmanager destructor
    //biome->name = name;
    //biome->r = color.r;
    //biome->g = color.g;
    //biome->b = color.b;
    //biome->surfaceBlock = surfaceBlock;
    //biome->treeChance = treeChance;
    //biome->vecIndex = index++;
    //biome->possibleTrees = treetypes;
    //biome->treeProbabilities = treeprobs;
    //biome->possibleFlora = floratypes;
    //biome->floraProbabilities = floraprobs;
    //biomesLookupMap.insert(make_pair(mapColor, biome));
    //biomesLookupVector.push_back(biome);
}

void Planet::addMainBiome(Biome *mainBiome)
{
    mainBiome->vecIndex = bindex++;
    allBiomesLookupVector.push_back(mainBiome);
    mainBiomesVector.push_back(mainBiome);
}

void Planet::addChildBiome(Biome *childBiome)
{
    childBiome->vecIndex = bindex++;
    allBiomesLookupVector.push_back(childBiome);
    childBiomesVector.push_back(childBiome);
}

double Planet::getGravityAccel(double dist)
{
    if (dist < radius) dist = radius;
    dist = dist*0.5; //convert to meters

    return (mass*M_G/(dist*dist)) * 0.01666 * 0.01666 * 2.0; //divide by 60 twice to account for frames
}

double Planet::getAirFrictionForce(double dist, double velocity)
{
    if (dist > atmosphere.radius) return 0.0; //no friction in space duh
    velocity *= 0.5; //reduce to m/s
    return 0.25*getAirDensity(dist)*velocity*velocity; //assuming drag of 0.5
}


//earths density is 1.2 kg/m3
//barometric formula
double Planet::getAirDensity(double dist)
{
    double maxDensity = 1.2;
    double mult;
    if (dist <= radius) return maxDensity;
    if (dist >= atmosphere.radius) return 0;
    mult = (dist-radius)/(atmosphere.radius-radius);
    return (pow(0.8, mult*20.0)-0.0115) * maxDensity;
}

Atmosphere::Atmosphere()
{
    m_Kr = 0.0025f;        // Rayleigh scattering constant
    m_Km = 0.0020f;        // Mie scattering constant
    m_ESun = 25.0f;        // Sun brightness constant
    m_g = -0.990f;        // The Mie phase asymmetry factor
    m_fExposure = 2.0f;
    m_fRayleighScaleDepth = 0.25f; //0.25

    m_fWavelength[0] = 0.650f;        // 650 nm for red
    m_fWavelength[1] = 0.570f;        // 570 nm for green
    m_fWavelength[2] = 0.475f;        // 475 nm for blue
    m_fWavelength4[0] = powf(m_fWavelength[0], 4.0f);
    m_fWavelength4[1] = powf(m_fWavelength[1], 4.0f);
    m_fWavelength4[2] = powf(m_fWavelength[2], 4.0f);

    nSamples = 3;
    fSamples = (float)nSamples;
}

void Atmosphere::initialize(string filePath, float PlanetRadius)
{
    if (filePath.empty()){
        m_Kr = 0.0025;
        m_Km = 0.0020;
        m_ESun = 25.0;
        m_g = -0.990;
        m_fExposure = 2.0;
        m_fWavelength[0] = 0.650;
        m_fWavelength[1] = 0.570;
        m_fWavelength[2] = 0.475;
        nSamples = 3;

        fSamples = (float)nSamples;
        m_fWavelength4[0] = powf(m_fWavelength[0], 4.0f);
        m_fWavelength4[1] = powf(m_fWavelength[1], 4.0f);
        m_fWavelength4[2] = powf(m_fWavelength[2], 4.0f);
    }
    else{
        loadProperties(filePath);
    }

    radius = PlanetRadius*(1.025);
    planetRadius = PlanetRadius;

    cout << "Loading Objects/icosphere.obj... ";
    objectLoader.load("Objects/icosphere.obj", vertices, indices);
    cout << "Done!\n";

    glGenBuffers(1, &(vboID)); // Create the buffer ID
    glBindBuffer(GL_ARRAY_BUFFER, vboID); // Bind the buffer (vertex array data)
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ColorVertex), NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &(vboIndexID));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndexID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLushort), NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(ColorVertex), &(vertices[0].position));
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLushort), &(indices[0]));
    indexSize = indices.size();

    vertices.clear();
    indices.clear();
}

void Atmosphere::loadProperties(string filePath)
{
    vector < vector <IniValue> > iniValues;
    vector <string> iniSections;
    fileManager.loadIniFile(filePath, iniValues, iniSections);

    int iVal;
    IniValue *iniVal;
    for (size_t i = 0; i < iniSections.size(); i++){
        for (size_t j = 0; j < iniValues[i].size(); j++){
            iniVal = &(iniValues[i][j]);

            iVal = fileManager.getIniVal(iniVal->key);
            switch (iVal){
                case INI_M_KR:
                    m_Kr = iniVal->getFloat();
                    break;
                case INI_M_KM:
                    m_Km = iniVal->getFloat();
                    break;
                case INI_M_ESUN:
                    m_ESun = iniVal->getFloat();
                    break;
                case INI_M_G:
                    m_g = iniVal->getFloat();
                    break;
                case INI_M_EXP:
                    m_fExposure = iniVal->getFloat();
                    break;
                case INI_WAVELENGTHR:
                    m_fWavelength[0] = iniVal->getFloat();
                    break;
                case INI_WAVELENGTHG:
                    m_fWavelength[1] = iniVal->getFloat();
                    break;
                case INI_WAVELENGTHB:
                    m_fWavelength[2] = iniVal->getFloat();
                    break;
                case INI_NSAMPLES:
                    nSamples = iniVal->getInt();
                    break;
            }
        }
    }

    fSamples = (float)nSamples;
    m_fWavelength4[0] = powf(m_fWavelength[0], 4.0f);
    m_fWavelength4[1] = powf(m_fWavelength[1], 4.0f);
    m_fWavelength4[2] = powf(m_fWavelength[2], 4.0f);
}

void Atmosphere::draw(float theta, glm::mat4 &MVP, glm::vec3 lightPos, glm::dvec3 &ppos)
{
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (glm::length(ppos) > radius){
        drawSkyFromSpace(theta, MVP, lightPos, ppos);
    }else{
        drawSkyFromAtmosphere(theta, MVP, lightPos, ppos);
    }
    
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Atmosphere::drawSkyFromAtmosphere(float theta, glm::mat4 &MVP, glm::vec3 lightPos, glm::dvec3 &ppos)
{
    atmosphereToSkyShader.Bind();

    glm::mat4 GlobalModelMatrix(1.0);
    GlobalModelMatrix[0][0] = radius;
    GlobalModelMatrix[1][1] = radius;
    GlobalModelMatrix[2][2] = radius;
    GlobalModelMatrix[3][0] = ((float)((double)-ppos.x));
    GlobalModelMatrix[3][1] = ((float)((double)-ppos.y));
    GlobalModelMatrix[3][2] = ((float)((double)-ppos.z));

    glm::quat quaternion;
    // Have to rotate it and draw it again to make a sphere
    glm::vec3 EulerAngles(M_PI, 0, 0);
    quaternion = glm::quat(EulerAngles);
    glm::mat4 RotationMatrix = glm::toMat4(quaternion); 
    glm::mat4 MVPr = MVP * GlobalModelMatrix;
    glm::mat4 M = GlobalModelMatrix;

    float m_Kr4PI = m_Kr*4.0f*M_PI;
    float m_Km4PI = m_Km*4.0f*M_PI;
    float m_fScale = 1.0 / (radius - planetRadius);

    glUniformMatrix4fv(atmosphereToSkyShader.mvpID, 1, GL_FALSE, &MVPr[0][0]);
//    glUniformMatrix4fv(atmosphereShader.mID, 1, GL_FALSE, &M[0][0]);
    glUniform3f(atmosphereToSkyShader.cameraPosID, (float)ppos.x, (float)ppos.y, (float)ppos.z);

    glUniform3f(atmosphereToSkyShader.lightPosID, lightPos.x, lightPos.y, lightPos.z);
//    glUniform1f(atmosphereShader.gammaID, gamma);
    glUniform3f(atmosphereToSkyShader.invWavelengthID, 1/m_fWavelength4[0], 1/m_fWavelength4[1], 1/m_fWavelength4[2]);
    glUniform1f(atmosphereToSkyShader.cameraHeightID, glm::length(ppos));
    
//    glUniform1f(atmosphereShader.cameraHeight2ID, glm::length(ppos)*glm::length(ppos));
    glUniform1f(atmosphereToSkyShader.outerRadiusID, radius);
//    glUniform1f(atmosphereShader.outerRadius2ID, outerRad*outerRad);
    glUniform1f(atmosphereToSkyShader.innerRadiusID, planetRadius);
//    //glUniform1f(atmosphereShader.innerRadius2ID, innerRad*innerRad);
    glUniform1f(atmosphereToSkyShader.KrESunID, m_Kr*m_ESun);
    glUniform1f(atmosphereToSkyShader.KmESunID, m_Km*m_ESun);
    glUniform1f(atmosphereToSkyShader.Kr4PIID, m_Kr4PI);
    glUniform1f(atmosphereToSkyShader.Km4PIID, m_Km4PI);
    glUniform1f(atmosphereToSkyShader.scaleID, m_fScale);
    glUniform1f(atmosphereToSkyShader.scaleDepthID, m_fRayleighScaleDepth);
    glUniform1f(atmosphereToSkyShader.scaleOverScaleDepthID, m_fScale/m_fRayleighScaleDepth);
    glUniform1f(atmosphereToSkyShader.gID, m_g);
    glUniform1f(atmosphereToSkyShader.g2ID, m_g*m_g);
    glUniform1f(atmosphereToSkyShader.fSamplesID, fSamples);
    glUniform1i(atmosphereToSkyShader.nSamplesID, nSamples);

    //if (debugVarc){
        glBindBuffer(GL_ARRAY_BUFFER, vboID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndexID);

    //}else{
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndexID2);

    //    glBindBuffer(GL_ARRAY_BUFFER, vbo2ID);
    //}
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*)0);

    //glDisable(GL_CULL_FACE);

    //if (debugVarc){
    glDrawElements(GL_TRIANGLES, indexSize, GL_UNSIGNED_SHORT, 0);
    //}else{
    //glDrawElements(GL_TRIANGLES, debugIndex, GL_UNSIGNED_SHORT, 0);
    
    //}
//    M = M * RotationMatrix;
//    MVPr = MVPr * RotationMatrix;
//    glUniformMatrix4fv(atmosphereToSkyShader.mvpID, 1, GL_FALSE, &MVPr[0][0]);
//    glUniformMatrix4fv(atmosphereShader.mID, 1, GL_FALSE, &M[0][0]);

    //glBindBuffer(GL_ARRAY_BUFFER, vbo2ID);

    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    //glFrontFace(GL_CW);
    //glDrawElements(GL_TRIANGLES, indiceSize, GL_UNSIGNED_SHORT, 0);
    //glFrontFace(GL_CCW);

    //glEnable(GL_CULL_FACE);

    atmosphereToSkyShader.UnBind();
}

void Atmosphere::drawSkyFromSpace(float theta, glm::mat4 &MVP, glm::vec3 lightPos, glm::dvec3 &ppos)
{
    spaceToSkyShader.Bind();

    glm::mat4 GlobalModelMatrix(1.0);
    GlobalModelMatrix[0][0] = radius;
    GlobalModelMatrix[1][1] = radius;
    GlobalModelMatrix[2][2] = radius;
    GlobalModelMatrix[3][0] = ((float)((double)-ppos.x));
    GlobalModelMatrix[3][1] = ((float)((double)-ppos.y));
    GlobalModelMatrix[3][2] = ((float)((double)-ppos.z));

    glm::quat quaternion;
    // Have to rotate it and draw it again to make a sphere
    glm::vec3 EulerAngles(M_PI, 0, 0);
    quaternion = glm::quat(EulerAngles);
    glm::mat4 RotationMatrix = glm::toMat4(quaternion); 
    glm::mat4 MVPr = MVP * GlobalModelMatrix;
    glm::mat4 M = GlobalModelMatrix;

    float m_Kr4PI = m_Kr*4.0f*M_PI;
    float m_Km4PI = m_Km*4.0f*M_PI;
    float m_fScale = 1.0 / (radius - planetRadius);

    glUniformMatrix4fv(spaceToSkyShader.mvpID, 1, GL_FALSE, &MVPr[0][0]);
//    glUniformMatrix4fv(atmosphereShader.mID, 1, GL_FALSE, &M[0][0]);
    glUniform3f(spaceToSkyShader.cameraPosID, (float)ppos.x, (float)ppos.y, (float)ppos.z);

    glUniform3f(spaceToSkyShader.lightPosID, lightPos.x, lightPos.y, lightPos.z);
//    glUniform1f(atmosphereShader.gammaID, gamma);
    glUniform3f(spaceToSkyShader.invWavelengthID, 1/m_fWavelength4[0], 1/m_fWavelength4[1], 1/m_fWavelength4[2]);
    //glUniform1f(spaceToSkyShader.cameraHeightID, glm::length(ppos));
    
    glUniform1f(spaceToSkyShader.cameraHeight2ID, glm::length(ppos)*glm::length(ppos));
    glUniform1f(spaceToSkyShader.outerRadiusID, radius);
    //glUniform1f(spaceToSkyShader.outerRadius2ID, radius*radius);
    glUniform1f(spaceToSkyShader.innerRadiusID, planetRadius);
    //glUniform1f(spaceToSkyShader.innerRadius2ID, planetRadius*planetRadius);
    glUniform1f(spaceToSkyShader.KrESunID, m_Kr*m_ESun);
    glUniform1f(spaceToSkyShader.KmESunID, m_Km*m_ESun);
    glUniform1f(spaceToSkyShader.Kr4PIID, m_Kr4PI);
    glUniform1f(spaceToSkyShader.Km4PIID, m_Km4PI);
    glUniform1f(spaceToSkyShader.scaleID, m_fScale);
    glUniform1f(spaceToSkyShader.scaleDepthID, m_fRayleighScaleDepth);
    glUniform1f(spaceToSkyShader.scaleOverScaleDepthID, m_fScale/m_fRayleighScaleDepth);
    glUniform1f(spaceToSkyShader.gID, m_g);
    glUniform1f(spaceToSkyShader.g2ID, m_g*m_g);
    glUniform1f(spaceToSkyShader.fSamplesID, fSamples);
    glUniform1i(spaceToSkyShader.nSamplesID, nSamples);

    glBindBuffer(GL_ARRAY_BUFFER, vboID);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndexID);

    //glDisable(GL_CULL_FACE);


    glDrawElements(GL_TRIANGLES, indexSize, GL_UNSIGNED_SHORT, 0);

    /*glBindBuffer(GL_ARRAY_BUFFER, vbo2ID);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glFrontFace(GL_CW);
    glDrawElements(GL_TRIANGLES, indiceSize, GL_UNSIGNED_SHORT, 0);
    glFrontFace(GL_CCW);*/

    //glEnable(GL_CULL_FACE);

    spaceToSkyShader.UnBind();
}