#include <ApplicationContext.h>

#include <scene/Camera.h>

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (ApplicationContext::instance().originak_key_callback)
	{
		ApplicationContext::instance().originak_key_callback(window, key, scancode, action, mods);
	}
	auto* scene = ApplicationContext::instance().getScene();
	auto* camera = scene->getActiveCamera();

	if (camera == nullptr)
	{
		return;
	}

	if (key == GLFW_KEY_W && action == GLFW_PRESS)
	{
		camera->getTransformation().translate(scene->getSceneSetup().forward);
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
	{
		camera->getTransformation().translate(-scene->getSceneSetup().forward);
	}
	if (key == GLFW_KEY_A && action == GLFW_PRESS)
	{
		auto right = glm::cross(scene->getSceneSetup().forward, scene->getSceneSetup().up);
		camera->getTransformation().translate(-right);
	}
	if (key == GLFW_KEY_D && action == GLFW_PRESS)
	{
		auto right = glm::cross(scene->getSceneSetup().forward, scene->getSceneSetup().up);
		camera->getTransformation().translate(right);
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
	auto& event_data = ApplicationContext::instance().mouse_event_data;
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
			auto& event_data = ApplicationContext::instance().mouse_event_data;
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);

			event_data.dragging_coordinates.x = -ypos;
			event_data.dragging_coordinates.y = xpos;
			event_data.is_dragging = true;
		}
		else if (action == GLFW_RELEASE)
		{
			ApplicationContext::instance().mouse_event_data.is_dragging = false;
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
