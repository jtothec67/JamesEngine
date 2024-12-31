#include "Component.h"
#include "Entity.h"
#include "Transform.h"
#include "Core.h"
#include "Input.h"
#include "Keyboard.h"
#include "Mouse.h"

namespace JamesEngine
{

	std::shared_ptr<Entity> Component::GetEntity()
	{
		return mEntity.lock();
	}

	std::shared_ptr<Input> Component::GetInput()
	{
		return GetEntity()->GetCore()->GetInput();
	}

	std::shared_ptr<Keyboard> Component::GetKeyboard()
	{
		return GetEntity()->GetCore()->GetInput()->GetKeyboard();
	}

	std::shared_ptr<Mouse> Component::GetMouse()
	{
		return GetEntity()->GetCore()->GetInput()->GetMouse();
	}

	std::shared_ptr<Transform> Component::GetTransform()
	{
		return GetEntity()->GetComponent<Transform>();
	}

	glm::vec3 Component::GetPosition()
	{
		return GetTransform()->GetPosition();
	}

	void Component::SetPosition(glm::vec3 _position)
	{
		GetTransform()->SetPosition(_position);
	}

	glm::vec3 Component::GetRotation()
	{
		return GetTransform()->GetRotation();
	}

	void Component::SetRotation(glm::vec3 _rotation)
	{
		GetTransform()->SetRotation(_rotation);
	}

	void Component::Tick()
	{
		OnTick();
	}

	void Component::Render()
	{
		OnRender();
	}

}