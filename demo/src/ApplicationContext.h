#pragma once

#include <GLFW/glfw3.h>

#include <scene/Scene.h>


class ApplicationContext
{
private:
	/**
	* @TODO: Implement proper id generation
	*/
	class IdGenerator
	{
	public:
		uint32_t generateId() { assert(next_id + 1 != 0 && "Overflow detected"); return next_id++; };
	private:
		uint32_t next_id{ 0 };
	};
public:
	struct MouseEventData
	{
		glm::vec2 dragging_coordinates{ 0.0f };
		bool is_dragging{ false };
	};

	static ApplicationContext& instance()
	{
		static ApplicationContext _instance;
		return _instance;
	}

	void init(Scene::Scene* scene, GLFWwindow* window_handler);
	friend void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	friend void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
	friend void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

	float getMouseSensitivity() const { return _mouse_sensitivity; }
	uint32_t generateId() { return _id_generator.generateId(); }
private:
	Scene::Scene* getScene() { return _scene; }

	Scene::Scene* _scene{ nullptr };
	MouseEventData mouse_event_data;
	float _mouse_sensitivity = 0.001f;
	GLFWcursorposfun original_cursor_position_callback{ nullptr };
	GLFWmousebuttonfun original_mouse_button_callback{ nullptr };
	GLFWkeyfun originak_key_callback{ nullptr };
	IdGenerator _id_generator;
};
