/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of Penumbra Overture.
 *
 * Penumbra Overture is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Penumbra Overture is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Penumbra Overture.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef GAME_GAME_ENEMY_SOLDIER_H
#define GAME_GAME_ENEMY_SOLDIER_H

#include "StdAfx.h"
#include "GameEnemy.h"

using namespace hpl;

//-----------------------------------------

class cGameEnemy_Soldier;

// BASE STATE
class iGameEnemyState_Soldier_Base : public iGameEnemyState
{
public:
	iGameEnemyState_Soldier_Base(int alId, cInit* apInit, iGameEnemy* apEnemy);

	virtual void OnSeePlayer(const cVector3f& avPosition, float afChance);
	virtual bool OnHearNoise(const cVector3f& avPosition, float afVolume);
	virtual void OnTakeHit(float afDamage);
	virtual void OnDeath(float afDamage);
	virtual void OnFlashlight(const cVector3f& avPosition);

	virtual void OnAnimationOver(const tString& asAnimName) {}

	virtual void OnDraw() {}
	virtual void OnPostSceneDraw() {}

protected:
	cGameEnemy_Soldier* mpEnemySoldier;
};

//-----------------------------------------

// IDLE STATE
class cGameEnemyState_Soldier_Idle : public iGameEnemyState_Soldier_Base
{
public:
	cGameEnemyState_Soldier_Idle(int alId, cInit* apInit, iGameEnemy* apEnemy) :
		iGameEnemyState_Soldier_Base(alId, apInit, apEnemy) {}

	void OnEnterState(iGameEnemyState* apPrevState);
	void OnLeaveState(iGameEnemyState* apNextState);

	void OnUpdate(float afTimeStep);
};

//-----------------------------------------

// ATTENTION STATE
class cGameEnemyState_Soldier_Attention : public iGameEnemyState_Soldier_Base
{
public:
	cGameEnemyState_Soldier_Attention(int alId, cInit* apInit, iGameEnemy* apEnemy) :
		iGameEnemyState_Soldier_Base(alId, apInit, apEnemy) {}

	void OnEnterState(iGameEnemyState* apPrevState);
	void OnLeaveState(iGameEnemyState* apNextState);

	void OnUpdate(float afTimeStep);

	void OnSeePlayer(const cVector3f& avPosition, float afChance);
	bool OnHearNoise(const cVector3f& avPosition, float afVolume);
	void OnFlashlight(const cVector3f& avPosition);

	void OnDraw();
private:
	float mfTime;

};

//-----------------------------------------

// HUNT STATE
class cGameEnemyState_Soldier_Hunt : public iGameEnemyState_Soldier_Base
{
public:
	cGameEnemyState_Soldier_Hunt(int alId, cInit* apInit, iGameEnemy* apEnemy) :
		iGameEnemyState_Soldier_Base(alId, apInit, apEnemy) {
		mbBreakingDoor = false;
	}

	void OnEnterState(iGameEnemyState* apPrevState);
	void OnLeaveState(iGameEnemyState* apNextState);

	void OnUpdate(float afTimeStep);

	void OnSeePlayer(const cVector3f& avPosition, float afChance);
	bool OnHearNoise(const cVector3f& avPosition, float afVolume);
	void OnFlashlight(const cVector3f& avPosition) {}

	void OnDraw();
private:
	float mfUpdatePathCount;
	float mfUpdateFreq;
	bool mbFreeEnemyPath;
	bool mbLostPlayer;
	float mfLostPlayerCount;
	float mfMaxLostPlayerCount;
	bool mbFoundNoPath;

	int mlStuckAtMaxCount;

	int mlBreakDoorCount;
	bool mbBreakingDoor;
};

//-----------------------------------------

// ATTACK STATE
class cGameEnemyState_Soldier_Attack : public iGameEnemyState_Soldier_Base
{
public:
	cGameEnemyState_Soldier_Attack(int alId, cInit* apInit, iGameEnemy* apEnemy) :
		iGameEnemyState_Soldier_Base(alId, apInit, apEnemy) {}

	void OnEnterState(iGameEnemyState* apPrevState);
	void OnLeaveState(iGameEnemyState* apNextState);

	void OnUpdate(float afTimeStep);

	void OnAnimationOver(const tString& asName);

	void OnPostSceneDraw();

	void OnSeePlayer(const cVector3f& avPosition, float afChance) {}
	bool OnHearNoise(const cVector3f& avPosition, float afVolume) { return false; }
	void OnFlashlight(const cVector3f& avPosition) {}
private:
	float mfDamageTimer;
	float mfJumpTimer;
	bool mbAttacked;
};

//-----------------------------------------

// BREAK DOOR STATE
class cGameEnemyState_Soldier_BreakDoor : public iGameEnemyState_Soldier_Base
{
public:
	cGameEnemyState_Soldier_BreakDoor(int alId, cInit* apInit, iGameEnemy* apEnemy) :
		iGameEnemyState_Soldier_Base(alId, apInit, apEnemy) {}

	void OnEnterState(iGameEnemyState* apPrevState);
	void OnLeaveState(iGameEnemyState* apNextState);

	void OnUpdate(float afTimeStep);

	void OnAnimationOver(const tString& asName);

	void OnPostSceneDraw();

	void OnSeePlayer(const cVector3f& avPosition, float afChance) {}
	bool OnHearNoise(const cVector3f& avPosition, float afVolume) { return false; }
	void OnFlashlight(const cVector3f& avPosition) {}
private:
	float mfDamageTimer;
	float mfStopMoveTimer;
	bool mbAttacked;
	bool mbStopped;
	int mlCount;
};

//-----------------------------------------

// FLEE STATE
class cGameEnemyState_Soldier_Flee : public iGameEnemyState_Soldier_Base
{
public:
	cGameEnemyState_Soldier_Flee(int alId, cInit* apInit, iGameEnemy* apEnemy) :
		iGameEnemyState_Soldier_Base(alId, apInit, apEnemy) {}

	void OnEnterState(iGameEnemyState* apPrevState);
	void OnLeaveState(iGameEnemyState* apNextState);

	void OnUpdate(float afTimeStep);

	bool OnHearNoise(const cVector3f& avPosition, float afVolume) { return false; }
	void OnSeePlayer(const cVector3f& avPosition, float afChance) {}
	void OnFlashlight(const cVector3f& avPosition) {}
private:
	float mfTimer;
	float mfBackAngle;
	bool mbBackwards;
	float mfCheckBehindTime;
	bool mbBackingFromBreakDoor;
};

//-----------------------------------------

// CALL BACKUP STATE
class cGameEnemyState_Soldier_CallBackup : public iGameEnemyState_Soldier_Base
{
public:
	cGameEnemyState_Soldier_CallBackup(int alId, cInit* apInit, iGameEnemy* apEnemy) :
		iGameEnemyState_Soldier_Base(alId, apInit, apEnemy) {}

	void OnEnterState(iGameEnemyState* apPrevState);
	void OnLeaveState(iGameEnemyState* apNextState);

	void OnUpdate(float afTimeStep);

	void OnSeePlayer(const cVector3f& avPosition, float afChance) {}
	bool OnHearNoise(const cVector3f& avPosition, float afVolume) { return false; }
	//void OnTakeHit(float afDamage){}
	void OnFlashlight(const cVector3f& avPosition) {}

	void OnAnimationOver(const tString& asName);

private:
	bool mbCheckAnim;
};

//-----------------------------------------

// KNOCKDOWN STATE
class cGameEnemyState_Soldier_KnockDown : public iGameEnemyState_Soldier_Base
{
public:
	cGameEnemyState_Soldier_KnockDown(int alId, cInit* apInit, iGameEnemy* apEnemy) :
		iGameEnemyState_Soldier_Base(alId, apInit, apEnemy) {}

	void OnEnterState(iGameEnemyState* apPrevState);
	void OnLeaveState(iGameEnemyState* apNextState);

	void OnUpdate(float afTimeStep);

	void OnSeePlayer(const cVector3f& avPosition, float afChance) {}
	bool OnHearNoise(const cVector3f& avPosition, float afVolume) { return false; }
	void OnTakeHit(float afDamage) {}
	void OnFlashlight(const cVector3f& avPosition) {}

	void OnAnimationOver(const tString& asName);

private:
	float mfTimer;
	bool mbCheckAnim;
	int mlPrevToughness;

	int mlStuckAtMaxCount;
};

//-----------------------------------------


// DEAD STATE
class cGameEnemyState_Soldier_Dead : public iGameEnemyState_Soldier_Base
{
public:
	cGameEnemyState_Soldier_Dead(int alId, cInit* apInit, iGameEnemy* apEnemy) :
		iGameEnemyState_Soldier_Base(alId, apInit, apEnemy) {}

	void OnEnterState(iGameEnemyState* apPrevState);
	void OnLeaveState(iGameEnemyState* apNextState);

	void OnUpdate(float afTimeStep);

	void OnSeePlayer(const cVector3f& avPosition, float afChance) {}
	bool OnHearNoise(const cVector3f& avPosition, float afVolume) { return false; }
	void OnTakeHit(float afDamage) {}
	void OnFlashlight(const cVector3f& avPosition) {}
};

//-----------------------------------------

// PATROL STATE
class cGameEnemyState_Soldier_Patrol : public iGameEnemyState_Soldier_Base
{
public:
	cGameEnemyState_Soldier_Patrol(int alId, cInit* apInit, iGameEnemy* apEnemy) :
		iGameEnemyState_Soldier_Base(alId, apInit, apEnemy) {}

	void OnEnterState(iGameEnemyState* apPrevState);
	void OnLeaveState(iGameEnemyState* apNextState);

	void OnUpdate(float afTimeStep);

	void OnAnimationOver(const tString& asName);
private:
	bool mbWaiting;
	bool mbAnimation;

	float mfIdleSoundTime;

	int mlStuckAtMaxCount;
};

//-----------------------------------------

// INVESTIGATE STATE
class cGameEnemyState_Soldier_Investigate : public iGameEnemyState_Soldier_Base
{
public:
	cGameEnemyState_Soldier_Investigate(int alId, cInit* apInit, iGameEnemy* apEnemy) :
		iGameEnemyState_Soldier_Base(alId, apInit, apEnemy) {
		mlKnockCount = 0;
	}

	void OnEnterState(iGameEnemyState* apPrevState);
	void OnLeaveState(iGameEnemyState* apNextState);

	void OnUpdate(float afTimeStep);

	bool OnHearNoise(const cVector3f& avPosition, float afVolume);

	float mfIdleSoundTime;
	float mfHearSoundCount;
	float mfHighestVolume;

	float mfInterest;

	int mlKnockCount;
};

//-----------------------------------------

// MOVETO STATE
class cGameEnemyState_Soldier_MoveTo : public iGameEnemyState_Soldier_Base
{
public:
	cGameEnemyState_Soldier_MoveTo(int alId, cInit* apInit, iGameEnemy* apEnemy) :
		iGameEnemyState_Soldier_Base(alId, apInit, apEnemy) {}

	void OnEnterState(iGameEnemyState* apPrevState);
	void OnLeaveState(iGameEnemyState* apNextState);

	void OnUpdate(float afTimeStep);

	float mfIdleSoundTime;
	int mlBreakCount;
};

//-----------------------------------------

class cGameEnemy_Soldier : public iGameEnemy
{
public:
	cGameEnemy_Soldier(cInit* apInit, const tString& asName, TiXmlElement* apGameElem);
	~cGameEnemy_Soldier();

	void OnLoad();

	void OnUpdate(float afTimeStep);

	void ShowPlayer(const cVector3f& avPlayerFeetPos);

	bool MoveToPos(const cVector3f& avFeetPos);

	bool IsFighting();

	iCollideShape* GetAttackShape() { return mpAttackShape; }
	iCollideShape* GetBreakDoorShape() { return mpBreakDoorShape; }

	float mfMinKnockDamage;
	float mfCertainKnockDamage;

	float mfLengthBodyToAss;

	int mlFaction; //Faction, might be allies and linguisdraconis. Will try to kill if not same faction.

	bool mbAlly; //Won't hurt player
	bool mbUnstable; //Will act more like monster than regular soldier

	iCharacterBody* mpWorstEnemyBody; //Whoever is their worst enemy, may be one that hurts or has hurt the most, if I can implement trauma from team getting killed...

	//State properties
	float mfIdleFOV;
	tString msIdleFoundPlayerSound;
	float mfIdleMinSeeChance;
	float mfIdleMinHearVolume;
	tString msIdleSound;
	float mfIdleSoundMinInteraval;
	float mfIdleSoundMaxInterval;
	float mfIdleCallBackupChance;

	tString msInvestigateSound;
	float mfInvestigateTime;

	tString msAttentionSound;
	float mfAttentionTime;
	float mfAttentionMinDist;

	float mfHuntFOV;
	float mfHuntSpeed;
	float mfHuntForLostPlayerTime;
	float mfHuntMinSeeChance;
	float mfHuntMinHearVolume;

	float mfAttackDistance;
	float mfAttackSpeed;
	float mfAttackJumpTime;
	float mfAttackDamageTime;
	cVector3f mvAttackDamageSize;
	float mfAttackDamageRange;
	float mfAttackMinDamage;
	float mfAttackMaxDamage;
	tString msAttackStartSound;
	tString msAttackHitSound;
	float mfAttackMinMass;
	float mfAttackMaxMass;
	float mfAttackMinImpulse;
	float mfAttackMaxImpulse;
	int mlAttackStrength;

	tString msBreakDoorAnimation;
	float mfBreakDoorSpeed;
	float mfBreakDoorDamageTime;
	cVector3f mvBreakDoorDamageSize;
	float mfBreakDoorDamageRange;
	float mfBreakDoorMinDamage;
	float mfBreakDoorMaxDamage;
	tString msBreakDoorStartSound;
	tString msBreakDoorHitSound;
	float mfBreakDoorMinMass;
	float mfBreakDoorMaxMass;
	float mfBreakDoorMinImpulse;
	float mfBreakDoorMaxImpulse;
	int mlBreakDoorStrength;
	bool mbBreakDoorRiseAtEnd;

	tString msKnockDownSound;

	tString msDeathSound;

	float mfFleePositionChance;
	float mfFleePositionMaxTime;
	float mfFleePositionMinDistance;
	float mfFleePositionMaxDistance;
	float mfFleeBackChance;
	float mfFleeBackTime;
	float mfFleeBackSpeed;

	tString msCallBackupAnimation;
	tString msCallBackupSound;
	float mfCallBackupRange;

private:
	iCollideShape* mpAttackShape;
	iCollideShape* mpBreakDoorShape;
};

//-----------------------------------------

#endif // GAME_GAME_ENEMY_SOLDIER_H
