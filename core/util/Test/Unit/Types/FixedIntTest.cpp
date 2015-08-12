#include "pch.h"

bool FixedIntTest()
{
	assertRetVal(ff::FixedInt(3.25) + ff::FixedInt(5.75) == ff::FixedInt(9), false);
	assertRetVal(ff::FixedInt(9) - ff::FixedInt(5.75) == ff::FixedInt(3.25), false);
	assertRetVal(ff::FixedInt(3.25) - ff::FixedInt(5.75) == ff::FixedInt(-2.5), false);

	assertRetVal(ff::FixedInt(3) * ff::FixedInt(5) == ff::FixedInt(15), false);
	assertRetVal(ff::FixedInt(3.5) * ff::FixedInt(5) == ff::FixedInt(17.5), false);
	assertRetVal(ff::FixedInt(17.5) / ff::FixedInt(3.5) == ff::FixedInt(5), false);

	assertRetVal(ff::FixedInt(-3) * ff::FixedInt(-5) == ff::FixedInt(15), false);
	assertRetVal(ff::FixedInt(-3.5) * ff::FixedInt(-5) == ff::FixedInt(17.5), false);
	assertRetVal(ff::FixedInt(-17.5) / ff::FixedInt(-3.5) == ff::FixedInt(5), false);

	assertRetVal(ff::FixedInt(-3) * ff::FixedInt(5) == ff::FixedInt(-15), false);
	assertRetVal(ff::FixedInt(-3.5) * ff::FixedInt(5) == ff::FixedInt(-17.5), false);
	assertRetVal(ff::FixedInt(-17.5) / ff::FixedInt(3.5) == ff::FixedInt(-5), false);

	assertRetVal(ff::FixedInt(-2.125) + ff::FixedInt(0.125) == ff::FixedInt(-2), false);
	assertRetVal(ff::FixedInt(-2.125) - ff::FixedInt(0.125) == ff::FixedInt(-2.25), false);

	ff::FixedInt i(4.75);
	assertRetVal(i == ff::FixedInt(4.75), false);
	assertRetVal((i += .25) == ff::FixedInt(5), false);
	assertRetVal((i -= .75) == ff::FixedInt(4.25), false);
	assertRetVal((i -= 8) == ff::FixedInt(-3.75), false);
	assertRetVal((i -= .25) == ff::FixedInt(-4), false);

	assertRetVal(ff::FixedInt(22.5) / 5 == ff::FixedInt(4.5), false);
	assertRetVal(ff::FixedInt(22.5) * 5 == ff::FixedInt(112.5), false);

	assertRetVal(ff::FixedInt(2.3).abs() == ff::FixedInt(2.3), false);
	assertRetVal(ff::FixedInt(-2.3).abs() == ff::FixedInt(2.3), false);
	assertRetVal(ff::FixedInt(2.3).floor() == ff::FixedInt(2), false);
	assertRetVal(ff::FixedInt(2.3).ceil() == ff::FixedInt(3), false);
	assertRetVal(ff::FixedInt(2.3).trunc() == ff::FixedInt(2), false);
	assertRetVal(ff::FixedInt(-2.3).floor() == ff::FixedInt(-3), false);
	assertRetVal(ff::FixedInt(-2.3).ceil() == ff::FixedInt(-2), false);
	assertRetVal(ff::FixedInt(-2.3).trunc() == ff::FixedInt(-2), false);

	assertRetVal(std::max(ff::FixedInt(70), ff::FixedInt(-70)) == ff::FixedInt(70), false);
	assertRetVal(std::min(ff::FixedInt(70), ff::FixedInt(-70)) == ff::FixedInt(-70), false);

	return true;
}
