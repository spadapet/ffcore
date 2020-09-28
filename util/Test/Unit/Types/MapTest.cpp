#include "pch.h"

bool MapTest()
{
	ff::Map<ff::String, int> table;
	std::array<wchar_t, 256> buf;

	for (int i = 0; i < 1000; i++)
	{
		_snwprintf_s(buf.data(), buf.size(), _TRUNCATE, L"%d", i * 17);
		ff::String str(buf.data());
		
		table.SetKey(str, i);
		table.InsertKey(ff::String(str), -i);
	}

	for (int i = 0; i < 1000; i++)
	{
		_snwprintf_s(buf.data(), buf.size(), _TRUNCATE, L"%d", i * 17);
		ff::String str(buf.data());
		
		auto pos = table.GetKey(str);
		assertRetVal(pos, false);
		assertRetVal(std::abs(pos->GetValue()) == i, false);

		pos = table.GetNextDupeKey(*pos);
		assertRetVal(pos, false);
		assertRetVal(std::abs(pos->GetValue()) == i, false);

		pos = table.GetNextDupeKey(*pos);
		assertRetVal(!pos, false);
	}

	assertRetVal(table.Size() == 2000, false);
	int count = 0;

	for (auto pos : table)
	{
		int i = std::abs(pos.GetValue());
		_snwprintf_s(buf.data(), buf.size(), _TRUNCATE, L"%d", i * 17);

		assertRetVal(pos.GetKey() == buf.data(), false);

		count++;
	}

	assertRetVal(count == 2000, false);

	for (const auto &iter: table)
	{
		int i = std::abs(iter.GetValue());
		_snwprintf_s(buf.data(), buf.size(), _TRUNCATE, L"%d", i * 17);

		assertRetVal(iter.GetKey() == buf.data(), false);
	}

	return true;
}
