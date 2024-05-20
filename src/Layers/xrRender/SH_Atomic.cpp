#include "stdafx.h"
#pragma hdrstop

#include "sh_atomic.h"
#include "ResourceManager.h"

// Atomic
//SVS::~SVS()
//{
//	_RELEASE(vs);
//	dxRenderDeviceRender::Instance().Resources->_DeleteVS(this);
//}
//SPS::~SPS()
//{
//	 _RELEASE(ps);
//	 dxRenderDeviceRender::Instance().Resources->_DeletePS(this);
//}
//SState::~SState()
//{
//	_RELEASE(state);
//	dxRenderDeviceRender::Instance().Resources->_DeleteState(this);
//}
//SDeclaration::~SDeclaration()
//{
//	_RELEASE(dcl);
//	dxRenderDeviceRender::Instance().Resources->_DeleteDecl(this);
//}

///////////////////////////////////////////////////////////////////////
//  SVS
SVS::SVS() : sh(0)
{}

SVS::~SVS()
{
	RImplementation.Resources->_DeleteVS(this);

	_RELEASE(sh);
}

///////////////////////////////////////////////////////////////////////
// SPS
SPS::~SPS()
{
	_RELEASE(sh);
	RImplementation.Resources->_DeletePS(this);
}

#if defined(USE_DX11)
///////////////////////////////////////////////////////////////////////
// SGS
SGS::~SGS()
{
	_RELEASE(sh);
	RImplementation.Resources->_DeleteGS(this);
}

SHS::~SHS()
{
	_RELEASE(sh);

	RImplementation.Resources->_DeleteHS(this);
}

SDS::~SDS()
{
	_RELEASE(sh);

	RImplementation.Resources->_DeleteDS(this);
}

SCS::~SCS()
{
	_RELEASE(sh);

	RImplementation.Resources->_DeleteCS(this);
}

///////////////////////////////////////////////////////////////////////
//	SInputSignature
SInputSignature::SInputSignature(ID3DBlob* pBlob)
{
	VERIFY(pBlob);
	signature = pBlob;
	signature->AddRef();
};

SInputSignature::~SInputSignature()
{
	_RELEASE(signature);
	RImplementation.Resources->_DeleteInputSignature(this);
}
#endif

///////////////////////////////////////////////////////////////////////
//	SState
SState::~SState()
{
	_RELEASE(state);
	RImplementation.Resources->_DeleteState(this);
}

///////////////////////////////////////////////////////////////////////
//	SDeclaration
SDeclaration::~SDeclaration()
{
	RImplementation.Resources->_DeleteDecl(this);
#if defined(USE_DX11)
	xr_map<ID3DBlob*, ID3DInputLayout*>::iterator iLayout;
	iLayout = vs_to_layout.begin();
	for (; iLayout != vs_to_layout.end(); ++iLayout)
	{
		//	Release vertex layout
		_RELEASE(iLayout->second);
	}
#else
	//	Release vertex layout
	_RELEASE(dcl);
#endif
}
