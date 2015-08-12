#include "pch.h"
#include "Data/Data.h"

bool SmartPtrTest()
{
	ff::ComPtr<ff::IDataVector> data1;
	ff::CreateDataVector(0, &data1);

	ff::SmartPtr<ff::IDataVector> data2;
	ff::CreateDataVector(0, &data2);

	assertRetVal(data1 != data2, false);
	assertRetVal(data2 != data1, false);

	data1 = data2;

	assertRetVal(data1 == data2, false);
	assertRetVal(data2 == data1, false);

	data2 = nullptr;

	assertRetVal(data1 != data2, false);
	assertRetVal(data2 != data1, false);

	data1.Release();

	assertRetVal(data1 == data2, false);
	assertRetVal(data2 == data1, false);

	ff::CreateDataVector(0, &data1);
	ff::CreateDataVector(0, &data2);

	ff::ComPtr<ff::IDataVector> data3 = data1;
	ff::ComPtr<ff::IDataVector> data4 = std::move(ff::SmartPtr<ff::IDataVector>(data1));
	ff::ComPtr<ff::IDataVector> data5 = ff::ComPtr<ff::IDataVector>(data2);
	ff::ComPtr<ff::IDataVector> data6 = ff::ComPtr<ff::IDataVector>(data1.Object());

	data3 = data4;
	data5 = data6;

	std::swap(data3, data5);
	assertRetVal(data3 == data6, false);
	assertRetVal(data4 == data5, false);

	return true;
}
