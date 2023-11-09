#include "Image.h"

#include <fstream>
namespace RenderEngine
{
    std::vector<uint8_t> Image::readData() const
    {
        std::ifstream file(_path.c_str(), std::ios::ate | std::ios::binary);

        if (!file.is_open()) {

            throw std::runtime_error("failed to open file " + _path.string());
        }

        size_t fileSize = file.tellg();
        std::vector<uint8_t> buffer(fileSize);

        file.seekg(0);
        static_assert(sizeof(char) == sizeof(uint8_t));
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

        file.close();

        return buffer;
    }
    BufferInfo Image::createBufferInfo() const
    {
        return BufferInfo{
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .size = getSize(),
                .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                .mapped = true
        };
    }
    VkDeviceSize Image::getSize() const
    {
        VkDeviceSize result = getWidth() * getHeight();
        switch (_format)
        {
        case VK_FORMAT_R8G8B8A8_SRGB:
            return result *= 4;
        default:
            throw std::runtime_error("Unhandled image format");
        }
        return result;
    }
}
