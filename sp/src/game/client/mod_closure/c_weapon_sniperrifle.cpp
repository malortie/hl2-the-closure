//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_weapon__stubs.h"
#include "c_basehlcombatweapon.h"
#include "fx.h"
#include "particles_localspace.h"
#include "view.h"
#include "particles_attractor.h"

class C_WeaponSniperrifle : public C_BaseHLCombatWeapon
{
	DECLARE_CLASS(C_WeaponSniperrifle, C_BaseHLCombatWeapon);
public:
	C_WeaponSniperrifle(void);

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	virtual void OnDataChanged(DataUpdateType_t updateType);
	virtual void ClientThink(void);

	virtual bool ShouldUseLargeViewModelVROverride() OVERRIDE{ return true; }

private:
	bool m_bInZoom;
};

STUB_WEAPON_CLASS_IMPLEMENT(weapon_sniperrifle, C_WeaponSniperrifle);

IMPLEMENT_CLIENTCLASS_DT(C_WeaponSniperrifle, DT_WeaponSniperrifle, CWeaponSniperrifle)
	RecvPropBool(RECVINFO(m_bInZoom)),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
C_WeaponSniperrifle::C_WeaponSniperrifle(void)
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_WeaponSniperrifle::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
	SetNextClientThink(CLIENT_THINK_ALWAYS);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_WeaponSniperrifle::ClientThink(void)
{
	return BaseClass::ClientThink();
}