#pragma once

namespace ff
{
	class MatrixStack;

	enum class MatrixType
	{
		World,
		Projection,
		Count
	};

	class IMatrixStackOwner
	{
	public:
		virtual void OnMatrixChanging(const MatrixStack& stack) = 0;
		virtual void OnMatrixChanged(const MatrixStack& stack) = 0;
	};

	class MatrixStack
	{
	public:
		UTIL_API MatrixStack();
		UTIL_API MatrixStack(IMatrixStackOwner* owner);

		UTIL_API const DirectX::XMFLOAT4X4& GetMatrix() const;
		UTIL_API void PushMatrix();
		UTIL_API void PopMatrix();
		UTIL_API void Reset(const DirectX::XMFLOAT4X4* defaultMatrix = nullptr); // only should be called by the owner

		UTIL_API void SetMatrix(const DirectX::XMFLOAT4X4& matrix);
		UTIL_API void TransformMatrix(const DirectX::XMFLOAT4X4& matrix);

	private:
		IMatrixStackOwner* _owner;
		ff::Vector<DirectX::XMFLOAT4X4> _stack;
	};
}
