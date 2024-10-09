#include <ApplicationContext.h>

#include <scene/Camera.h>

#include <iostream>
#include <numeric>

#include <imgui.h>

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (ApplicationContext::instance().originak_key_callback)
    {
        ApplicationContext::instance().originak_key_callback(window, key, scancode, action, mods);
    }
    switch (action)
    {
        case GLFW_PRESS:
            ApplicationContext::instance()._keyboard_event_data.pressed_keys.insert(key);
            break;
        case GLFW_RELEASE:
            ApplicationContext::instance()._keyboard_event_data.pressed_keys.erase(key);
            break;
        default:
            break;
    }
}

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{

    if (ApplicationContext::instance().original_cursor_position_callback)
    {
        ApplicationContext::instance().original_cursor_position_callback(window, xpos, ypos);
    }
    auto* camera = ApplicationContext::instance().getScene()->getActiveCamera();

    if (camera == nullptr)
    {
        return;
    }
    auto& event_data = ApplicationContext::instance()._mouse_event_data;
    if (event_data.is_dragging == false)
    {
        return;
    }
    const glm::vec2 current_coordinates = glm::vec2{ -ypos, xpos };
    const glm::vec2 delta = event_data.dragging_coordinates - current_coordinates;

    const glm::quat rotation{ glm::vec3(delta, 0.0f) * ApplicationContext::instance().getMouseSensitivity() };

    camera->getTransformation().rotate(rotation);

    event_data.dragging_coordinates = current_coordinates;

}
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (ApplicationContext::instance().original_mouse_button_callback)
    {
        ApplicationContext::instance().original_mouse_button_callback(window, button, action, mods);
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            auto& event_data = ApplicationContext::instance()._mouse_event_data;
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);

            event_data.dragging_coordinates.x = static_cast<float>(-ypos);
            event_data.dragging_coordinates.y = static_cast<float>(xpos);
            event_data.is_dragging = true;
        }
        else if (action == GLFW_RELEASE)
        {
            ApplicationContext::instance()._mouse_event_data.is_dragging = false;
        }
    }
}

void ApplicationContext::init(Scene::Scene* scene, GLFWwindow* window_handler)
{
    _scene = scene;
    if (window_handler != nullptr)
    {
        original_cursor_position_callback = glfwSetCursorPosCallback(window_handler, cursorPositionCallback);
        originak_key_callback = glfwSetKeyCallback(window_handler, keyCallback);
        original_mouse_button_callback = glfwSetMouseButtonCallback(window_handler, mouseButtonCallback);
    }
}

void ApplicationContext::updateInputEvents()
{
    updateKeyboardEvent();
}

void ApplicationContext::onFrameBegin()
{
    _frame_start_time = ApplicationClock::now();
}

void ApplicationContext::onGui()
{
    std::lock_guard lock(_ui_mutex);

    ImGui::Begin("Application");
    ImGui::SliderFloat("Mouse Sensitivity", &_mouse_sensitivity, 0.0f, 1.0f);
    ImGui::SliderFloat("Keyboard Sensitivity", &_keyboard_sensitivity, 0.0f, 1.0f);
    ImGui::Text("FPS: %06.1f", _fps_data.current);
    ImGui::Text("FPS min: %06.1f", _fps_data.min);
    ImGui::Text("FPS max: %06.1f", _fps_data.max);
    ImGui::Text("FPS avg of %d: %06.1f",
                _fps_data.tail.size(),
                std::accumulate(_fps_data.tail.begin(), _fps_data.tail.end(), 0.0f) / static_cast<float>(_fps_data.tail.size()));
    ImGui::End();
}

void ApplicationContext::onFrameEnd()
{
    std::lock_guard lock(_ui_mutex);
    const auto frame_time = std::chrono::steady_clock::now() - _frame_start_time;
    const auto frame_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(frame_time).count();
    if (frame_time_ms == 0)
    {
        return;
    }
    auto fps = 1000 / static_cast<float>(frame_time_ms);
    _fps_data.current = fps;
    _fps_data.min = std::min(fps, _fps_data.min);
    _fps_data.max = std::max(fps, _fps_data.max);
    _fps_data.tail.push_front(fps);
    if (_fps_data.tail.size() > FpsData::tail_size)
    {
        _fps_data.tail.pop_back();
    }
}

void ApplicationContext::updateKeyboardEvent()
{
    auto* scene = ApplicationContext::instance().getScene();
    auto* camera = scene->getActiveCamera();

    if (camera == nullptr || _keyboard_event_data.pressed_keys.empty())
    {
        return;
    }
    const auto forward = camera->getTransformation().getRotation() * scene->getSceneSetup().forward;
    const auto up = camera->getTransformation().getRotation() * scene->getSceneSetup().up;
    for (int key : _keyboard_event_data.pressed_keys)
    {
        if (key == GLFW_KEY_W)
        {
            camera->getTransformation().translate(_keyboard_sensitivity * forward);
        }
        if (key == GLFW_KEY_S)
        {
            camera->getTransformation().translate(-_keyboard_sensitivity * forward);
        }
        if (key == GLFW_KEY_A)
        {
            auto right = glm::cross(forward, up);
            camera->getTransformation().translate(-_keyboard_sensitivity * right);
        }
        if (key == GLFW_KEY_D)
        {
            auto right = glm::cross(forward, up);
            camera->getTransformation().translate(_keyboard_sensitivity * right);
        }
    }
}
