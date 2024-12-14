#ifndef RenderVisual_included
#define RenderVisual_included
#pragma once

class IKinematics;
class IKinematicsAnimated;
class IParticleCustom;
struct vis_data;

class IRenderVisual
{
public:
	virtual ~IRenderVisual() { ; }
	virtual vis_data& getVisData() = 0;
	virtual u32 getType() = 0;

#ifdef DEBUG
	virtual shared_str getDebugName() = 0;
#endif

	virtual IRenderVisual* getSubModel(u8 idx) { return nullptr; } //--#SM+#--
	virtual IKinematics* dcast_PKinematics() { return nullptr; }
	virtual IKinematicsAnimated* dcast_PKinematicsAnimated() { return nullptr; }
	virtual IParticleCustom* dcast_ParticleCustom() { return nullptr; }

	virtual fastdelegate::FastDelegate<void(float dist)>& get_callback_dist() = 0;
	virtual void update_distance_to_camera(Fmatrix* transform_matrix = nullptr) = 0;
	virtual float get_distance_to_camera_base() const = 0;
	virtual float getDistanceToCamera() const = 0;
};

#endif //	RenderVisual_included
