#include "ShaderLoader.h"

bgfx::ShaderHandle loadShader(const char* _name) {
	char data[600];
	std::ifstream file(_name, std::ios::binary);
	size_t fileSize;
	if (file.is_open()) {
		file.seekg(0, std::ios::end);
		fileSize = file.tellg();
		file.seekg(0, std::ios::beg);
		file.read(data, fileSize);
		file.close();
	}
	const bgfx::Memory* mem = bgfx::copy(data, fileSize + 1);
	mem->data[mem->size - 1] = '\0';
	bgfx::ShaderHandle handle = bgfx::createShader(mem);
	bgfx::setName(handle, _name);
	return handle;
}


const bgfx::Memory* readFileRawToBgfxMemory(const char* filename) {
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

const bgfx::Memory* readFileRawToBgfxMemory(const BGFXH_STRING& filename) {
	return readFileRawToBgfxMemory(filename.c_str());
}

bgfx::ProgramHandle loadProgram(const char* vertexShaderFile, const char* fragementShaderFile) {
	const bgfx::Memory* vsShaderFileMem = readFileRawToBgfxMemory(vertexShaderFile); //bgfx will auto-dealloc this when done
	bgfx::ShaderHandle vsh = bgfx::createShader(vsShaderFileMem);
	bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
	if (fragementShaderFile) {
		const bgfx::Memory* fsShaderFileMem = readFileRawToBgfxMemory(fragementShaderFile);
		fsh = bgfx::createShader(fsShaderFileMem);
	}

	return bgfx::createProgram(vsh, fsh, true /* destroy shaders when program is destroyed */);
}

bgfx::ProgramHandle loadProgram(const BGFXH_STRING& vertexShaderFile, const BGFXH_STRING& fragmentShaderFile) {
	return loadProgram(vertexShaderFile.c_str(), fragmentShaderFile.c_str());
}
