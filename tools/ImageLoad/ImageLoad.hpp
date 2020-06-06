/**
 * 2020 Jonathan Mendez
 */

#pragma once
#include <functional>
#include <memory>

using ImageRID = uintptr_t;

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

class ImageByteData;
class Texture;

namespace std {
    void swap(ImageByteData& a, ImageByteData& b);
    void swap(Texture& a, Texture& b);
}

class ImageByteData {
    friend class Texture;
public:
    struct D {
        void operator()(uint8_t* d) const;
    };
private:
    int width;
    int height;
    int num_channels;
    std::unique_ptr<uint8_t, D> byte_data;
    
    friend void std::swap(ImageByteData& a, ImageByteData& b);

    void free();
    void loadFILE(FILE* image_file, bool flip = false);
    void loadPATH(const std::string& image_path, bool flip = false);
    void load(const std::string& image_location, bool flip = false);
public:
    ImageByteData();
    ImageByteData(const ImageByteData& copy);
    ImageByteData(ImageByteData&& move);

    ImageByteData& operator=(ImageByteData assign);

    ImageByteData(const std::string& image_location, bool flip = false);

    inline int getWidth() const
    { return this->width; }

    inline int getHeight() const
    { return this->height; }

    std::unique_ptr<uint8_t, D> cloneBytes() const;
};


class Texture {
    std::unique_ptr<ImageByteData> img_res;
private:
    ImageRID handle;
    void free();
public:
    static void upload(Texture& texture);
    static void uploadAsync(std::shared_ptr<Texture> texture_shared);
public:

    Texture();
    Texture(const Texture& copy);
    Texture(Texture&& move);

    Texture& operator=(Texture assign);

    Texture(ImageByteData&& move_byte_data);
    ~Texture();
    
    /**
     * Get the resource handle of the image on the gpu.
     */
    inline ImageRID getHandle() const
    { return handle; }

    inline int getWidth() const
    { return img_res->width; }

    inline int getHeight() const
    { return img_res->height; }
};
