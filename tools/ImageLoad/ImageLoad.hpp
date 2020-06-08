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

class ImagePixelData;
class Texture;

namespace std {
    void swap(ImagePixelData& a, ImagePixelData& b);
    void swap(Texture& a, Texture& b);
}

class ImagePixelData {
    friend class Texture;
public:
    struct D {
        void operator()(uint8_t* d) const;
    };
private:
    int width;
    int height;
    int num_channels;
    std::unique_ptr<uint8_t, D> pixel_bytes;
    
    friend void std::swap(ImagePixelData& a, ImagePixelData& b);
public:
    static void load(ImagePixelData& image, const std::string& image_location, bool flip = false);
public:
    ImagePixelData();
    ImagePixelData(const ImagePixelData& copy);
    ImagePixelData(ImagePixelData&& move);

    ImagePixelData& operator=(ImagePixelData assign);

    ImagePixelData(const std::string& image_location, bool flip = false);

    inline int getWidth() const
    { return this->width; }

    inline int getHeight() const
    { return this->height; }

    std::unique_ptr<uint8_t, D> clonePixelBytes() const;
    std::unique_ptr<uint8_t, D> movePixelBytes();
};


class Texture {
    friend void std::swap(Texture& a, Texture& b);
private:
    std::unique_ptr<ImagePixelData> image_data;
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

    Texture(ImagePixelData&& move_byte_data);
    ~Texture();
    
    /**
     * Get the resource handle of the image on the gpu.
     */
    inline ImageRID getHandle() const
    { return handle; }

    inline int getWidth() const
    { return image_data->width; }

    inline int getHeight() const
    { return image_data->height; }
};
