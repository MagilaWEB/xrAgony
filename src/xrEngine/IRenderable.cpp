#include "stdafx.h"
#include "xrCDB/ISpatial.h"
#include "IRenderable.h"

// XXX: rename this file to RenderableBase.cpp
RenderableBase::RenderableBase()
{
	renderable.xform.identity();
	renderable.visual = nullptr;
	renderable.pROS = nullptr;
	renderable.pROS_Allowed = TRUE;
	ISpatial* self = dynamic_cast<ISpatial*>(this);
	if (self)
		self->GetSpatialData().type |= STYPE_RENDERABLE;
}

extern ENGINE_API BOOL g_bRendering;
RenderableBase::~RenderableBase()
{
	VERIFY(!g_bRendering);
	::Render->model_Delete(renderable.visual);
	if (renderable.pROS)
		::Render->ros_destroy(renderable.pROS);
	renderable.visual = nullptr;
	renderable.pROS = nullptr;
}

IRender_ObjectSpecific* RenderableBase::renderable_ROS()
{
	if (0 == renderable.pROS && renderable.pROS_Allowed)
		renderable.pROS = ::Render->ros_create(this);
	return renderable.pROS;
}