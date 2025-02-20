////////////////////////////////////////////////////////////////////////////
//	Module 		: object_item_client_server.h
//	Created 	: 27.05.2004
//  Modified 	: 30.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Object item client and server class
////////////////////////////////////////////////////////////////////////////

#ifndef object_item_client_serverH
#define object_item_client_serverH

#pragma once

#include "object_factory_space.h"
#include "object_item_abstract.h"
#include "object_factory.h"

template <typename _client_type, typename _server_type>
class CObjectItemClientServer : public CObjectItemAbstract
{
protected:
	typedef CObjectItemAbstract inherited;
	typedef _client_type CLIENT_TYPE;
	typedef _server_type SERVER_TYPE;

public:
	IC CObjectItemClientServer(const CLASS_ID& clsid, LPCSTR script_clsid);
#ifndef NO_XR_GAME
	virtual ObjectFactory::ClientObjectBaseClass* client_object() const;
#endif
	virtual ObjectFactory::ServerObjectBaseClass* server_object(LPCSTR section) const;
};

#include "object_item_client_server_inline.h"

#endif
