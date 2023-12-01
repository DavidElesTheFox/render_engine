#include <ApplicationContext.h>

#include <scene/Camera.h>

#include <iostream>

#include <imgui.h>

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (ApplicationContext::instance().originak_key_callback)
	{
		ApplicationContext::instance().originak_key_callback(window, key, scancode, action, mods);
	}
    switch(action)
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

			event_data.dragging_coordinates.x = -ypos;
			event_data.dragging_coordinates.y = xpos;
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

	original_cursor_position_callback = glfwSetCursorPosCallback(window_handler, cursorPositionCallback);
	originak_key_callback = glfwSetKeyCallback(window_handler, keyCallback);
	original_mouse_button_callback = glfwSetMouseButtonCallback(window_handler, mouseButtonCallback);
}

void ApplicationContext::updateInputEvents()
{
    updateKeyboardEvent();
}

void ApplicationContext::onGui()
{
    ImGui::Begin("Application");
    ImGui::SliderFloat("Mouse Sensitivity", &_mouse_sensitivity, 0.0f, 1.0f);
    ImGui::SliderFloat("Keyboard Sensitivity", &_keyboard_sensitivity, 0.0f, 1.0f);
    ImGui::End();
}

void ApplicationContext::updateKeyboardEvent()
{
    auto* scene = ApplicationContext::instance().getScene();
    auto* camera = scene->getActiveCamera();

    if(camera == nullptr || _keyboard_event_data.pressed_keys.empty())
    {
        return;
    }
    const auto forward = camera->getTransformation().getRotation() * scene->getSceneSetup().forward;
    const auto up = camera->getTransformation().getRotation() * scene->getSceneSetup().up;
    for(int key : _keyboard_event_data.pressed_keys)
    {
        if(key == GLFW_KEY_W)
        {
            camera->getTransformation().translate(_keyboard_sensitivity * forward);
        }
        if(key == GLFW_KEY_S)
        {
            camera->getTransformation().translate(-_keyboard_sensitivity * forward);
        }
        if(key == GLFW_KEY_A)
        {
            auto right = glm::cross(forward, up);
            camera->getTransformation().translate(-_keyboard_sensitivity * right);
        }
        if(key == GLFW_KEY_D)
        {
            auto right = glm::cross(forward, up);
            camera->getTransformation().translate(_keyboard_sensitivity * right);
        }
    }
}
