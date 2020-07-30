#pragma once

namespace ff
{
	const double PI_D = 3.1415926535897932384626433832795;
	const float PI_F = 3.1415926535897932384626433832795f;
	const double PI2_D = PI_D * 2;
	const float PI2_F = PI_F * 2;
	const float RAD_TO_DEG_F = 360.0f / PI2_F;
	const float DEG_TO_RAD_F = PI2_F / 360.0f;

	// Help iteration with size_t
	const size_t INVALID_SIZE = (size_t)-1;
	const DWORD INVALID_DWORD = (DWORD)-1;

	const size_t ADVANCES_PER_SECOND = 60;
	const int ADVANCES_PER_SECOND_I = 60;
	const double ADVANCES_PER_SECOND_D = 60.0;
	const float ADVANCES_PER_SECOND_F = 60.0f;
	const double SECONDS_PER_ADVANCE_D = 1.0 / 60.0;
	const float SECONDS_PER_ADVANCE_F = 1.0f / 60.0f;
}
