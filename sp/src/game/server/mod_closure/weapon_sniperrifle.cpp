//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "ai_basenpc.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "gamestats.h"
#include "rumble_shared.h"
#include "beam_shared.h"
#include "proto_sniper.h"
#include "hl2_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeaponSniperrifle
//-----------------------------------------------------------------------------

class CWeaponSniperrifle : public CBaseHLCombatWeapon
{
	DECLARE_CLASS(CWeaponSniperrifle, CBaseHLCombatWeapon);
public:

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CWeaponSniperrifle(void);

	virtual void Spawn(void);
	virtual void Precache(void);

	void	PrimaryAttack(void);
	void	SecondaryAttack(void);
	void	Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);

	virtual bool	Deploy(void);
	virtual bool	Holster(CBaseCombatWeapon *pSwitchingTo = NULL);
	virtual bool	Reload(void);
	virtual void	ItemPostFrame(void);
	virtual void	ItemBusyFrame(void);

	float	WeaponAutoAimScale()	{ return 0.6f; }

	virtual bool			IsWeaponZoomed() { return m_bInZoom; }

	bool	ShouldDisplayHUDHint() { return true; }

private:

	void	CheckZoomToggle(void);
	void	ToggleZoom(void);

	void CheckLaserBeam(void);
	bool IsLaserOn(void) { return m_pBeam != NULL; }

	void LaserOff(void);
	void LaserOn(void);
	void SetLaserBrightness(int brightness);
	void ToggleLaser(void);

	void CreateLaser(void);
	void DestroyLaser(void);
	void UpdateLaser(void);

	CNetworkVar(bool, m_bInZoom);
	bool		m_bWasZoomed;
	bool		m_bBeamWasOn;
	CBeam*		m_pBeam;

	short		m_sFlashSprite;
	short		m_sHaloSprite;
	int			m_iBeamAttachment;
	int			m_iBeamBrightness;
	float		m_flNextAttack3Time;
};

LINK_ENTITY_TO_CLASS(weapon_sniperrifle, CWeaponSniperrifle);

PRECACHE_WEAPON_REGISTER(weapon_sniperrifle);

IMPLEMENT_SERVERCLASS_ST(CWeaponSniperrifle, DT_WeaponSniperrifle)
	SendPropBool(SENDINFO(m_bInZoom)),
END_SEND_TABLE()

BEGIN_DATADESC(CWeaponSniperrifle)
	DEFINE_FIELD(m_bInZoom, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bWasZoomed, FIELD_BOOLEAN),
	DEFINE_FIELD(m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(m_bBeamWasOn, FIELD_BOOLEAN),
	DEFINE_FIELD(m_sFlashSprite, FIELD_SHORT),
	DEFINE_FIELD(m_sHaloSprite, FIELD_SHORT),
	DEFINE_FIELD(m_iBeamAttachment, FIELD_INTEGER),
	DEFINE_FIELD(m_iBeamBrightness, FIELD_INTEGER),
	DEFINE_FIELD(m_flNextAttack3Time, FIELD_TIME),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponSniperrifle::CWeaponSniperrifle(void)
{
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;

	m_bInZoom = false;
	m_bWasZoomed = false;
	m_bBeamWasOn = false;
	m_pBeam = NULL;

	m_iBeamAttachment = 2;
	m_iBeamBrightness = 255;
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::Spawn(void)
{
	BaseClass::Spawn();

	m_iBeamAttachment = 2;
	m_iBeamBrightness = 255;

	m_flNextAttack3Time = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::Precache(void)
{
	m_sHaloSprite = PrecacheModel("sprites/light_glow03.vmt");
	m_sFlashSprite = PrecacheModel("sprites/muzzleflash1.vmt");
	PrecacheModel("effects/bluelaser1.vmt");

	UTIL_PrecacheOther("sniperbullet");

	PrecacheScriptSound("NPC_Sniper.TargetDestroyed");
	PrecacheScriptSound("NPC_Sniper.HearDanger");
	PrecacheScriptSound("NPC_Sniper.FireBullet");
	PrecacheScriptSound("NPC_Sniper.Reload");
	PrecacheScriptSound("NPC_Sniper.SonicBoom");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	switch (pEvent->event)
	{
	case EVENT_WEAPON_RELOAD:
	{
		CEffectData data;

		// Emit six spent shells
		for (int i = 0; i < 6; i++)
		{
			data.m_vOrigin = pOwner->WorldSpaceCenter() + RandomVector(-4, 4);
			data.m_vAngles = QAngle(90, random->RandomInt(0, 360), 0);
			data.m_nEntIndex = entindex();

			DispatchEffect("ShellEject", data);
		}

		break;
	}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::PrimaryAttack(void)
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	if (m_iClip1 <= 0)
	{
		if (!m_bFireOnEmpty)
		{
			Reload();
		}
		else
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());


	WeaponSound(SINGLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	m_iClip1--;

	pPlayer->RumbleEffect(RUMBLE_357, 0, RUMBLE_FLAG_RESTART);

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(m_bInZoom ? 0 : AUTOAIM_SCALE_DEFAULT);

	pPlayer->FireBullets(1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0);

	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);

	pPlayer->ViewPunch(QAngle(random->RandomFloat(-8, -10), 0, 0));

	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner());

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	trace_t tr;
	UTIL_TraceLine(vecSrc, vecSrc + (vecAiming * MAX_TRACE_LENGTH), MASK_ALL, this, COLLISION_GROUP_NONE, &tr);

	// Create the sniper bullet.
	CSniperBullet* pBullet = (CSniperBullet *)Create("sniperbullet", vecSrc, GetLocalAngles(), NULL);

	Assert(pBullet != NULL);

	if (!pBullet->Start(tr.startpos, tr.endpos, this, true))
	{
		// Bullet must still be active.
		return;
	}

	pBullet->SetOwnerEntity(this);

	CPASAttenuationFilter filternoatten(this, ATTN_NONE);
	EmitSound(filternoatten, entindex(), "NPC_Sniper.FireBullet");

	CPVSFilter filter(tr.startpos);
	te->Sprite(filter, 0.0, &tr.startpos, m_sFlashSprite, 0.3, 255);

	SetLaserBrightness(0);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::SecondaryAttack(void)
{
	CBasePlayer*pPlayer = ToBasePlayer(GetOwner());

	// Send scope message to display the hud.
	CSingleUserRecipientFilter user(pPlayer);
	user.MakeReliable();
	UserMessageBegin(user, "Scope");
		WRITE_BYTE(m_bInZoom);
	MessageEnd();

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponSniperrifle::Reload(void)
{
	bool fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);
	if (fRet)
	{
		if (IsLaserOn())
		{
			LaserOff();
			m_bBeamWasOn = true;
		}

		if (m_bInZoom)
		{
			ToggleZoom();

			m_bWasZoomed = true;
		}

		SetLaserBrightness(0);

		WeaponSound(RELOAD);

		CPASAttenuationFilter filternoatten(this, ATTN_NONE);
		EmitSound(filternoatten, entindex(), "Weapon_Sniperrifle.Reload");

	}

	return fRet;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponSniperrifle::Deploy(void)
{
	bool fRet = DefaultDeploy((char*)GetViewModel(), (char*)GetWorldModel(), GetDrawActivity(), (char*)GetAnimPrefix());

	if (fRet)
	{
		WeaponSound( DEPLOY );

		m_bBeamWasOn = false;

		m_bWasZoomed = false;
	}

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponSniperrifle::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	bool fRet = BaseClass::Holster(pSwitchingTo);

	if (fRet)
	{
		LaserOff();

		m_bBeamWasOn = false;

		m_bWasZoomed = false;

		if (m_bInZoom)
		{
			ToggleZoom();
		}
	}

	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::ItemBusyFrame(void)
{
	// Allow zoom toggling even when we're reloading
	CheckZoomToggle();

	CheckLaserBeam();

	UpdateLaser();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::ItemPostFrame(void)
{
	// Allow zoom toggling
	CheckZoomToggle();

	CheckLaserBeam();

	UpdateLaser();

	BaseClass::ItemPostFrame();

	CBasePlayer* pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	if (pPlayer->m_nButtons & IN_ATTACK3 && (m_flNextAttack3Time < gpGlobals->curtime))
	{
		ToggleLaser();

		m_flNextAttack3Time = gpGlobals->curtime + 0.5f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::ToggleZoom(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	if (m_bInZoom)
	{
		if (pPlayer->SetFOV(this, 0, 0.2f))
		{
			m_bInZoom = false;
		}
	}
	else
	{
		if (pPlayer->SetFOV(this, 20, 0.1f))
		{
			m_bInZoom = true;
		}
	}

	WeaponSound(SPECIAL1);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::CheckZoomToggle(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer->m_afButtonPressed & IN_ATTACK2)
	{
		ToggleZoom();
	}

	if (m_bWasZoomed && (m_flNextPrimaryAttack < gpGlobals->curtime))
	{
		ToggleZoom();

		m_bWasZoomed = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::CheckLaserBeam(void)
{
	if (m_bBeamWasOn)
	{
		LaserOn();
		m_bBeamWasOn = false;
	}

	if (m_flNextPrimaryAttack < gpGlobals->curtime)
	{
		SetLaserBrightness(255);
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::LaserOff(void)
{
	if (m_pBeam)
	{
		UTIL_Remove(m_pBeam);
		m_pBeam = NULL;
	}

	SetNextThink(gpGlobals->curtime + 0.1f);

	WeaponSound(SPECIAL3);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define LASER_LEAD_DIST	64
void CWeaponSniperrifle::LaserOn()
{
	if (!m_pBeam)
	{
		m_pBeam = CBeam::BeamCreate("effects/bluelaser1.vmt", 1.0f);
		m_pBeam->SetColor(0, 100, 255);
	}
	else
	{
		// Beam seems to be on.
		return;
	}

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	Vector vecOrigin, forward;

	vecOrigin = GetOwner()->Weapon_ShootPosition();
	pPlayer->EyeVectors(&forward, NULL, NULL);

	trace_t tr;
	UTIL_TraceLine(vecOrigin, vecOrigin + (forward * MAX_TRACE_LENGTH), MASK_ALL, this, COLLISION_GROUP_NONE, &tr);

	// The beam is backwards, sortof. The endpoint is the sniper. This is
	// so that the beam can be tapered to very thin where it emits from the sniper.
	m_pBeam->PointEntInit(tr.endpos, this);
	m_pBeam->SetEndAttachment(m_iBeamAttachment);
	m_pBeam->SetBrightness(255);
	m_pBeam->SetNoise(0);
	m_pBeam->SetWidth(1.0f);
	m_pBeam->SetEndWidth(0);
	m_pBeam->SetScrollRate(0);
	m_pBeam->SetFadeLength(0);
	m_pBeam->SetHaloTexture(m_sHaloSprite);
	m_pBeam->SetHaloScale(4.0f);

	// Think faster whilst painting. Higher resolution on the 
	// beam movement.
	SetNextThink(gpGlobals->curtime + 0.02);

	WeaponSound(SPECIAL2);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::SetLaserBrightness(int brightness)
{
	m_iBeamBrightness = brightness;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::ToggleLaser(void)
{
	if (IsLaserOn())
		LaserOff();
	else
		LaserOn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSniperrifle::UpdateLaser()
{
	if (!m_pBeam)
		return;

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	Vector vecOrigin, forward;

	vecOrigin = GetOwner()->Weapon_ShootPosition();
	pPlayer->EyeVectors(&forward, NULL, NULL);

	trace_t tr;
	UTIL_TraceLine(vecOrigin, vecOrigin + (forward * MAX_TRACE_LENGTH), MASK_ALL, this, COLLISION_GROUP_NONE, &tr);

	m_pBeam->SetStartPos(tr.endpos);
	m_pBeam->SetBrightness(m_iBeamBrightness);

	if (m_bInZoom)
	{
		vecOrigin.z -= 4;

		m_pBeam->SetType(BEAM_POINTS);
		m_pBeam->SetEndAttachment(0);
		m_pBeam->SetEndPos(vecOrigin);
	}
	else
	{
		m_pBeam->SetType(BEAM_ENTPOINT);
		m_pBeam->SetEndAttachment(m_iBeamAttachment);
	}

	m_pBeam->RelinkBeam();
}