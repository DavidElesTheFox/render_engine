#include <render_engine/assets/VolumetricObject.h>

namespace RenderEngine
{
    namespace
    {
        std::vector<glm::vec3> generatePositions(const glm::vec3& extent)
        {
            // Z forward right handed NDC in Vulkan
            glm::vec3 origin{ 0.0f };
            glm::vec3 half_extent = extent / 2.0f;

            glm::vec3 left_top_corner = origin - half_extent;
            const glm::vec3 right = { extent.x, 0.0f, 0.0f };
            const glm::vec3 down = { 0.0f, extent.y, 0.0f };
            const glm::vec3 back = { 0.0f, 0.0f, extent.z };
            return {
                left_top_corner,
                left_top_corner + right,
                left_top_corner + right + down,
                left_top_corner + down,
                left_top_corner + back,
                left_top_corner + back + right,
                left_top_corner + back + right + down,
                left_top_corner + back + down,
            };
        }

        std::vector<int16_t> generateIndexes()
        {
            /*
            4--------------5
            |              |
            |              |
            |              |
        0---+----------1   |
        |   |          |   |
        |   |          |   |
        |   7----------+---6
        |              |
        |              |
        |              |
        3--------------2
            */
            return
            {
                /* Front Face
                    0------------1
                    |            |
                    |            |
                    |            |
                    |            |
                    |            |
                    |            |
                    3------------2
                */
                0, 1, 2,
                2, 3, 0,
                /* Right Face
                    1------------5
                    |            |
                    |            |
                    |            |
                    |            |
                    |            |
                    |            |
                    2------------6
                */
                1, 5, 6,
                6, 2, 1,
                /* Back Face
                    5------------4
                    |            |
                    |            |
                    |            |
                    |            |
                    |            |
                    |            |
                    6------------7
                */
                5, 4, 7,
                7, 6, 5,
                /* Left Face
                    4------------0
                    |            |
                    |            |
                    |            |
                    |            |
                    |            |
                    |            |
                    7------------3
                */
                4, 0, 3,
                3, 7, 4,
                /* Bottom Face
                    3------------2
                    |            |
                    |            |
                    |            |
                    |            |
                    |            |
                    |            |
                    7------------6
                */
                3, 2, 6,
                6, 7, 3,
                /* Top Face
                    4------------5
                    |            |
                    |            |
                    |            |
                    |            |
                    |            |
                    |            |
                    0------------1
                */
                4, 5, 1,
                1, 0, 4
            };
        }

        std::vector<glm::vec3> generate3DTextureCoordinates()
        {
            return {
                glm::vec3{0.0f, 0.0f, 0.0f},
                glm::vec3{1.0f, 0.0f, 0.0f},
                glm::vec3{1.0f, 1.0f, 0.0f},
                glm::vec3{0.0f, 1.0f, 0.0f},

                glm::vec3{0.0f, 0.0f, 1.0f},
                glm::vec3{1.0f, 0.0f, 1.0f},
                glm::vec3{1.0f, 1.0f, 1.0f},
                glm::vec3{0.0f, 1.0f, 1.0f},
            };
        }
    }
    std::unique_ptr<Geometry> VolumetricObject::generateGeometry(const glm::vec3& extent)
    {
        std::unique_ptr<Geometry> result = std::make_unique<Geometry>();
        result->positions = generatePositions(extent);
        result->indexes = generateIndexes();
        result->texture_coord_3d = generate3DTextureCoordinates();
        return result;
    }
}