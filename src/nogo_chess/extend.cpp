/* This File is part of $PROGRAM_NAME
/  $PROGRAM_NAME is free software: you can redistribute it and/or modify
/  it under the terms of the GNU Lesser General Public License as published by
/  the Free Software Foundation, either version 3 of the License, or
/  (at your option) any later version.
/
/  $PROGRAM_NAME is distributed in the hope that it will be useful,
/  but WITHOUT ANY WARRANTY; without even the implied warranty of
/  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/  GNU Lesser General Public License for more details.
/
/  You should have received a copy of the GNU Lesser General Public License
/  along with $PROGRAM_NAME at /LICENSE.
/  If not, see <http://www.gnu.org/licenses/>. */


#include <vector>
#include <stdio.h>
#include <iostream>

#include <glad/glad.h>
#include <glfw/glfw3.h>

#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "function.h"
#include "catalog.h"
#include "fetch.h"
#include "extend.h"

int Shader::init(std::string path, Catalog cfg) {

    int success = 0;
    FileSystem fs[2] = {};
    char* content[2] = {};
    const char* suffix[2] = {".vsh", ".fsh"};

    const int infoSize = 4096;
    char info[infoSize] = {};

    const int msgSize = 128;
    char msg0[2][msgSize] = { "\n\nVERTEX COMPILATION ERROR MESSAGES:\n",
                              "\n\nFRAGMENT COMPILATION ERROR MESSAGES:\n" };

    char msg1[2][msgSize] = { "Failed to load shader::Vertex Shader Compilation failed! Details in render.log\n",
                              "Failed to load shader::Fragment Shader Compilation failed! Details in render.log" };

    for (int i = 0; i < 2; i++) {
        if (!fs[i].init((path + suffix[i]).c_str(), 0)) return i + 1;
        fs[i].fill();
        content[i] = new char[fs[i].length];
        fs[i].fetch(content[i]);
    }

    int handle[2] = { handles.vs, handles.fs };
    int mode[2] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };

    for (int i = 0; i < 2; i++) {
        handle[i] = glCreateShader(mode[i]);
        glShaderSource(handle[i], 1, &content[i], NULL);
        glCompileShader(handle[i]);
        glGetShaderiv(handle[i], GL_COMPILE_STATUS, &success);

        if (!success) {
            glGetShaderInfoLog(handle[i], infoSize, NULL, info);
            cfg.print(("SHADER::" + path + "\n" + msg0[i] + info).c_str());
            memset(info, 0, infoSize);
            std::cout << msg1[i] << std::endl;
        }
        delete[] content[i];
    }
    {
        handles.main = glCreateProgram();
        for (int i = 0; i < 2; i++) glAttachShader(handles.main, handle[i]);
        glLinkProgram(handles.main);
        glGetProgramiv(handles.main, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(handles.main, infoSize, NULL, info);
            cfg.print(("SHADER::" + path + "\n\nSHADER LINKING ERROR MESSAGES:\n" + info).c_str());
            memset(info, 0, infoSize);
            std::cout << "Failed to load shader::Shader Linking failed! Details in render.log" << std::endl;
        }}

    for (int i = 0; i < 2; i++) glDeleteShader(handle[i]);
    return 0;
}

void Mesh::do_Draw(Shader shade) {

    // Draw Mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

}
void Mesh::do_SetupMesh() {

    // Create Buffers/Arrays
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    // Load Data into Vertex Buffers
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // A great thing about structs is that their memory layout is sequential for all its items.
    // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
    // again translates to 3/2 floats which translates to a byte array.
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &(vertices[0]), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &(indices[0]), GL_STATIC_DRAW);

    // Set the Vertex Attribute Pointers
    // Vertex Positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // Vertex Normals
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // Vertex Texture Coords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    // Vertex Tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    // Vertex BiTangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

    glBindVertexArray(0);

}

void Model::do_Draw() {
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].do_Draw(shade);
}
Mesh Model::do_ProcessMesh(aiMesh* mesh, const aiScene* scene) {
    // Data to fill
    std::vector<unsigned int> indices = {};
    std::vector<Vertex> vertices = {};
    // Walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vert = {};
        glm::vec3 v3 = {}; // We declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
        // Positions
        v3.x = mesh->mVertices[i].x;
        v3.y = mesh->mVertices[i].y;
        v3.z = mesh->mVertices[i].z;
        vert.Position = v3;
        // Normals
        if (mesh->HasNormals()) {
            v3.x = mesh->mNormals[i].x;
            v3.y = mesh->mNormals[i].y;
            v3.z = mesh->mNormals[i].z;
            vert.Normal = v3;
        }
        // Texture Coordinates
        if (mesh->mTextureCoords[0]) {
            glm::vec2 v2 = {};
            // A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
            // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
            v2.x = mesh->mTextureCoords[0][i].x;
            v2.y = mesh->mTextureCoords[0][i].y;
            vert.TexCoords = v2;
            // Tangent
            v3.x = mesh->mTangents[i].x;
            v3.y = mesh->mTangents[i].y;
            v3.z = mesh->mTangents[i].z;
            vert.Tangent = v3;
            // Bitangent
            v3.x = mesh->mBitangents[i].x;
            v3.y = mesh->mBitangents[i].y;
            v3.z = mesh->mBitangents[i].z;
            vert.Bitangent = v3;
        }
        else vert.TexCoords = glm::vec2(0.0f, 0.0f);
        vertices.push_back(vert);
    }
    // Now walk through each of the mesh's faces and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        // Retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    // Process materials
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    // Return a mesh object created from the extracted mesh data
    return Mesh(vertices, indices);
}
void Model::do_ProcessNode(aiNode* node, const aiScene* scene) {
    // Process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        // The node object only contains indices to index the actual objects in the scene. 
        // The scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(do_ProcessMesh(mesh, scene));
    }
    // After we've processed all of the meshes (if any) we then recursively process each of the children nodes.
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        do_ProcessNode(node->mChildren[i], scene);
    }
}
void Model::do_LoadModel(std::string const& path, Catalog* ctlog) {
    ctg = ctlog;
    // Read model via ASSIMP
    Assimp::Importer importer = {};
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // Check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        ctg->print("ERROR::ASSIMP:: ");
        ctg->print(importer.GetErrorString());
        ctg->print("\n");
    }
    // Process ASSIMP's root node recursively
    do_ProcessNode(scene->mRootNode, scene);
}