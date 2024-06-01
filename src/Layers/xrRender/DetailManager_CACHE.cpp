#include "stdafx.h"
#include "DetailManager.h"

void CDetailManager::cache_Initialize()
{
	// Centroid
	cache_cx = 0;
	cache_cz = 0;

	// Initialize cache-grid
	Slot* slt = cache_pool;
	for (u32 i = 0; i < dm_cache_line; i++)
	{
		for (u32 j = 0; j < dm_cache_line; j++, slt++)
		{
			cache[i][j] = slt;
			cache_Task(j, i, slt);
		}
	}

	VERIFY(cache_Validate());

	for (u32 _mz1 = 0; _mz1 < dm_cache1_line; _mz1++)
	{
		for (u32 _mx1 = 0; _mx1 < dm_cache1_line; _mx1++)
		{
			CacheSlot1& MS = cache_level1[_mz1][_mx1];
			for (int _z = 0; _z < dm_cache1_count; _z++)
			{
				for (int _x = 0; _x < dm_cache1_count; _x++)
					MS.slots[_z * dm_cache1_count + _x] = &cache[_mz1 * dm_cache1_count + _z][_mx1 * dm_cache1_count + _x];
			}
		}
	}
}

void CDetailManager::cache_Task(int gx, int gz, Slot* D)
{
	if (!D)
		return;
	int sx = cg2w_X(gx);
	int sz = cg2w_Z(gz);
	DetailSlot& DS = QueryDB(sx, sz);

	D->empty = (DS.id0 == DetailSlot::ID_Empty) &&
		(DS.id1 == DetailSlot::ID_Empty) &&
		(DS.id2 == DetailSlot::ID_Empty) &&
		(DS.id3 == DetailSlot::ID_Empty);

	// Unpacking
	u32 old_type = D->type;
	D->type = stPending;
	D->sx = sx;
	D->sz = sz;

	D->vis.box.vMin.set(sx * dm_slot_size, DS.r_ybase(), sz * dm_slot_size);
	D->vis.box.vMax.set(D->vis.box.vMin.x + dm_slot_size, DS.r_ybase() + DS.r_yheight(), D->vis.box.vMin.z + dm_slot_size);
	D->vis.box.grow(EPS_L);

	for (u32 i = 0; i < dm_obj_in_slot; i++)
	{
		D->G[i].id = DS.r_id(i);
		for (SlotItem* item : D->G[i].items)
			poolSI.destroy(item);
		D->G[i].items.clear();
	}

	if (old_type != stPending)
	{
		VERIFY(stPending == D->type);
		cache_task.push_back(D);
	}
}


BOOL CDetailManager::cache_Validate()
{
	for (u32 z = 0; z < dm_cache_line; z++)
	{
		for (u32 x = 0; x < dm_cache_line; x++)
		{
			int		w_x = cg2w_X(x);
			int		w_z = cg2w_Z(z);
			Slot* D = cache[z][x];

			if (D->sx != w_x || D->sz != w_z)
				return FALSE;
		}
	}
	return TRUE;
}

void CDetailManager::cache_Update(int v_x, int v_z, Fvector& view)
{
	bool bNeedMegaUpdate = (cache_cx != v_x) || (cache_cz != v_z);
	// *****	Cache shift
	while (cache_cx != v_x)
	{
		if (v_x > cache_cx)
		{
			// shift matrix to left
			cache_cx++;
			for (u32 z = 0; z < dm_cache_line; z++)
			{
				Slot* S = cache[z][0];
				for (u32 x = 1; x < dm_cache_line; x++)
					cache[z][x - 1] = cache[z][x];

				cache[z][dm_cache_line - 1] = S;
				cache_Task(dm_cache_line - 1, z, S);
			}
		}
		else
		{
			// shift matrix to right
			cache_cx--;
			for (u32 z = 0; z < dm_cache_line; z++)
			{
				Slot* S = cache[z][dm_cache_line - 1];
				for (u32 x = dm_cache_line - 1; x > 0; x--)
					cache[z][x] = cache[z][x - 1];

				cache[z][0] = S;
				cache_Task(0, z, S);
			}
		}
	}

	while (cache_cz != v_z)
	{
		if (v_z > cache_cz)
		{
			// shift matrix down a bit
			cache_cz++;
			for (u32 x = 0; x < dm_cache_line; x++)
			{
				Slot* S = cache[dm_cache_line - 1][x];
				for (u32 z = dm_cache_line - 1; z > 0; z--)
					cache[z][x] = cache[z - 1][x];

				cache[0][x] = S;
				cache_Task(x, 0, S);
			}
		}
		else
		{
			// shift matrix up
			cache_cz--;
			for (u32 x = 0; x < dm_cache_line; x++)
			{
				Slot* S = cache[0][x];
				for (u32 z = 1; z < dm_cache_line; z++)
					cache[z - 1][x] = cache[z][x];

				cache[dm_cache_line - 1][x] = S;
				cache_Task(x, dm_cache_line - 1, S);
			}
		}
	}

	if (bNeedMegaUpdate)
	{
		for (u32 _mz1 = 0; _mz1 < dm_cache1_line; _mz1++)
		{
			for (u32 _mx1 = 0; _mx1 < dm_cache1_line; _mx1++)
			{
				CacheSlot1& MS = cache_level1[_mz1][_mx1];
				MS.empty = TRUE;
				MS.vis.clear();
				for (u32 _i = 0; _i < (dm_cache1_count * dm_cache1_count); _i++)
				{
					auto local_slot = MS.slots[_i];
					if (local_slot)
					{
						Slot* PS = *local_slot;
						if (PS)
						{
							Slot& S = *PS;
							MS.vis.box.merge(S.vis.box);
							if (!S.empty)
								MS.empty = FALSE;
						}
					}
				}
				MS.vis.box.getsphere(MS.vis.sphere.P, MS.vis.sphere.R);
			}
		}
	}
}

void CDetailManager::spawn_Slots(Fvector& view)
{
	// Task performer
	if (!Device.ActiveMain())
	{
		for (u32 i = 0; i < (u32)ps_r__detail_limit_spawn && !cache_task.empty(); i++)
		{
			if (cache_task.empty())
				break;

			u32 cache_id = 0;
			float cache_distance = flt_max;

			FOR_START(u32, 0, cache_task.size(), id)
			{
				// Gain access to data
				Slot* slot = cache_task[id];
				if (slot)
				{
					VERIFY(stPending == slot->type);

					// Estimate
					Fvector slot_position;
					slot->vis.box.getcenter(slot_position);
					float distance = view.distance_to_sqr(slot_position);

					// Select
					if (distance < cache_distance)
					{
						cache_distance = distance;
						cache_id = id;
					}
				}
			}
			FOR_END

			// Decompress and remove task
			cache_Decompress(cache_task[cache_id]);
			cache_task.erase(cache_task.begin() + cache_id);
		}
	}
}

DetailSlot& CDetailManager::QueryDB(int sx, int sz)
{
	int db_x = sx + dtH.x_offs();
	int db_z = sz + dtH.z_offs();
	if ((db_x >= 0) && (db_x<int(dtH.x_size())) && (db_z >= 0) && (db_z<int(dtH.z_size())))
	{
		return dtSlots[db_z * dtH.x_size() + db_x];
	}

	// Empty slot
	DS_empty.w_id(0, DetailSlot::ID_Empty);
	DS_empty.w_id(1, DetailSlot::ID_Empty);
	DS_empty.w_id(2, DetailSlot::ID_Empty);
	DS_empty.w_id(3, DetailSlot::ID_Empty);
	return DS_empty;
}
