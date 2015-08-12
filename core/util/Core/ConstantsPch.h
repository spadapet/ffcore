#pragma once

namespace ff
{
	static const double PI_D = 3.1415926535897932384626433832795;
	static const float PI_F = 3.1415926535897932384626433832795f;
	static const double PI2_D = PI_D * 2;
	static const float PI2_F = PI_F * 2;
	static const float RAD_TO_DEG_F = 360.0f / PI2_F;
	static const float DEG_TO_RAD_F = PI2_F / 360.0f;

	// Help iteration with size_t
	const size_t INVALID_SIZE = (size_t)-1;
	const DWORD INVALID_DWORD = (DWORD)-1;
}
