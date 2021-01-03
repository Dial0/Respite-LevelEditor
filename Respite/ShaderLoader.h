#pragma once
#include <iostream>
#include <string>
#include <stdio.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <fstream>

bgfx::ShaderHandle loadShader(const char* _name);

typedef std::string BGFXH_STRING;

const bgfx::Memory* readFileRawToBgfxMemory(const char* filename);

const bgfx::Memory* readFileRawToBgfxMemory(const BGFXH_STRING& filename);

bgfx::ProgramHandle loadProgram(const char* vertexShaderFile, const char* fragementShaderFile);

bgfx::ProgramHandle loadProgram(const BGFXH_STRING& vertexShaderFile, const BGFXH_STRING& fragmentShaderFile);
