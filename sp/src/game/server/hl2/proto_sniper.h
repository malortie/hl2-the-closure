//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


//=========================================================
//=========================================================
class CSniperTarget : public CPointEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS(CSniperTarget, CPointEntity);

	bool KeyValue(const char *szKeyName, const char *szValue);

	string_t m_iszGroupName;
};

//=========================================================
//=========================================================
class CSniperBullet : public CBaseEntity
{
public:
	DECLARE_CLASS(CSniperBullet, CBaseEntity);

	CSniperBullet(void) { Init(); }

	Vector	m_vecDir;

	Vector		m_vecStart;
	Vector		m_vecEnd;

	float	m_flLastThink;
	float	m_SoundTime;
	int		m_AmmoType;
	int		m_PenetratedAmmoType;
	float	m_Speed;
	bool	m_bDirectShot;

	void Precache(void);
	bool IsActive(void) { return m_fActive; }

	bool Start(const Vector &vecOrigin, const Vector &vecTarget, CBaseEntity *pOwner, bool bDirectShot);
	void Stop(void);

	void BulletThink(void);

	void Init(void);

	DECLARE_DATADESC();

private:

	// Only one shot per sniper at a time. If a bullet hasn't
	// hit, the shooter must wait.
	bool	m_fActive;

	// This tracks how many times this single bullet has 
	// struck. This is for penetration, so the bullet can
	// go through things.
	int		m_iImpacts;
};


#define SNIPER_MAX_GROUP_TARGETS	16


//=========================================================
//=========================================================
class CProtoSniper : public CAI_BaseNPC
{
	DECLARE_CLASS(CProtoSniper, CAI_BaseNPC);

public:
	CProtoSniper(void);
	void	Precache(void);
	void	Spawn(void);
	Class_T Classify(void);
	float	MaxYawSpeed(void);
	Vector EyePosition(void);

	void UpdateEfficiency(bool bInPVS)	{ SetEfficiency((GetSleepState() != AISS_AWAKE) ? AIE_DORMANT : AIE_NORMAL); SetMoveEfficiency(AIME_NORMAL); }

	bool IsLaserOn(void) { return m_pBeam != NULL; }

	void Event_Killed(const CTakeDamageInfo &info);
	void Event_KilledOther(CBaseEntity *pVictim, const CTakeDamageInfo &info);
	void UpdateOnRemove(void);
	int OnTakeDamage_Alive(const CTakeDamageInfo &info);
	bool WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions) { return true; }
	int IRelationPriority(CBaseEntity *pTarget);
	bool IsFastSniper();

	bool QuerySeeEntity(CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC = false);

	virtual bool FInViewCone(CBaseEntity *pEntity);

	void StartTask(const Task_t *pTask);
	void RunTask(const Task_t *pTask);
	int RangeAttack1Conditions(float flDot, float flDist);
	bool FireBullet(const Vector &vecTarget, bool bDirectShot);
	float GetBulletSpeed();
	Vector  DesiredBodyTarget(CBaseEntity *pTarget);
	Vector	LeadTarget(CBaseEntity *pTarget);
	CBaseEntity *PickDeadPlayerTarget();

	virtual int SelectSchedule(void);
	virtual int TranslateSchedule(int scheduleType);

	bool KeyValue(const char *szKeyName, const char *szValue);

	void PrescheduleThink(void);

	static const char *pAttackSounds[];

	bool FCanCheckAttacks(void);
	bool FindDecoyObject(void);

	void ScopeGlint();

	int GetSoundInterests(void);
	void OnListened();

	Vector GetBulletOrigin(void);

	virtual int Restore(IRestore &restore);

	virtual void OnScheduleChange(void);

	bool FVisible(CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL);

	bool ShouldNotDistanceCull() { return true; }

	int DrawDebugTextOverlays();

	void NotifyShotMissedTarget();

private:

	bool ShouldSnapShot(void);
	void ClearTargetGroup(void);

	float GetPositionParameter(float flTime, bool fLinear);

	void GetPaintAim(const Vector &vecStart, const Vector &vecGoal, float flParameter, Vector *pProgress);

	bool IsSweepingRandomly(void) { return m_iNumGroupTargets > 0; }

	void ClearOldDecoys(void);
	void AddOldDecoy(CBaseEntity *pDecoy);
	bool HasOldDecoy(CBaseEntity *pDecoy);
	bool FindFrustratedShot(float flNoise);

	bool VerifyShot(CBaseEntity *pTarget);

	void SetSweepTarget(const char *pszTarget);

	// Inputs
	void InputEnableSniper(inputdata_t &inputdata);
	void InputDisableSniper(inputdata_t &inputdata);
	void InputSetDecoyRadius(inputdata_t &inputdata);
	void InputSweepTarget(inputdata_t &inputdata);
	void InputSweepTargetHighestPriority(inputdata_t &inputdata);
	void InputSweepGroupRandomly(inputdata_t &inputdata);
	void InputStopSweeping(inputdata_t &inputdata);
	void InputProtectTarget(inputdata_t &inputdata);

#if HL2_EPISODIC
	void InputSetPaintInterval(inputdata_t &inputdata);
	void InputSetPaintIntervalVariance(inputdata_t &inputdata);
#endif

	void LaserOff(void);
	void LaserOn(const Vector &vecTarget, const Vector &vecDeviance);

	void PaintTarget(const Vector &vecTarget, float flPaintTime);

	bool IsPlayerAllySniper();

private:

	/// This is the variable from which m_flPaintTime gets set.
	/// How long to aim at someone before shooting them.
	float m_flKeyfieldPaintTime;

	/// A random number from 0 to this is added to m_flKeyfieldPaintTime
	/// to yield m_flPaintTime's initial delay.
	float m_flKeyfieldPaintTimeNoise;

	// This keeps track of the last spot the laser painted. For 
	// continuous sweeping that changes direction.
	Vector m_vecPaintCursor;
	float  m_flPaintTime;

	bool						m_fWeaponLoaded;
	bool						m_fEnabled;
	bool						m_fIsPatient;
	float						m_flPatience;
	int							m_iMisses;
	EHANDLE						m_hDecoyObject;
	EHANDLE						m_hSweepTarget;
	Vector						m_vecDecoyObjectTarget;
	Vector						m_vecFrustratedTarget;
	Vector						m_vecPaintStart; // used to track where a sweep starts for the purpose of interpolating.

	float						m_flFrustration;

	float						m_flThinkInterval;

	float						m_flDecoyRadius;

	CBeam						*m_pBeam;

	bool m_fSnapShot;

	int							m_iNumGroupTargets;
	CBaseEntity					*m_pGroupTarget[SNIPER_MAX_GROUP_TARGETS];

	bool						m_bSweepHighestPriority; // My hack :[ (sjb)
	int							m_iBeamBrightness;

	// bullet stopping energy shield effect.
	float						m_flShieldDist;
	float						m_flShieldRadius;

	float						m_flTimeLastAttackedPlayer;

	// Protection
	EHANDLE						m_hProtectTarget;			// Entity that this sniper is supposed to protect
	float						m_flDangerEnemyDistance;	// Distance to the enemy nearest the protect target

	// Have I warned the target that I'm pointing my laser at them?
	bool						m_bWarnedTargetEntity;

	float						m_flTimeLastShotMissed;
	bool						m_bKilledPlayer;
	bool						m_bShootZombiesInChest;		///< if true, do not try to shoot zombies in the headcrab

	COutputEvent				m_OnShotFired;

	DEFINE_CUSTOM_AI;

	DECLARE_DATADESC();
};
