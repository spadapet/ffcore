#pragma once

namespace ff
{
	bool IsValidKey(unsigned int vk);
	Noesis::Key GetKey(unsigned int vk);
	bool IsValidMouseButton(unsigned int vk);
	Noesis::MouseButton GetMouseButton(unsigned int vk);
}
