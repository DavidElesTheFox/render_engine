#pragma once

#include <GLFW/glfw3.h>

#include <chrono>
#include <render_engine/RenderContext.h>
#include <scene/Scene.h>
#include <set>

class ApplicationContext
{
private:
    /**
    * @TODO: Implement proper id generation
    */
    class IdGenerator
    {
    public:
        uint32_t generateId()
        {
            if (next_id >= RenderEngine::RenderContext::kEngineReservedIdStart)
            {
                throw std::runtime_error("No more object has place in the application");
            }
            assert(next_id + 1 != 0 && "Overflow detected");
            return next_id++;
        };
    private:
        uint32_t next_id{ 0 };
    };
public:
    struct MouseEventData
    {
        glm::vec2 dragging_coordinates{ 0.0f };
        bool is_dragging{ false };
    };
    struct KeyboardEventData
    {
        std::set<int> pressed_keys;
    };

    static ApplicationContext& instance()
    {
        static ApplicationContext _instance;
        return _instance;
    }

    void init(Scene::Scene* scene, GLFWwindow* window_handler);
    void updateInputEvents();
    friend void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    friend void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    friend void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    float getMouseSensitivity() const { return _mouse_sensitivity; }
    uint32_t generateId() { return _id_generator.generateId(); }

    void onFrameBegin();
    void onGui();
    void onFrameEnd();

private:
    using ApplicationClock = std::chrono::steady_clock;
    struct FpsData
    {
        float current{ 0.0f };
        float min = std::numeric_limits<float>::max();
        float max = std::numeric_limits<float>::min();
        std::deque<float> tail;
        static constexpr const uint32_t tail_size = 120;
    };
    void updateKeyboardEvent();
    Scene::Scene* getScene() { return _scene; }

    Scene::Scene* _scene{ nullptr };
    MouseEventData _mouse_event_data;
    KeyboardEventData _keyboard_event_data;
    float _mouse_sensitivity = 0.005f;
    float _keyboard_sensitivity = 0.01f;
    GLFWcursorposfun original_cursor_position_callback{ nullptr };
    GLFWmousebuttonfun original_mouse_button_callback{ nullptr };
    GLFWkeyfun originak_key_callback{ nullptr };
    IdGenerator _id_generator;
    FpsData _fps_data{};
    ApplicationClock::time_point _frame_start_time{};
};
