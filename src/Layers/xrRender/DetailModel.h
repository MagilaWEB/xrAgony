#ifndef DetailModelH
#define DetailModelH
#pragma once

#include "IRenderDetailModel.h"

class ECORE_API CDetail : public IRender_DetailModel
{
public:
	size_t vOffset{ 0 };
	size_t iOffset{ 0 };
	void Load(IReader* S);
	void Optimize();
	virtual void Unload();

	virtual void transfer(Fmatrix& mXform, fvfVertexOut* vDest, u32 C, u16* iDest, u32 iOffset);
	virtual void transfer(Fmatrix& mXform, fvfVertexOut* vDest, u32 C, u16* iDest, u32 iOffset, float du, float dv);
	virtual ~CDetail();
};
#endif
