/*

MIT License

Copyright (c) 2020 Jonathan Mendez

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

 */
#include <iostream>
#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "ImageLoad.hpp"


#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#include "GL/gl3w.h"
#include "GLFW/glfw3.h"

#include "../TP/TP.hpp"

namespace GPUTexture {
    void openGLUpload(ImageRID& rid, int width, int height, int num_channels, const uint8_t* bytes){
        unsigned int pixel_fmt_src;
        switch(num_channels){
            case 1:
                pixel_fmt_src = GL_R8;
                break;
            case 2:
                pixel_fmt_src = GL_RG8;
                break;
            case 3: // RGB
                pixel_fmt_src = GL_RGB;
                break;
            case 4: // RGBA
                pixel_fmt_src = GL_RGBA;
                break;
            default:
                return;
        }
        GLuint tex_id = 0;
        glGenTextures(1, &tex_id);
        glBindTexture(GL_TEXTURE_2D, tex_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        /*
        // Texture filtering.
        */
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Without this there are crashes when deleting and assigning a new texture.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, pixel_fmt_src, GL_UNSIGNED_BYTE, bytes);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        rid = tex_id;
    }

    void openGLCopy(ImageRID& dest, const ImageRID& src, int width, int height, int num_channels){
        ImageRID rid = 0;
        // Prepare the destination texture.
        openGLUpload(rid, width, height, num_channels, nullptr);
        glCopyImageSubData(
            src, GL_TEXTURE_2D, 0, 0, 0, 0,
            rid,GL_TEXTURE_2D, 0, 0, 0, 0,
            width, height, 1
        );
        dest = rid;
    }

    /**
     * Does not modify the value of rid.
     * "glDeleteTextures silently ignores 0's and names that do not correspond to existing textures." - khronos.org
     */
    void openGLFree(const ImageRID& rid){
        glDeleteTextures(1, (GLuint*)&rid);
    }

    namespace SideLoader {
        static std::mutex gl_ctx_mutex;
        static GLFWwindow* texture_sideload_ctx = nullptr;


        void create_context() {
            if(texture_sideload_ctx)
                return; // Has already been successfully created.

            // Create a seperate glfw context and share texture resources with the current context.
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            texture_sideload_ctx = glfwCreateWindow(640, 480, "Texture sideloader.", NULL, glfwGetCurrentContext());
        }
        
        void add_job(GPUTextureJob job){
            TP::add_job(
                [job = std::move(job)](){
                    std::lock_guard<std::mutex> lock{gl_ctx_mutex};
                    glfwMakeContextCurrent(texture_sideload_ctx);
                    job();
                    glfwMakeContextCurrent(NULL);
                }
            );
        }
    }
}

ImagePixelData::ImagePixelData(ImagePixelData&& move_data)
    : ImagePixelData()
{
    std::swap(*this, move_data);
    //move_data.bytes = nullptr;
}

void Texture::free() {
    GPUTexture::SideLoader::add_job([handle = handle](){
        GPUTexture::openGLFree(handle);
    });
    handle = 0;
}

ImagePixelData::ImagePixelData()
    : width{}
    , height{}
    , num_channels{}
    , pixel_bytes{}
{}

ImagePixelData::ImagePixelData(const ImagePixelData& copy)
    : width{ copy.width }
    , height{ copy.height }
    , num_channels{ copy.num_channels }
    , pixel_bytes{ copy.clonePixelBytes() }
{}

ImagePixelData& ImagePixelData::operator=(ImagePixelData assign) {
    std::swap(*this, assign);
    return *this;
}

std::unique_ptr<uint8_t, ImagePixelData::D> ImagePixelData::clonePixelBytes() const {
    decltype(pixel_bytes) bd_clone;

    if(pixel_bytes)
        memcpy(bd_clone.get(), pixel_bytes.get(), width*height*num_channels);
    
    return bd_clone;
}

std::unique_ptr<uint8_t, ImagePixelData::D> ImagePixelData::movePixelBytes() {
    return std::move(pixel_bytes);
}

ImagePixelData::ImagePixelData(const std::string& image_location, bool flip) {
    this->load(*this, image_location, flip);
}

void ImagePixelData::load(ImagePixelData& image, const std::string& image_location, bool flip) {
    FILE* image_file = fopen(image_location.c_str(), "rb");
    if(image_file == nullptr)
        return;
    
    // Load the image file location.
	stbi_set_flip_vertically_on_load(flip);
    uint8_t* bytes{ stbi_load_from_file(image_file, &image.width, &image.height, &image.num_channels, 0) };
    image.pixel_bytes = decltype(image.pixel_bytes)(
	    bytes,
        D() // The function object will be called to free the bytes read from the file.
    );

    fclose(image_file);
}

void Texture::upload(Texture& texture) {
    if(!texture.image_data->pixel_bytes)
        return; // There is no image data to upload to the gpu.
    if(!glfwGetCurrentContext())
        return; // There is no open gl context, therefore we cannot upload the texture data.
    GPUTexture::openGLFree(texture.handle);
    GPUTexture::openGLUpload(
        texture.handle,
        texture.image_data->width,
        texture.image_data->height,
        texture.image_data->num_channels,
        texture.image_data->pixel_bytes.get()
    );
    texture.image_data->pixel_bytes.reset();
}

void Texture::uploadAsync(std::shared_ptr<Texture> texture) {
    GPUTexture::SideLoader::add_job([texture = std::move(texture)](){
        Texture::upload(*texture);
    });
}

void ImagePixelData::D::operator()(uint8_t* d) const {
    stbi_image_free(d);
}

namespace std {
    void swap(ImagePixelData& a, ImagePixelData& b){
        swap(a.width, b.width);
        swap(a.height, b.height);
        swap(a.num_channels, b.num_channels);
        swap(a.pixel_bytes, b.pixel_bytes);
    }

    void swap(Texture& a, Texture& b){
        swap(a.handle, b.handle);
        swap(a.image_data, b.image_data);
    }
}

Texture::Texture()
    : image_data{std::make_unique<ImagePixelData>()}
    , handle{ }
{}

Texture::Texture(ImagePixelData&& image)
    : image_data{std::make_unique<ImagePixelData>(std::move(image))}
{}

Texture::~Texture() {
    free();
}

Texture& Texture::operator=(Texture assign){
    std::swap(*this, assign);
    return *this;
}
