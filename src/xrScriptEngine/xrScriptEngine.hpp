#pragma once

#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4995)
#pragma warning(disable : 4267)
#pragma warning(disable : 4100) // unreferenced formal parameter

#include "lua.hpp"

#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4456) // declaration of 'x' hides previous local declaration
#pragma warning(disable : 4458) // declaration of 'x' hides class member
#pragma warning(disable : 4459) // declaration of 'x' hides global declaration
#pragma warning(disable : 4913) // user defined binary operator 'x' exists but no overload could convert all operands
#pragma warning(disable : 4297) // function assumed not to throw exception but does
// XXX: define LUABIND_DYNAMIC_LINK in engine config header
#include <luabind/luabind.hpp>
#include <luabind/class.hpp>
#include <luabind/object.hpp>
#include <luabind/operator.hpp>
#include <luabind/adopt_policy.hpp>
#include <luabind/return_reference_to_policy.hpp>
#include <luabind/out_value_policy.hpp>
#include <luabind/iterator_policy.hpp>

#pragma warning(pop)