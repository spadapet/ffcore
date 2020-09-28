#include "pch.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/SavedData.h"
#include "Dict/Dict.h"
#include "Graph/DataBlob.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphShader.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "String/StringUtil.h"
#include "Value/Values.h"
#include "Windows/FileUtil.h"

static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_DEFINES(L"defines");
static ff::StaticString PROP_ENTRY(L"entry");
static ff::StaticString PROP_FILE(L"file");
static ff::StaticString PROP_TARGET(L"target");

class __declspec(uuid("d91a2232-1247-4c34-bbc3-e61e3240e051"))
	GraphShader
	: public ff::ComBase
	, public ff::IGraphShader
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(GraphShader);

	// IGraphShader
	virtual ff::IData* GetData() const override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	ff::ComPtr<ff::IData> _data;
};

BEGIN_INTERFACES(GraphShader)
	HAS_INTERFACE(ff::IGraphShader)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<GraphShader>(L"shader");
	});

class ShaderInclude : public ID3DInclude
{
public:
	ShaderInclude(ff::StringRef basePath);

	// ID3DInclude
	COM_FUNC Open(D3D_INCLUDE_TYPE type, LPCSTR file, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes) override;
	COM_FUNC Close(LPCVOID pData) override;

private:
	ff::String _basePath;
};

ShaderInclude::ShaderInclude(ff::StringRef basePath)
	: _basePath(basePath)
{
}

HRESULT ShaderInclude::Open(D3D_INCLUDE_TYPE type, LPCSTR file, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes)
{
	assertRetVal(file && *file && ppData && pBytes, E_FAIL);

	ff::String fullPath = _basePath;
	ff::AppendPathTail(fullPath, ff::StringFromACP(file));
	fullPath = ff::CanonicalizePath(fullPath);

	ff::ComPtr<ff::IData> data;
	assertRetVal(ff::ReadWholeFile(fullPath, &data), E_FAIL);

	*ppData = new BYTE[data->GetSize()];
	*pBytes = (UINT)data->GetSize();

	::CopyMemory((BYTE*)*ppData, data->GetMem(), data->GetSize());

	return S_OK;
}

HRESULT ShaderInclude::Close(LPCVOID pData)
{
	delete[](BYTE*)pData;

	return S_OK;
}

static ff::ComPtr<ff::IData> CompileShaderString(
	ff::StringRef fileContents,
	ff::StringRef filePath,
	ff::StringRef basePath,
	ff::StringRef entry,
	ff::StringRef target,
	const ff::Map<ff::String, ff::String>& defines,
	bool debug,
	ff::Vector<ff::String>& compilerErrors)
{
	assertRetVal(entry.size() && target.size(), nullptr);

	ff::Vector<char> acpFileContents = ff::StringToACP(fileContents);
	ff::Vector<char> acpFilePath = ff::StringToACP(filePath);
	ff::Vector<char> acpEntry = ff::StringToACP(entry);
	ff::Vector<char> acpTarget = ff::StringToACP(target);

	ff::List<ff::Vector<char>> acpDefines;
	ff::Vector<D3D_SHADER_MACRO> definesVector;

	for (const ff::KeyValue<ff::String, ff::String>& kvp : defines)
	{
		acpDefines.Push(ff::StringToACP(kvp.GetKey()));
		ff::Vector<char>& acpName = *acpDefines.GetLast();

		acpDefines.Push(ff::StringToACP(kvp.GetValue()));
		ff::Vector<char>& acpValue = *acpDefines.GetLast();

		definesVector.Push(D3D_SHADER_MACRO());
		definesVector.GetLast().Name = acpName.Data();
		definesVector.GetLast().Definition = acpValue.Data();
	}

	definesVector.Push(D3D_SHADER_MACRO());
	definesVector.GetLast().Name = "ENTRY";
	definesVector.GetLast().Definition = acpEntry.Data();

	definesVector.Push(D3D_SHADER_MACRO());
	definesVector.GetLast().Name = "TARGET";
	definesVector.GetLast().Definition = acpTarget.Data();

	definesVector.Push(D3D_SHADER_MACRO());
	definesVector.GetLast().Name = "LEVEL9";
	definesVector.GetLast().Definition = "0";

	if (target.find(L"_level_9_1") != ff::INVALID_SIZE)
	{
		definesVector.GetLast().Definition = "1";
	}
	else if (target.find(L"_level_9_2") != ff::INVALID_SIZE)
	{
		definesVector.GetLast().Definition = "2";
	}
	else if (target.find(L"_level_9_3") != ff::INVALID_SIZE)
	{
		definesVector.GetLast().Definition = "3";
	}

	definesVector.Push(D3D_SHADER_MACRO());
	definesVector.GetLast().Name = nullptr;
	definesVector.GetLast().Definition = nullptr;

	ff::ComPtr<ID3DBlob> shaderBlob;
	ff::ComPtr<ID3DBlob> compilerErrorsBlob;
	ShaderInclude shaderInclude(basePath);

	HRESULT hr = ::D3DCompile(
		acpFileContents.ConstData(),
		acpFileContents.Size() - 1,
		acpFilePath.ConstData(),
		definesVector.ConstData(),
		&shaderInclude,
		acpEntry.ConstData(),
		acpTarget.ConstData(),
		debug
		? D3DCOMPILE_DEBUG | D3DCOMPILE_WARNINGS_ARE_ERRORS
		: D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS,
		0, // effect flags
		&shaderBlob,
		&compilerErrorsBlob);

	if (FAILED(hr))
	{
		if (compilerErrorsBlob)
		{
			LPCSTR acpErrors = (LPCSTR)compilerErrorsBlob->GetBufferPointer();
			ff::String errorStrings = ff::StringFromACP(acpErrors);
			compilerErrors = ff::SplitString(errorStrings, ff::String(L"\r\n"), false);
		}

		return nullptr;
	}

	ff::ComPtr<ff::IData> shaderData;
	assertRetVal(ff::CreateDataFromBlob(shaderBlob, &shaderData), nullptr);
	return shaderData;
}

static ff::ComPtr<ff::IData> CompileShaderFile(
	ff::StringRef file,
	ff::StringRef entry,
	ff::StringRef target,
	const ff::Map<ff::String, ff::String>& defines,
	bool debug,
	ff::Vector<ff::String>& compilerErrors)
{
	ff::String fileContents;
	assertRetVal(ff::ReadWholeFile(file, fileContents), false);

	ff::String basePath = file;
	ff::StripPathTail(basePath);

	return ::CompileShaderString(fileContents, file, basePath, entry, target, defines, debug, compilerErrors);
}

GraphShader::GraphShader()
{
}

GraphShader::~GraphShader()
{
}

ff::IData* GraphShader::GetData() const
{
	return _data;
}

bool GraphShader::LoadFromSource(const ff::Dict& dict)
{
	bool debugProp = dict.Get<ff::BoolValue>(ff::RES_DEBUG);
	ff::String fullFile = dict.Get<ff::StringValue>(PROP_FILE);
	ff::String entryProp = dict.Get<ff::StringValue>(PROP_ENTRY, ff::String(L"main"));
	ff::String targetProp = dict.Get<ff::StringValue>(PROP_TARGET);
	assertRetVal(entryProp.size() && targetProp.size(), nullptr);

	ff::Map<ff::String, ff::String> defines;
	ff::ValuePtr definesProp = dict.GetValue(PROP_DEFINES);

	if (definesProp && definesProp->IsType<ff::DictValue>())
	{
		for (ff::StringRef name : definesProp->GetValue<ff::DictValue>().GetAllNames())
		{
			defines.SetKey(name, definesProp->GetValue<ff::DictValue>().Get<ff::StringValue>(name));
		}
	}

	ff::Vector<ff::String> compilerErrors;
	_data = ::CompileShaderFile(fullFile, entryProp, targetProp, defines, debugProp, compilerErrors);

	if (compilerErrors.Size())
	{
		ff::ComPtr<ff::IResourceLoadListener> loadListener;
		if (loadListener.QueryFrom(dict.Get<ff::ObjectValue>(ff::RES_LOAD_LISTENER)))
		{
			for (ff::StringRef error : compilerErrors)
			{
				loadListener->AddError(ff::String::format_new(L"Shader compiler error: %s", error.c_str()));
			}
		}
	}

	assertRetVal(_data, false);
	return _data;
}

bool GraphShader::LoadFromCache(const ff::Dict& dict)
{
	_data = dict.Get<ff::DataValue>(PROP_DATA);
	assertRetVal(_data, false);

	return true;
}

bool GraphShader::SaveToCache(ff::Dict& dict)
{
	dict.Set<ff::DataValue>(PROP_DATA, _data);

	return true;
}
