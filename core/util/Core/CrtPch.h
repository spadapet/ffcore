#pragma once

// C-Runtime includes
// Precompiled header only

#include <array>
#include <atomic>
#include <forward_list>
#include <functional>
#include <iostream>
#include <regex>
#include <type_traits>
#include <typeindex>
#include <typeinfo>

#define MAKE_POD(name) \
	template<> struct std::is_trivially_constructible<name> { static const bool value = true; }; \
	template<> struct std::is_trivially_copy_constructible<name> { static const bool value = true; }; \
	template<> struct std::is_trivially_move_constructible<name> { static const bool value = true; }; \
	template<> struct std::is_trivially_default_constructible<name> { static const bool value = true; }; \
	template<> struct std::is_trivial<name> { static const bool value = true; }; \
	template<> struct std::is_destructible<name> { static const bool value = false; };
