#pragma once

#ifdef RENDER
struct IPhysicsShell;
struct IPhysicsElement;
#else
class IPhysicsShell;
class IPhysicsElement;
#endif
xr_pure_interface IObjectPhysicsCollision
{
public:
	virtual const IPhysicsShell* physics_shell() const = 0;
	virtual const IPhysicsElement* physics_character() const = 0; // depricated
};
