#include "pch.h"
#include "Assets/util-resource.h"
#include "Data/DataWriterReader.h"
#include "Globals/GlobalsScope.h"
#include "Globals/ProcessGlobals.h"
#include "Resource/Resources.h"
#include "Resource/ResourceValue.h"
#include "Value/Values.h"

bool ProcessGlobalsTest()
{
	int done = 0;
	{
		ff::ProcessGlobals program;
		assertRetVal(!program.IsValid(), false);

		ff::GlobalsScope programScope(program);
		assertRetVal(program.IsValid(), false);
		assertRetVal(ff::DidProgramStart(), false);

		assertRetVal(!program.IsShuttingDown(), false);
		assertRetVal(!ff::IsProgramShuttingDown(), false);

		ff::AtProgramShutdown([&done]
		{
			ff::Log::GlobalTrace(L"Shut down\n");
			done++;
		});

		program.AtShutdown([&done]
		{
			ff::Log::GlobalTraceF(L"Shut down %d\n", 2);
			done++;
		});

		const ff::Module *module = program.GetModules().Get(ff::String(L"util"));
		assertRetVal(module->GetName() == L"util", false);
		assertRetVal(module->GetResources()->GetString(ff::String(L"APP_STARTUP")).size(), false);

		ff::SharedResourceValue value = module->GetResources()->GetResource(ff::String(L"Renderer.LineVS"));
		assertRetVal(value != nullptr && value.get()->GetValue() && value.get()->GetValue()->IsType<ff::NullValue>(), false);

		ff::AutoResourceValue autoValue = value;
		assertRetVal(autoValue.Flush()->IsType<ff::ObjectValue>(), false);
	}

	assertRetVal(done == 2, false);

	return true;
}
