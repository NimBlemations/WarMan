#ifndef GAME_HUD_MODEL_WEAPON_RANGED_H
#define GAME_HUD_MODEL_WEAPON_RANGED_H

#include "StdAfx.h"
#include "GameTypes.h"

#include "PlayerHands.h"
using namespace hpl;

class cRangedWeaponAttack
{
public:
	cHudModelPose mStart;
	cHudModelPose mEnd;

	float mfAttackLength;
	float mfReloadLength;
	float mfReloadFullLength;
	float mfTimeOfAttack;

	float mfMaxImpulse;
	float mfMinImpulse;

	float mfMinMass;
	float mfMaxMass;

	float mfMinDamage;
	float mfMaxDamage;

	cVector3f mvSpinMul;

	float mfDamageRange;
	cVector3f mvDamageSize;

	float mfAttackRange;

	float mfAttackSpeed;
	int mlAttackStrength;

	tString msHitPS;
	int mlHitPSPrio;

	tString msShootSound;
	tString msEmptySound;
	tString msHitSound;

	iCollideShape* mpCollider;
	cBoundingVolume mBV;
};

//-------------------------------------------

class cRangedRayCallback : public iPhysicsRayCallback
{
public:
	void Reset();
	bool OnIntersect(iPhysicsBody* pBody, cPhysicsRayParams* apParams);

	iPhysicsBody* mpClosestBody;
	float mfShortestDist;
	cVector3f mvPosition;
	cVector3f mvNormal;
};

//-------------------------------------------

class cHudModel_WeaponRanged : public iHudModel
{
	friend class cPlayerHands;
public:
	cHudModel_WeaponRanged();

	void LoadData(TiXmlElement* apRootElem);

	void OnAttackDown();
	void OnAttackUp();

	bool UpdatePoseMatrix(cMatrixf& aPoseMtx, float afTimeStep);

	void PostSceneDraw();

	bool IsAttacking();

	cVector3f GetHapticSize() { return mvHapticSize; }
	float GetHapticScale() { return mfHapticScale; }
	cVector3f GetHapticRot() { return mvHapticRot; }


	cRangedWeaponAttack* GetAttack(int alX) { return  &mvAttacks[alX]; }
private:
	void ResetExtraData();

	void Attack();
	void HitBody(iPhysicsBody* apBody);

	void PlaySound(const tString& asSound);

	void LoadExtraEntites();
	void DestroyExtraEntities();

	bool mbDrawDebug;

	int mlCurrentAttack;
	int mlAttackState;
	float mfTime;

	float mfHapticScale;
	cVector3f mvHapticSize;
	cVector3f mvHapticRot;

	cMatrixf m_mtxPrevPose;
	cMatrixf m_mtxNextPose;

	float mfMoveSpeed;

	bool mbButtonDown;
	bool mbAttacked;

	bool mbHands;

	cRangedRayCallback mRayCallback;

	std::vector<cRangedWeaponAttack> mvAttacks;

	iLowLevelHaptic* mpLowLevelHaptic;
	iHapticForce* mpHHitForce;
};

//-------------------------------------------

#endif // GAME_HUD_MODEL_WEAPON_RANGED_H
