#pragma once

#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include "Model.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

std::vector<std::string> getNextLineAndSplitIntoTokens(std::istream& str)
{
    std::vector<std::string>   result;
    std::string                line;
    std::getline(str, line);

    std::stringstream          lineStream(line);
    std::string                cell;

    while (std::getline(lineStream, cell, ','))
    {
        result.push_back(cell);
    }
    // This checks for a trailing comma with no data after it.
    if (!lineStream && cell.empty())
    {
        // If there was a trailing comma then add an empty element.
        result.push_back("");
    }
    return result;
}

struct staticMesh
{
    uint16_t* trilist;
    VertexData* vertexdata;
    bgfx::VertexBufferHandle vbh;
    bgfx::IndexBufferHandle ibh;
};

struct animatedMesh
{
    uint16_t* trilist;
    //animated vertexdata
    //bonesmatrix
    bgfx::VertexBufferHandle vbh;
    bgfx::IndexBufferHandle ibh;
};

struct texture
{
    bgfx::TextureHandle texh;
};

int LoadStaticMeshFile(std::string fileName, staticMesh& mesh)
{
    std::ifstream fs;
    fs.open(fileName, std::ios::in | std::ios::binary);
    fs.seekg(0, std::ios::end);
    const size_t LEN = fs.tellg();
    fs.seekg(0, std::ios::beg);

    //------------------
    //load tri indicies
    //------------------

    uint16_t* trilist = NULL;
    uint16_t trilist_count;

    uint8_t header = 0;
    fs.read((char*)&header, 1);

    if (header == uint8_t(0xAA))
    {
        uint32_t count = 0;
        fs.read((char*)&count, 4);

        trilist_count = count;
        trilist = new uint16_t[count];

        uint32_t readsize = trilist_count * sizeof(uint16_t);

        fs.read((char*)trilist, readsize);

        uint8_t end = 0;
        fs.read((char*)&end, 1);
        if (end != 0x00)
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }

    mesh.trilist = trilist;

    //----------------
    //Load Vertex Data
    //----------------

    VertexData* vertexdata = NULL;
    uint32_t vertexdata_count;

    fs.read((char*)&header, 1);
    if (header == uint8_t(0xBB))
    {
        uint32_t count = 0;
        fs.read((char*)&count, 4);

        uint32_t vertexData_size = sizeof(VertexData);

        uint32_t ReadSize = count * vertexData_size;

        vertexdata_count = count;
        vertexdata = new VertexData[count];

        fs.read((char*)vertexdata, ReadSize);

        uint8_t end = 0;
        fs.read((char*)&end, 1);
        if (end != 0x00)
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }

    mesh.vertexdata = vertexdata;



    size_t readpos = fs.tellg();

    fs.read((char*)&header, 1);
    if (header == uint8_t(0xDD))
    {
        assert("mesh load failed - fileformat");//error
    }



    fs.close();
}

const bgfx::Memory* AreadTexture(const char* filename) {
    /// Opens a file and returns a bgfx::Memory of the raw data. The lifetime of the data is controlled by bgfx
    std::ifstream fs(filename, std::ios::in | std::ios::binary);
    if (!fs.is_open()) {
        return NULL;
    }
    fs.seekg(0, std::ios::end);
    const size_t LEN = fs.tellg();
    fs.seekg(0, std::ios::beg);

    const bgfx::Memory* mem = bgfx::alloc(LEN);
    fs.read((char*)mem->data, LEN);
    fs.close();
    return mem;
}

int LoadTexture(std::string fileName, bgfx::TextureHandle &ret_tex)
{
    if(fileName.substr(fileName.find_last_of(".") + 1) == "png") // load png image
    {
        int x, y, n;
        unsigned char* image = stbi_load(fileName.c_str(), &x, &y, &n, 0);
        const bgfx::Memory* mem_image = bgfx::makeRef(image, x * y * n);
        //const bgfx::Memory* mem_image = bgfx::copy(image, x * y * n);
        //stbi_image_free(image);
        bgfx::TextureHandle texture = bgfx::createTexture2D(x, y, false, 1, bgfx::TextureFormat::RGBA8, 0 | BGFX_TEXTURE_RT
            | BGFX_SAMPLER_MIN_POINT
            | BGFX_SAMPLER_MAG_POINT
            | BGFX_SAMPLER_MIP_POINT
            | BGFX_SAMPLER_U_CLAMP
            | BGFX_SAMPLER_V_CLAMP, mem_image);

        return 1;
    }
    else if(fileName.substr(fileName.find_last_of(".") + 1) == "dds")    //if DDS
    {
        bgfx::TextureHandle texture = bgfx::createTexture(AreadTexture(fileName.c_str()));
        return 1;
    }
    else
    {
        return 0;
    }


}

void LoadPropsFile(std::string fileName)
{

    std::unordered_map<std::string, texture> TexturesMap;
    std::unordered_map<std::string, staticMesh> StaticMeshMap;

    std::filebuf fb;
    if (fb.open(fileName, std::ios::in))
    {
        std::istream is(&fb);

        if (is)
        {
            std::vector<std::string> line = getNextLineAndSplitIntoTokens(is); //read first header line
        }

        while (is)
        {
            std::vector<std::string> line = getNextLineAndSplitIntoTokens(is);
            if (line.size() == 3)
            {
                std::string name = line[0];
                std::string meshfile = line[1];
                std::string texturefile = line[2];

                //-----------------------------
                //load texture in map
                //if texture not already loaded
                //-----------------------------

                if (TexturesMap.find(texturefile) == TexturesMap.end())
                {
                    bgfx::TextureHandle texhandle;
                    if (LoadTexture("Assets\\\Texture\\" + texturefile, texhandle))
                    {
                        texture newtexture;
                        newtexture.texh = texhandle;
                        std::pair<std::string, texture> addtexture(texturefile, newtexture);
                        TexturesMap.insert(addtexture);
                    }
                    else
                    {
                        //texture cant be loaded
                        //replace with debug texture?
                    }
                }

                //---------------------------
                //load meshdata if not in map
                //---------------------------

                if (StaticMeshMap.find(meshfile) == StaticMeshMap.end())
                {
                    staticMesh Meshcontainer;
                    if (LoadStaticMeshFile("Assets\\\Mesh\\" + meshfile, Meshcontainer))
                    {
                        std::pair<std::string,staticMesh> addmesh(meshfile, Meshcontainer);
                        StaticMeshMap.insert(addmesh);
                    }

                }





            }




        }
        fb.close();
    }





	//read props file
	//create prop entity if doesnt already exist?
	//check if mesh already loaded
	//load if not
	//check if texture already loaded
	//load if not
}