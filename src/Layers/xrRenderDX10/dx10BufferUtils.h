#ifndef dx10BufferUtils_included
#define dx10BufferUtils_included
#pragma once



namespace dx10BufferUtils
{
	u32 GetFVFVertexSize(u32 FVF);
	u32 GetDeclVertexSize(const D3DVERTEXELEMENT9* decl, u32 Stream);
	u32 GetDeclLength(const D3DVERTEXELEMENT9* decl);

	HRESULT CreateVertexBuffer(ID3DVertexBuffer** ppBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
	HRESULT CreateIndexBuffer(ID3DIndexBuffer** ppBuffer, const void* pData, UINT DataSize, bool bImmutable = true);
	HRESULT CreateConstantBuffer(ID3DBuffer** ppBuffer, UINT DataSize);
	void ConvertVertexDeclaration(const xr_vector<D3DVERTEXELEMENT9>& declIn, xr_vector<D3D_INPUT_ELEMENT_DESC>& declOut);
};

#endif //	dx10BufferUtils_included
