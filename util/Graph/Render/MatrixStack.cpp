#include "pch.h"
#include "Graph/Render/MatrixStack.h"

ff::MatrixStack::MatrixStack()
	: MatrixStack(nullptr)
{
}

ff::MatrixStack::MatrixStack(IMatrixStackOwner* owner)
	: _owner(owner)
{
	_stack.Push(ff::GetIdentityMatrix());
}

const DirectX::XMFLOAT4X4& ff::MatrixStack::GetMatrix() const
{
	return _stack.GetLast();
}

void ff::MatrixStack::PushMatrix()
{
	DirectX::XMFLOAT4X4 matrix = GetMatrix();
	_stack.Push(matrix);
}

void ff::MatrixStack::PopMatrix()
{
	assertRet(_stack.Size() > 1);

	bool change = _stack[_stack.Size() - 1] != _stack[_stack.Size() - 2];
	if (change && _owner)
	{
		_owner->OnMatrixChanging(*this);
	}

	_stack.Pop();

	if (change && _owner)
	{
		_owner->OnMatrixChanging(*this);
	}
}

void ff::MatrixStack::Reset(const DirectX::XMFLOAT4X4* defaultMatrix)
{
	_stack.Resize(1);

	if (defaultMatrix)
	{
		_stack[0] = *defaultMatrix;
	}
}

void ff::MatrixStack::SetMatrix(const DirectX::XMFLOAT4X4& matrix)
{
	if (matrix != GetMatrix())
	{
		if (_owner)
		{
			_owner->OnMatrixChanging(*this);
		}

		if (_stack.Size() == 1)
		{
			PushMatrix();
		}

		_stack.GetLast() = matrix;

		if (_owner)
		{
			_owner->OnMatrixChanged(*this);
		}
	}
}

void ff::MatrixStack::TransformMatrix(const DirectX::XMFLOAT4X4& matrix)
{
	if (matrix != ff::GetIdentityMatrix())
	{
		if (_owner)
		{
			_owner->OnMatrixChanging(*this);
		}

		if (_stack.Size() == 1)
		{
			PushMatrix();
		}

		DirectX::XMFLOAT4X4& myMatrix = _stack.GetLast();
		DirectX::XMStoreFloat4x4(&myMatrix, DirectX::XMLoadFloat4x4(&matrix) * DirectX::XMLoadFloat4x4(&myMatrix));

		if (_owner)
		{
			_owner->OnMatrixChanged(*this);
		}
	}
}
