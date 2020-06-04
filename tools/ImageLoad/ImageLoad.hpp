/**
 * 2020 Jonathan Mendez
 */

#pragma once
#include <functional>

using ImageRID = uintptr_t;

struct ImageResourceData {
    int width;
    int height;
    int num_channels;
    uint8_t* bytes;

    ImageResourceData() = default;
    ImageResourceData(const ImageResourceData& copy_data);
    ImageResourceData(ImageResourceData&& move_data);

    ImageResourceData& operator=(const ImageResourceData& copy_data);

    ~ImageResourceData(){
        if(bytes)
            delete[] bytes;
    }
};

namespace std {
    inline void swap(ImageResourceData& a, ImageResourceData& b){
        swap(a.width, b.width);
        swap(a.height, b.height);
        swap(a.num_channels, b.num_channels);
        swap(a.bytes, b.bytes);
    }
}

namespace GPUTexture {
    void openGLUpload(ImageRID& rid, int width, int height, int num_channels, const uint8_t* bytes);
    void openGLFree(const ImageRID& rid);

    /**
     * A seperate thread for texture upload jobs to be appended.
     */
    namespace SideLoader {
        using GPUTextureJob = std::function<void()>;
        void create_context();
        void add_job(std::function<void()>);
    }
}

class ImageResource {
private:
    ImageResourceData data;

    ImageRID rid = 0;
    
    void freeBytes();
    void freeTexture();
    void loadFILE(FILE* image_file, bool flip = false);
    void loadPATH(const std::string& image_path, bool flip = false);
public:
    ~ImageResource();
    inline ImageResource(){}
    ImageResource(const ImageResource& img_res);
    ImageResource(const std::string& image_location, bool to_gpu, bool flip = false);
    void load(const std::string& image_location, bool to_gpu, bool flip = false);
    
    void sendToGPU();

    inline int getWidth() { return this->data.width; }
    inline int getHeight() { return this->data.height; }
    /**
     * Get the resource handle of the image on the gpu.
     */
    inline ImageRID getRID() {
        return this->rid;
    }
};
