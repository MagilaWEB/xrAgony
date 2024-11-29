#include "stdafx.h"
#include "xr_effgamma.h"
#include "xrCore/Media/Image.hpp"
#include "xrEngine/xrImage_Resampler.h"

#include <wincodec.h>

using namespace XRay::Media;

#define GAMESAVE_SIZE 128

IC u32 convert(float c)
{
	u32 C = iFloor(c);
	if (C > 255)
		C = 255;
	return C;
}

IC void MouseRayFromPoint(Fvector& direction, int x, int y, Fmatrix& m_CamMat)
{
	int halfwidth = ::IDevice->cast()->dwWidth / 2;
	int halfheight = ::IDevice->cast()->dwHeight / 2;

	Ivector2 point2;
	point2.set(x - halfwidth, halfheight - y);

	float size_y = VIEWPORT_NEAR * tanf(deg2rad(60.f) * 0.5f);
	float size_x = size_y / (::IDevice->cast()->fHeight_2 / ::IDevice->cast()->fWidth_2);

	float r_pt = float(point2.x) * size_x / (float)halfwidth;
	float u_pt = float(point2.y) * size_y / (float)halfheight;

	direction.mul(m_CamMat.k, VIEWPORT_NEAR);
	direction.mad(direction, m_CamMat.j, u_pt);
	direction.mad(direction, m_CamMat.i, r_pt);
	direction.normalize();
}

#define SM_FOR_SEND_WIDTH 640
#define SM_FOR_SEND_HEIGHT 480

void CRender::Screenshot(ScreenshotMode mode, LPCSTR name) {
	ID3DResource* pSrcTexture;
	HW.pBaseRT->GetResource(&pSrcTexture);

	VERIFY(pSrcTexture);

	if (!pSrcTexture)
	{
		Log("! Failed to make a screenshot: couldn't obtain base RT resource");
		return;
	}

	DirectX::ScratchImage image;
	if (FAILED(CaptureTexture(HW.pDevice, HW.pContext, pSrcTexture, image)))
	{
		Log("! Failed to make a screenshot: couldn't capture texture");
		_RELEASE(pSrcTexture);
		return;
	}

	// Save
	switch (mode)
	{
	case IRender::SM_FOR_GAMESAVE:
	{
		// resize
		DirectX::ScratchImage resized;
		auto hr = Resize(*image.GetImage(0, 0, 0), GAMESAVE_SIZE, GAMESAVE_SIZE,
			DirectX::TEX_FILTER_BOX, resized);
		if (FAILED(hr))
			goto _end_;

		// compress
		DirectX::ScratchImage compressed;
		hr = Compress(*resized.GetImage(0, 0, 0), DXGI_FORMAT_BC1_UNORM,
			DirectX::TEX_COMPRESS_DEFAULT | DirectX::TEX_COMPRESS_PARALLEL, 0.0f, compressed);
		if (FAILED(hr))
			goto _end_;

		// save (logical & physical)
		DirectX::Blob saved;
		hr = SaveToDDSMemory(*compressed.GetImage(0, 0, 0), DirectX::DDS_FLAGS_FORCE_DX9_LEGACY, saved);
		if (FAILED(hr))
			goto _end_;

		if (IWriter* fs = FS.w_open(name))
		{
			fs->w(saved.GetBufferPointer(), saved.GetBufferSize());
			FS.w_close(fs);
		}
		break;
	}
	case IRender::SM_NORMAL:
	{
		string64 t_stemp;
		string_path buf;
		xr_sprintf(buf, "ss_%s_%s_(%s).jpg", Core.UserName, timestamp(t_stemp), g_pGameLevel ? g_pGameLevel->name().c_str() : "mainmenu");

		DirectX::Blob saved;
		auto hr = SaveToWICMemory(*image.GetImage(0, 0, 0), DirectX::WIC_FLAGS_NONE, GUID_ContainerFormatJpeg, saved);
		if (SUCCEEDED(hr))
		{
			if (IWriter* fs = FS.w_open("$screenshots$", buf))
			{
				fs->w(saved.GetBufferPointer(), saved.GetBufferSize());
				FS.w_close(fs);
			}
		}

		// hq
		if (strstr(Core.Params, "-ss_tga"))
		{
			xr_sprintf(buf, "ssq_%s_%s_(%s).tga", Core.UserName, timestamp(t_stemp), g_pGameLevel ? g_pGameLevel->name().c_str() : "mainmenu");

			hr = SaveToTGAMemory(*image.GetImage(0, 0, 0), saved);
			if (FAILED(hr))
				goto _end_;

			if (IWriter* fs = FS.w_open("$screenshots$", buf))
			{
				fs->w(saved.GetBufferPointer(), saved.GetBufferSize());
				FS.w_close(fs);
			}
		}
		break;
	}
	case IRender::SM_FOR_LEVELMAP:
	case IRender::SM_FOR_CUBEMAP:
	{
		string_path buf;
		VERIFY(name);
		strconcat(sizeof(buf), buf, name, ".tga");

		ID3DTexture2D* pTex = Target->t_ss_async;
		HW.pContext->CopyResource(pTex, pSrcTexture);

		D3D_MAPPED_TEXTURE2D MappedData;
		HW.pContext->Map(pTex, 0, D3D_MAP_READ, 0, &MappedData);
		// Swap r and b, but don't kill alpha
		{
			u32* pPixel = (u32*)MappedData.pData;
			u32* pEnd = pPixel + (::IDevice->cast()->dwWidth * ::IDevice->cast()->dwHeight);

			for (; pPixel != pEnd; pPixel++)
			{
				u32 p = *pPixel;
				*pPixel = color_argb(color_get_A(p), color_get_B(p), color_get_G(p), color_get_R(p));
			}
		}
		// save
		u32* data = (u32*)xr_malloc(::IDevice->cast()->dwHeight * ::IDevice->cast()->dwHeight * 4);
		imf_Process(data, ::IDevice->cast()->dwHeight, ::IDevice->cast()->dwHeight, (u32*)MappedData.pData, ::IDevice->cast()->dwWidth, ::IDevice->cast()->dwHeight, imf_lanczos3);
		HW.pContext->Unmap(pTex, 0);

		if (IWriter* fs = FS.w_open("$screenshots$", buf))
		{
			XRay::Media::Image img{ ::IDevice->cast()->dwHeight, ::IDevice->cast()->dwHeight, data, ImageFormat::RGBA8 };
			img.SaveTGA(*fs, true);
			FS.w_close(fs);
		}
		xr_free(data);
		break;
	}
	} // switch (mode)

_end_:
	_RELEASE(pSrcTexture);
}

void CRender::ScreenshotAsyncBegin()
{
	VERIFY(!m_bMakeAsyncSS);
	m_bMakeAsyncSS = true;
}

void CRender::ScreenshotAsyncEnd(CMemoryWriter& memory_writer)
{
	VERIFY(!m_bMakeAsyncSS);

	// Don't own. No need to release.
	ID3DTexture2D* pTex = Target->t_ss_async;

	D3D_MAPPED_TEXTURE2D MappedData;

	HW.pContext->Map(pTex, 0, D3D_MAP_READ, 0, &MappedData);

	{
		u32* pPixel = (u32*)MappedData.pData;
		u32* pEnd = pPixel + (::IDevice->cast()->dwWidth * ::IDevice->cast()->dwHeight);

		// Kill alpha and swap r and b.
		for (; pPixel != pEnd; pPixel++)
		{
			u32 p = *pPixel;
			*pPixel = color_xrgb(color_get_B(p), color_get_G(p), color_get_R(p));
		}

		memory_writer.w(&::IDevice->cast()->dwWidth, sizeof(::IDevice->cast()->dwWidth));
		memory_writer.w(&::IDevice->cast()->dwHeight, sizeof(::IDevice->cast()->dwHeight));
		memory_writer.w(MappedData.pData, (::IDevice->cast()->dwWidth * ::IDevice->cast()->dwHeight) * 4);
	}

	HW.pContext->Unmap(pTex, 0);
}

void DoAsyncScreenshot() { RImplementation.Target->DoAsyncScreenshot(); }
