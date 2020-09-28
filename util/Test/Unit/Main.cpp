#include "pch.h"
#include "Globals/GlobalsScope.h"
#include "Globals/ProcessGlobals.h"
#include "MainUtilInclude.h"

bool DictPerfTest();

bool EntityTest();
bool FixedIntTest();
bool JsonDeepValue();
bool JsonParserTest();
bool JsonPrintTest();
bool JsonTokenizerTest();
bool ListTest();
bool MapTest();
bool PoolTest();
bool ProcessGlobalsTest();
bool SmallDictTest();
bool SmallDictPersistTest();
bool SmartPtrTest();
bool StringSortTest();
bool StringTest();
bool StringHashTest();
bool ValueTest();
bool VectorTest();

int wmain(int argc, wchar_t *argv[])
{
	assertRetVal(ProcessGlobalsTest(), 1);

	ff::ProcessGlobals globals;
	ff::GlobalsScope globalsScope(globals);
	assertRetVal(globals.IsValid(), 1);

	bool runPerfTests = argc > 1 && !wcscmp(argv[1], L"perf");

	if (runPerfTests)
	{
		assertRetVal(DictPerfTest(), 1);
	}
	else
	{
		assertRetVal(EntityTest(), 1);
		assertRetVal(FixedIntTest(), 1);
		assertRetVal(JsonDeepValue(), 1);
		assertRetVal(JsonParserTest(), 1);
		assertRetVal(JsonPrintTest(), 1);
		assertRetVal(JsonTokenizerTest(), 1);
		assertRetVal(ListTest(), 1);
		assertRetVal(MapTest(), 1);
		assertRetVal(PoolTest(), 1);
		assertRetVal(SmallDictTest(), 1);
		assertRetVal(SmallDictPersistTest(), 1);
		assertRetVal(SmartPtrTest(), 1);
		assertRetVal(StringSortTest(), 1);
		assertRetVal(StringTest(), 1);
		assertRetVal(StringHashTest(), 1);
		assertRetVal(ValueTest(), 1);
		assertRetVal(VectorTest(), 1);
	}

	return 0;
}
