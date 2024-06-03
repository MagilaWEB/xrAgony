///////////////////////////////////////////////////////////////
// InfoDocument.h
// InfoDocument - документ, содержащий сюжетную информацию
///////////////////////////////////////////////////////////////

#pragma once

#include "inventory_item_object.h"
#include "InfoPortionDefs.h"

class CInfoDocument : public CInventoryItemObject
{
private:
	typedef CInventoryItemObject inherited;

public:
	CInfoDocument(void) = default;

	virtual BOOL net_Spawn(CSE_Abstract* DC) override;
	virtual void Load(LPCSTR section) override;
	virtual void net_Destroy() override;
	virtual void shedule_Update(u32 dt) override;
	virtual void UpdateCL() override;
	virtual void renderable_Render() override;

	virtual void OnH_A_Chield() override;
	virtual void OnH_B_Independent(bool just_before_destroy) override;

protected:
	//индекс информации, содержащейся в документе
	shared_str m_Info;
};
