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
#include "GameEnemy_Soldier.h"


#include "Player.h"
#include "AttackHandler.h"
#include "EffectHandler.h"
#include "GameMusicHandler.h"
#include "GameSwingDoor.h"
#include "MapHandler.h"

 //////////////////////////////////////////////////////////////////////////
 // BASE STATE
 //////////////////////////////////////////////////////////////////////////

 //-----------------------------------------------------------------------

iGameEnemyState_Soldier_Base::iGameEnemyState_Soldier_Base(int alId, cInit* apInit, iGameEnemy* apEnemy)
	: iGameEnemyState(alId, apInit, apEnemy)
{
	mpEnemySoldier = static_cast<cGameEnemy_Soldier*>(mpEnemy);
}

//-----------------------------------------------------------------------

void iGameEnemyState_Soldier_Base::OnSeePlayer(const cVector3f& avPosition, float afChance)
{
	//return;
	if (mpPlayer->GetHealth() <= 0) return;

	if (afChance >= mpEnemySoldier->mfIdleMinSeeChance)
	{
		/*if( (mlId == STATE_IDLE || mlId == STATE_INVESTIGATE || mlId == STATE_PATROL) &&
			cMath::RandRectf(0,1) < mpEnemySoldier->mfIdleCallBackupChance &&
			mpEnemy->CheckForTeamMate(8,true)==false)
		{
			mpEnemy->ChangeState(STATE_CALLBACKUP);
		}
		else
		{
			//mpEnemy->ChangeState(STATE_HUNT);
			//mpEnemySoldier->PlaySound(mpEnemySoldier->msIdleFoundPlayerSound);
		}*/

		float fDist = cMath::Vector3Dist(mpMover->GetCharBody()->GetFeetPosition(),
			mpPlayer->GetCharacterBody()->GetFeetPosition());
		if (fDist >= mpEnemySoldier->mfAttentionMinDist)
		{
			mpEnemy->ChangeState(STATE_ATTENTION);
		}
		else
		{
			mpEnemy->ChangeState(STATE_HUNT);
			mpEnemySoldier->PlaySound(mpEnemySoldier->msIdleFoundPlayerSound);
		}

	}
}

bool iGameEnemyState_Soldier_Base::OnHearNoise(const cVector3f& avPosition, float afVolume)
{
	//return false;
	float afDistance = (mpMover->GetCharBody()->GetPosition() - avPosition).Length();

	if (afVolume >= mpEnemySoldier->mfIdleMinHearVolume && afDistance > 0.4f)
	{
		mpEnemy->SetTempPosition(avPosition);
		mpEnemy->ChangeState(STATE_INVESTIGATE);
		return true;
	}

	return false;
}

void iGameEnemyState_Soldier_Base::OnTakeHit(float afDamage)
{
	if (afDamage >= mpEnemySoldier->mfMinKnockDamage)
	{
		if (mpInit->mbWeaponAttacking)
		{
			float fChance = afDamage / mpEnemySoldier->mfCertainKnockDamage;//(mpEnemySoldier->mfCertainKnockDamage*4);
			if (fChance > cMath::RandRectf(0, 1))
			{
				mpEnemy->ChangeState(STATE_KNOCKDOWN);
			}
		}
		else
		{
			if (afDamage >= mpEnemySoldier->mfCertainKnockDamage)
			{
				mpEnemy->ChangeState(STATE_KNOCKDOWN);
			}
			else
			{
				float fChance = afDamage / mpEnemySoldier->mfCertainKnockDamage;
				if (fChance > cMath::RandRectf(0, 1))
				{
					mpEnemy->ChangeState(STATE_KNOCKDOWN);
				}
			}
		}
	}
}

void iGameEnemyState_Soldier_Base::OnFlashlight(const cVector3f& avPosition)
{
	//mpInit->mpEffectHandler->GetSubTitle()->Add("Flashlight!",0.5f,true);
	//OnSeePlayer(mpPlayer->GetCharacterBody()->GetFeetPosition(),1.0f);

	//mpEnemy->SetLastPlayerPos(mpPlayer->GetCharacterBody()->GetFeetPosition());
	//mpEnemy->ChangeState(STATE_HUNT);

	mpEnemy->SetTempPosition(avPosition);
	mpEnemy->ChangeState(STATE_INVESTIGATE);
}

void iGameEnemyState_Soldier_Base::OnDeath(float afDamage)
{
	mpEnemy->ChangeState(STATE_DEAD);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// IDLE STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Idle::OnEnterState(iGameEnemyState* apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpEnemySoldier->mfIdleFOV);

	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Idle::OnLeaveState(iGameEnemyState* apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Idle::OnUpdate(float afTimeStep)
{
	if (mpEnemy->GetPatrolNodeNum() > 0)
	{
		mpEnemy->ChangeState(STATE_PATROL);
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PATROL STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Patrol::OnEnterState(iGameEnemyState* apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpEnemySoldier->mfIdleFOV);

	//Setup patrol
	cEnemyPatrolNode* pPatrolNode = mpEnemy->CurrentPatrolNode();
	cAINode* pNode = mpMover->GetNodeContainer()->GetNodeFromName(pPatrolNode->msNodeName);

	if (mpEnemy->GetDoorBreakCount() > 3.0f)
	{
		mpEnemy->SetDoorBreakCount(0);
		mpMover->SetMaxDoorToughness(0);
	}

	mbWaiting = false;
	mbAnimation = false;
	mlStuckAtMaxCount = 0;


	mfIdleSoundTime = cMath::RandRectf(mpEnemySoldier->mfIdleSoundMinInteraval,
		mpEnemySoldier->mfIdleSoundMaxInterval);

	mpMover->SetMaxDoorToughness(-1);

	if (mpMover->MoveToPos(pNode->GetPosition()) == false)
	{
		//tString sStr = "Could not get to path node "+pPatrolNode->msNodeName;
		//mpInit->mpEffectHandler->GetSubTitle()->Add(sStr,3,true);

		mpEnemy->IncCurrentPatrolNode();
		mbWaiting = true;
		mpEnemy->SetWaitTime(1.0f);
	}
	else
	{
		//tString sStr = "Moving to path node "+pPatrolNode->msNodeName;
		//mpInit->mpEffectHandler->GetSubTitle()->Add(sStr,3,true);
	}


	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Patrol::OnLeaveState(iGameEnemyState* apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Patrol::OnUpdate(float afTimeStep)
{
	/////////////////////////////////
	//Waiting for timer or animation to end
	if (mbWaiting)
	{
		//////////////////////////////
		//Play idle sound
		if (mfIdleSoundTime <= 0)
		{
			mfIdleSoundTime = cMath::RandRectf(mpEnemySoldier->mfIdleSoundMinInteraval,
				mpEnemySoldier->mfIdleSoundMaxInterval);

			mpEnemy->PlaySound(mpEnemySoldier->msIdleSound);
		}
		else
		{
			mfIdleSoundTime -= afTimeStep;
		}

		//////////////////////////////
		// Timer is up
		if (mpEnemy->GetWaitTimeCount() >= mpEnemy->GetWaitTime())
		{
			if (mbAnimation == false)
			{
				mpEnemy->SetWaitTimeCount(0);
				cEnemyPatrolNode* pPatrolNode = mpEnemy->CurrentPatrolNode();
				cAINode* pNode = mpMover->GetNodeContainer()->GetNodeFromName(pPatrolNode->msNodeName);

				mpEnemy->UseMoveStateAnimations();
				mbWaiting = false;

				if (mpMover->MoveToPos(pNode->GetPosition()) == false)
				{
					//tString sStr = "Could not get to path node "+pPatrolNode->msNodeName;
					//mpInit->mpEffectHandler->GetSubTitle()->Add(sStr,3,false);
					mpEnemy->IncCurrentPatrolNode();
					mbWaiting = true;
					mpEnemy->SetWaitTime(1.0f);
				}
				else
				{
					//tString sStr = "Moving to path node "+pPatrolNode->msNodeName;
					//mpInit->mpEffectHandler->GetSubTitle()->Add(sStr,3,false);
				}

			}
			else
			{
				mpEnemy->GetCurrentAnimation()->SetLoop(false);
			}
		}
		else
		{
			mpEnemy->AddWaitTimeCount(afTimeStep);
		}
	}
	/////////////////////////////////
	//Check if path is over
	else
	{
		//////////////////////////////
		//Play idle sound
		if (mfIdleSoundTime <= 0)
		{
			mfIdleSoundTime = cMath::RandRectf(mpEnemySoldier->mfIdleSoundMinInteraval,
				mpEnemySoldier->mfIdleSoundMaxInterval);

			mpEnemy->PlaySound(mpEnemySoldier->msIdleSound);
		}
		else
		{
			mfIdleSoundTime -= afTimeStep;
		}

		//////////////////////////////
		//Stuck counter
		if (mpMover->GetStuckCounter() > 1.7f)
		{
			if (mpEnemy->CheckForDoor())
			{
				mpEnemy->ChangeState(STATE_BREAKDOOR);
			}
			else
			{
				mlStuckAtMaxCount++;
				if (mlStuckAtMaxCount >= 6)
				{
					mpEnemy->ChangeState(STATE_IDLE);
					mpEnemy->SetWaitTime(1.0f);
					mpEnemy->IncCurrentPatrolNode();
				}
			}
			mpMover->ResetStuckCounter();
		}

		//////////////////////////////
		//Got to ned of path
		if (mpMover->IsMoving() == false)
		{
			cEnemyPatrolNode* pPatrolNode = mpEnemy->CurrentPatrolNode();

			mpEnemy->SetWaitTime(pPatrolNode->mfWaitTime);
			mpEnemy->IncCurrentPatrolNode();

			if (pPatrolNode->msAnimation != "")
			{
				mpEnemy->PlayAnim(pPatrolNode->msAnimation, true, 0.2f);
				mbAnimation = true;
			}

			mbWaiting = true;
		}
	}
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Patrol::OnAnimationOver(const tString& asName)
{
	mbAnimation = false;
}

//////////////////////////////////////////////////////////////////////////
// ATTENTION STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Attention::OnEnterState(iGameEnemyState* apPrevState)
{
	//Animation
	mpEnemy->PlayAnim("Angry", true, 0.2f);


	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpEnemySoldier->mfIdleFOV);

	mpMover->Stop();
	mpMover->TurnToPos(mpPlayer->GetCharacterBody()->GetFeetPosition());

	mpEnemy->PlaySound(mpEnemySoldier->msAttentionSound);
	mfTime = mpEnemySoldier->mfAttentionTime;

#ifndef DEMO_VERSION
	if (mpInit->mDifficulty == eGameDifficulty_Easy) mfTime *= 1.7f;
	if (mpInit->mbHasHaptics) mfTime *= 1.3f;

#endif
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Attention::OnLeaveState(iGameEnemyState* apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Attention::OnUpdate(float afTimeStep)
{
	mpMover->TurnToPos(mpPlayer->GetCharacterBody()->GetFeetPosition());
	mfTime -= afTimeStep;

	if (mfTime <= 0)
	{
		if (mpEnemy->CanSeePlayer())
		{
			mpEnemy->ChangeState(STATE_HUNT);
			mpEnemy->PlaySound(mpEnemySoldier->msIdleFoundPlayerSound);
		}
		else
		{
			if (mlPreviousState == STATE_ATTENTION)
				mpEnemy->ChangeState(STATE_IDLE);
			else
				mpEnemy->ChangeState(mlPreviousState);
		}
	}

}

//-----------------------------------------------------------------------


void cGameEnemyState_Soldier_Attention::OnSeePlayer(const cVector3f& avPosition, float afChance)
{

}
bool cGameEnemyState_Soldier_Attention::OnHearNoise(const cVector3f& avPosition, float afVolume)
{
	return false;
}
void cGameEnemyState_Soldier_Attention::OnFlashlight(const cVector3f& avPosition)
{
	mpEnemy->ChangeState(STATE_HUNT);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Attention::OnDraw()
{

}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// INVESTIGATE STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Investigate::OnEnterState(iGameEnemyState* apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpEnemySoldier->mfIdleFOV);

	//Play sound
	mpEnemy->PlaySound(mpEnemySoldier->msInvestigateSound);

	cAINode* pNode = mpMover->GetAINodeAtPosInRange(mpEnemy->GetTempPosition(), 0.0f, 5.0f, true, 0.1f);

	if (mpEnemy->GetDoorBreakCount() > 6.0f)
	{
		mpEnemy->SetDoorBreakCount(0);
		mpMover->SetMaxDoorToughness(0);
	}

	if (pNode)
	{
		if (mpMover->MoveToPos(pNode->GetPosition()) == false)
		{
			mpEnemy->ChangeState(STATE_IDLE);
		}
	}
	else
	{
		mpEnemy->ChangeState(STATE_IDLE);
	}

	mpMover->SetMaxDoorToughness(-1);

	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);

	mfIdleSoundTime = cMath::RandRectf(mpEnemySoldier->mfIdleSoundMinInteraval,
		mpEnemySoldier->mfIdleSoundMaxInterval);

	mfInterest = mpEnemySoldier->mfInvestigateTime;

	if (apPrevState->GetId() != STATE_INVESTIGATE)
	{
		mfHighestVolume = 0.0f;
	}

	mfHearSoundCount = 5.0f;
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Investigate::OnLeaveState(iGameEnemyState* apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Investigate::OnUpdate(float afTimeStep)
{
	if (mfHearSoundCount > 0) {
		mfHearSoundCount -= afTimeStep;
		if (mfHearSoundCount <= 0)mfHearSoundCount = 0;
	}
	if (mfInterest > 0) {
		mfInterest -= afTimeStep;
		if (mfInterest <= 0) mfInterest = 0;
	}

	////////////////////////////////
	//Play idle sound
	if (mfIdleSoundTime <= 0)
	{
		mfIdleSoundTime = cMath::RandRectf(mpEnemySoldier->mfIdleSoundMinInteraval,
			mpEnemySoldier->mfIdleSoundMaxInterval);

		mpEnemy->PlaySound(mpEnemySoldier->msIdleSound);
	}
	else
	{
		mfIdleSoundTime -= afTimeStep;
	}

	////////////////////////////////
	//Stuck counter
	if (mpMover->GetStuckCounter() > 1.5f)
	{
		if (mlKnockCount == 1) {
			mpEnemy->ChangeState(STATE_IDLE);
			mlKnockCount = 0;
		}
		else
		{
			if (mpEnemy->CheckForDoor())
			{
				mpEnemy->ChangeState(STATE_BREAKDOOR);
			}
			mpMover->ResetStuckCounter();
			mlKnockCount++;
		}
	}

	if (mpMover->IsMoving() == false && mfInterest <= 0)
	{
		mlKnockCount = 0;
		mpEnemy->ChangeState(STATE_IDLE);
	}
	else if (mpMover->IsMoving() == false)
	{
		cAINode* mpNearNode = mpMover->GetAINodeInRange(0.0f, 5.0f);
		mpMover->MoveToPos(mpNearNode->GetPosition());
	}
}

//-----------------------------------------------------------------------

bool cGameEnemyState_Soldier_Investigate::OnHearNoise(const cVector3f& avPosition, float afVolume)
{
	if (mfHearSoundCount <= 0 && mfHighestVolume < afVolume &&
		afVolume >= mpEnemySoldier->mfIdleMinHearVolume)
	{
		mfHighestVolume = afVolume;
		mfInterest += mpEnemySoldier->mfInvestigateTime / 4.0f;
		mpEnemy->SetTempPosition(avPosition);
		OnEnterState(this);

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// MOVE TO STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_MoveTo::OnEnterState(iGameEnemyState* apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	//Setup enemy
	mpEnemy->SetFOV(mpEnemySoldier->mfIdleFOV);

	//Play sound
	mpEnemy->PlaySound(mpEnemySoldier->msInvestigateSound);

	if (mpMover->MoveToPos(mpEnemy->GetTempPosition()) == false)
	{
		//mpInit->mpEffectHandler->GetSubTitle()->Add("Could not move to pos!\n",3,true);
		mpEnemy->ChangeState(apPrevState->GetId());
		return;
	}
	else
	{
		//mpInit->mpEffectHandler->GetSubTitle()->Add("Moving to pos!\n",3,true);
	}

	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);

	mfIdleSoundTime = cMath::RandRectf(mpEnemySoldier->mfIdleSoundMinInteraval,
		mpEnemySoldier->mfIdleSoundMaxInterval);

	mlBreakCount = 0;
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_MoveTo::OnLeaveState(iGameEnemyState* apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_MoveTo::OnUpdate(float afTimeStep)
{
	////////////////////////////////
	//Play idle sound
	if (mfIdleSoundTime <= 0)
	{
		mfIdleSoundTime = cMath::RandRectf(mpEnemySoldier->mfIdleSoundMinInteraval,
			mpEnemySoldier->mfIdleSoundMaxInterval);

		mpEnemy->PlaySound(mpEnemySoldier->msIdleSound);
	}
	else
	{
		mfIdleSoundTime -= afTimeStep;
	}

	////////////////////////////////
	//Stuck counter
	if (mpMover->GetStuckCounter() > 1.5f)
	{
		if (mlBreakCount == 1)
		{
			mpEnemy->ChangeState(STATE_IDLE);
		}
		else
		{
			if (mpEnemy->CheckForDoor())
			{
				mpEnemy->ChangeState(STATE_BREAKDOOR);
			}
			mpMover->ResetStuckCounter();
			mlBreakCount++;
		}
	}

	if (mpMover->IsMoving() == false)
	{
		mpEnemy->ChangeState(STATE_IDLE);
	}
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// HUNT STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Hunt::OnEnterState(iGameEnemyState* apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

#ifndef DEMO_VERSION
	float fMul = 1.0f;
	if (mpInit->mbHasHaptics) fMul = 0.6f;

	if (mpInit->mDifficulty == eGameDifficulty_Easy)
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward, mpEnemySoldier->mfHuntSpeed * 0.7f * fMul);
	else if (mpInit->mDifficulty == eGameDifficulty_Normal)
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward, mpEnemySoldier->mfHuntSpeed * fMul);
	else
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward, mpEnemySoldier->mfHuntSpeed * 1.25f * fMul);
#else
	mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward, mpEnemySoldier->mfHuntSpeed);
#endif


	//Setup enemy
	mpEnemy->SetFOV(mpEnemySoldier->mfHuntFOV);

	mfUpdatePathCount = 0;
	mfUpdateFreq = 1.0f;
	mbFreeEnemyPath = false;

	if (mbBreakingDoor && mpEnemy->CanSeePlayer() == false)
	{
		mlBreakDoorCount++;
		if (mlBreakDoorCount >= 3)
		{
			mpEnemy->ChangeState(STATE_IDLE);
			return;
		}
	}
	else
	{
		mlBreakDoorCount = 0;
	}

	mbBreakingDoor = false;
	mbFoundNoPath = false;
	mbLostPlayer = false;
	mfLostPlayerCount = 0;
	mfMaxLostPlayerCount = mpEnemySoldier->mfHuntForLostPlayerTime;

	mlStuckAtMaxCount = 0;

	mpInit->mpMusicHandler->AddAttacker(mpEnemy);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Hunt::OnLeaveState(iGameEnemyState* apNextState)
{
	mpMover->SetMaxDoorToughness(-1);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Hunt::OnUpdate(float afTimeStep)
{
	if (mpPlayer->GetHealth() <= 0) {
		mpEnemy->ChangeState(STATE_IDLE);
		return;
	}

	if (mpMover->GetStuckCounter() > 1.1f)
	{
		if (mpEnemy->CheckForDoor())
		{
			mbBreakingDoor = true;
			mpEnemy->ChangeState(STATE_BREAKDOOR);
		}
		else
		{
			mlStuckAtMaxCount++;
			if (mlStuckAtMaxCount >= 6)
			{
				mpEnemy->ChangeState(STATE_IDLE);
			}
		}
		mpMover->ResetStuckCounter();
	}

	if (mfUpdatePathCount <= 0)
	{
		mbFoundNoPath = false;
		//mpInit->mpEffectHandler->GetSubTitle()->Add("Update Path!",1.0f,true);
		mfUpdatePathCount = mfUpdateFreq;

		cAINodeContainer* pNodeCont = mpEnemy->GetMover()->GetNodeContainer();

		//Log("%s: Checking free path\n",mpEnemy->GetName().c_str());

		//Check if there is a free path to the player
		if (mbLostPlayer == false && mpMover->FreeDirectPathToChar(mpEnemySoldier->mpWorstEnemyBody))
		{
			mbFreeEnemyPath = true;
			mpMover->Stop();
			mpMover->SetMaxDoorToughness(-1);
		}
		else
		{
			mbFreeEnemyPath = false;
		}

		//Get path to player
		if (mbFreeEnemyPath == false && mbLostPlayer == false)
		{
			if (mpEnemy->GetDoorBreakCount() > 6.0f)
			{
				mpMover->SetMaxDoorToughness(0);
			}

			//Log("%s: Move to pos\n",mpEnemy->GetName().c_str());

			if (mpMover->MoveToPos(mpEnemy->GetLastPlayerPos()) == false)
			{
				bool bFoundAnotherWay = false;
				/*float fHeight = mpMover->GetCharBody()->GetPosition().y -
								mpPlayer->GetCharacterBody()->GetPosition().y;
				if(cMath::Abs(fHeight) > mpMover->GetNodeContainer()->GetMaxHeight())
				{
					cVector3f vPos = mpEnemy->GetLastPlayerPos();
					vPos.y = mpMover->GetCharBody()->GetFeetPosition().y+0.1f;

					if(mpMover->MoveToPos(vPos))
					{
						bFoundAnotherWay = true;
					}
				}*/

				if (bFoundAnotherWay == false)
				{
					mfUpdatePathCount = mfUpdateFreq * 5.0f;
					mpMover->Stop();
					//Set this so the enemey at least runs toward the player.
					mbFoundNoPath = true;
				}
			}

			//Log("%s: Done with that.\n",mpEnemy->GetName().c_str());
		}
	}
	else
	{
		mfUpdatePathCount -= afTimeStep;
	}

	////////////////////////////////
	//Go directly towards the enemy
	if (mbFreeEnemyPath || (mbFoundNoPath && mpMover->IsMoving() == false))
	{
		//Go towards enemy
		mpMover->MoveDirectToPos(mpEnemySoldier->mpWorstEnemyBody->GetFeetPosition(), afTimeStep);

		//Check if he should attack.
		if (mpMover->DistanceToChar2D(mpEnemySoldier->mpWorstEnemyBody) < mpEnemySoldier->mfAttackDistance)
		{
			float fHeight = mpMover->GetCharBody()->GetPosition().y -
				mpEnemySoldier->mpWorstEnemyBody->GetPosition().y;

			//Enemy is above
			if (fHeight < 0)
			{
				fHeight += mpMover->GetCharBody()->GetSize().y / 2.0f;
				float fMax = mpEnemySoldier->mvAttackDamageSize.y;///2.0f;
				if (fHeight > -fMax)
				{
					mpEnemy->ChangeState(STATE_ATTACK);
				}
				else
				{
					//random attack if enemy is not too far up.
					if (cMath::RandRectf(0, 1) < 0.2f)//fHeight*2 > -fMax && 
						mpEnemy->ChangeState(STATE_ATTACK);
					else
						mpEnemy->ChangeState(STATE_FLEE);
				}
			}
			else
			{
				mpEnemy->ChangeState(STATE_ATTACK);
			}


		}
	}
	////////////////////////////////
	//Update path search
	else if (mbFreeEnemyPath == false)
	{
		if (mbLostPlayer == false && mpMover->IsMoving() == false && mpEnemy->CanSeePlayer() == false)
		{
			mbLostPlayer = true;
			mfLostPlayerCount = mfMaxLostPlayerCount;
		}

		if (mbLostPlayer)
		{
			mpMover->GetCharBody()->Move(eCharDir_Forward, 1.0f, afTimeStep);

			mfLostPlayerCount -= afTimeStep;
			if (mfLostPlayerCount <= 0 || mpMover->GetStuckCounter() > 0.5f)
			{
				mpEnemy->ChangeState(STATE_IDLE);
			}
		}
	}
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Hunt::OnSeePlayer(const cVector3f& avPosition, float afChance)
{
	if (mbLostPlayer && afChance >= mpEnemySoldier->mfHuntMinSeeChance)
	{
		mbLostPlayer = false;
		mfUpdatePathCount = 0;
	}
}

//-----------------------------------------------------------------------

bool cGameEnemyState_Soldier_Hunt::OnHearNoise(const cVector3f& avPosition, float afVolume)
{
	//////////////////////////////////
	//If player is lost the sound might be of help
	if (mbLostPlayer)
	{
		//Check if sound can be heard
		if (afVolume >= mpEnemySoldier->mfHuntMinHearVolume)
		{
			//Check if a node is found near the sound.
			cAINode* pNode = mpMover->GetAINodeAtPosInRange(avPosition, 0.0f, 5.0f, true, 0.1f);
			if (pNode)
			{
				//Update last player postion.
				mbLostPlayer = false;
				mfUpdatePathCount = 0;
				mpEnemy->SetLastPlayerPos(pNode->GetPosition());

				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Hunt::OnDraw()
{
	float fWantedSpeed = mpMover->GetCharBody()->GetMoveSpeed(eCharDir_Forward);
	float fRealSpeed = cMath::Vector3Dist(mpMover->GetCharBody()->GetPosition(),
		mpMover->GetCharBody()->GetLastPosition());
	fRealSpeed = fRealSpeed / (1.0f / 60.0f);

	float fDist = mpMover->DistanceToChar2D(mpEnemySoldier->mpWorstEnemyBody);

	mpInit->mpDefaultFont->Draw(cVector3f(0, 110, 100), 14, cColor(1, 1, 1, 1), eFontAlign_Left,
		_W("LostPlayerCount: %f FreePath: %d NoPath: %d MaxStuck: %d Dist: %f / %f"),
		mfLostPlayerCount, mbFreeEnemyPath,
		mbFoundNoPath,
		mlStuckAtMaxCount,
		fDist,
		mpEnemySoldier->mfAttackDistance);
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// ATTACK STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Attack::OnEnterState(iGameEnemyState* apPrevState)
{
	///////////////
	//Setup body
	mpEnemy->SetupBody();
	if (mpEnemySoldier->mfAttackSpeed > 0)
	{
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward, mpEnemySoldier->mfAttackSpeed);
		mpMover->SetMaxTurnSpeed(10000.0f);

		//mpMover->SetMinBreakAngle(cMath::ToRad(140));
	}

	///////////////
	//Animation
	float fHeight = mpPlayer->GetCharacterBody()->GetPosition().y -
		mpMover->GetCharBody()->GetPosition().y;

	//Player is above
	if (fHeight > 0.1f)
		mpEnemy->PlayAnim("Attack", false, 0.2f);
	else
		mpEnemy->PlayAnim("AttackLow", false, 0.2f);

	///////////////
	//Other
	mpEnemySoldier->PlaySound(mpEnemySoldier->msAttackStartSound);

	mfDamageTimer = mpEnemySoldier->mfAttackDamageTime;
	mfJumpTimer = mpEnemySoldier->mfAttackJumpTime;
	mbAttacked = false;

	if (mpEnemy->GetOnAttackCallback() != "")
	{
		tString sCommand = mpEnemy->GetOnAttackCallback() + "(\"" + mpEnemy->GetName() + "\")";
		mpInit->RunScriptCommand(sCommand);
	}
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Attack::OnLeaveState(iGameEnemyState* apNextState)
{
	mpEnemySoldier->SetSkipSoundTriggerCount(2.0f);
	mpMover->ResetStuckCounter();


}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Attack::OnUpdate(float afTimeStep)
{
	//Move forward
	if (mpEnemySoldier->mfAttackSpeed > 0)
	{
		if (mfJumpTimer <= 0)
		{
			mpMover->MoveDirectToPos(mpPlayer->GetCharacterBody()->GetFeetPosition(), afTimeStep);
		}
		else
		{
			mpMover->TurnToPos(mpPlayer->GetCharacterBody()->GetFeetPosition());
			mfJumpTimer -= afTimeStep;
		}
	}

	if (mbAttacked) return;

	//////////////////////////////////////
	//Get the 2D distance to the player
	cVector3f vStart = mpEnemySoldier->mpWorstEnemyBody->GetPosition();
	vStart.y = 0;
	cVector3f vEnd = mpMover->GetCharBody()->GetPosition();
	vEnd.y = 0;
	float fDist2D = cMath::Vector3DistSqr(vStart, vEnd);
	float fMinRange = mpEnemySoldier->mfAttackDamageRange;

	////////////////////////////////////////
	// Check if dog is in range of player
	if (fDist2D <= fMinRange * fMinRange && mfDamageTimer <= 0)
	{
		if (mbAttacked == false)
		{
			cVector3f vPos = mpMover->GetCharBody()->GetPosition() +
				mpMover->GetCharBody()->GetForward() *
				mpEnemySoldier->mfAttackDamageRange;

			cVector3f vRot = cVector3f(0, mpMover->GetCharBody()->GetYaw(), 0);
			cMatrixf mtxOffset = cMath::MatrixRotate(vRot, eEulerRotationOrder_XYZ);
			mtxOffset.SetTranslation(vPos);

			eAttackTargetFlag target = eAttackTargetFlag_Player | eAttackTargetFlag_Bodies | eAttackTargetFlag_Enemy;

			mpInit->mpPlayer->mbDamageFromPos = true;
			mpInit->mpPlayer->mvDamagePos = mpMover->GetCharBody()->GetPosition();
			if (mpInit->mpAttackHandler->CreateShapeAttack(mpEnemySoldier->GetAttackShape(),
				mtxOffset,
				mpMover->GetCharBody()->GetPosition(),
				cMath::RandRectf(mpEnemySoldier->mfAttackMinDamage,
					mpEnemySoldier->mfAttackMaxDamage),

				mpEnemySoldier->mfAttackMinMass, mpEnemySoldier->mfAttackMaxMass,
				mpEnemySoldier->mfAttackMinImpulse, mpEnemySoldier->mfAttackMaxImpulse,

				mpEnemySoldier->mlAttackStrength,

				target, NULL))
			{
				mpEnemySoldier->PlaySound(mpEnemySoldier->msAttackHitSound);
			}
			mpInit->mpPlayer->mbDamageFromPos = false;
			mbAttacked = true;
		}
	}
	else
	{
		mfDamageTimer -= afTimeStep;
	}

}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Attack::OnAnimationOver(const tString& asName)
{
	if (mpPlayer->GetHealth() <= 0)
	{
		float fDist = mpMover->DistanceToChar2D(mpInit->mpPlayer->GetCharacterBody());
		if (fDist < 2.3f)
		{
			//mpEnemy->SetTempFloat(60.0f);
			mpEnemy->ChangeState(STATE_IDLE);
		}
		else
		{
			mpEnemy->ChangeState(STATE_FLEE);
		}
	}
	else
	{
		mpEnemy->ChangeState(STATE_FLEE);
	}
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Attack::OnPostSceneDraw()
{
	cCamera3D* pCamera = static_cast<cCamera3D*>(mpInit->mpGame->GetScene()->GetCamera());

	cVector3f vPos = mpMover->GetCharBody()->GetPosition() +
		mpMover->GetCharBody()->GetForward() *
		mpEnemySoldier->mfAttackDamageRange;

	cVector3f vRot = cVector3f(0, mpMover->GetCharBody()->GetYaw(), 0);
	cMatrixf mtxOffset = cMath::MatrixRotate(vRot, eEulerRotationOrder_XYZ);
	mtxOffset.SetTranslation(vPos);


	cMatrixf mtxCollider = cMath::MatrixMul(pCamera->GetViewMatrix(), mtxOffset);

	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetMatrix(eMatrix_ModelView, mtxCollider);

	cVector3f vSize = mpEnemySoldier->GetAttackShape()->GetSize();
	mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(vSize * 0.5f, vSize * -0.5f,
		cColor(1, 0, 1, 1));
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// FLEE STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Flee::OnEnterState(iGameEnemyState* apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();
	mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward, mpEnemySoldier->mfHuntSpeed);
	mpMover->GetCharBody()->SetMaxNegativeMoveSpeed(eCharDir_Forward, -mpEnemySoldier->mfFleeBackSpeed);

	float fPosMul = 1;
	if (apPrevState->GetId() == STATE_KNOCKDOWN) fPosMul = 4.0f;

	mbBackingFromBreakDoor = false;
	if (apPrevState->GetId() == STATE_BREAKDOOR)
		mbBackingFromBreakDoor = true;

	///////////////////////////////////////
	// The dog has just broken a door
	if (mbBackingFromBreakDoor)
	{
		mfBackAngle = mpMover->GetCharBody()->GetYaw();
		mbBackwards = true;

		mfTimer = mpEnemySoldier->mfFleeBackTime;
		mfCheckBehindTime = 1.0f / 10.0f;
	}
	///////////////////////////////////////
	// Normal flee
	else
	{
		if ((apPrevState->GetId() == STATE_KNOCKDOWN || apPrevState->GetId() == STATE_HUNT ||
			cMath::RandRectf(0, 1) < 0)//mpEnemySoldier->mfFleePositionChance) 
			)
		{
			cAINode* pNode = mpMover->GetAINodeInRange(mpEnemySoldier->mfFleePositionMinDistance * fPosMul,
				mpEnemySoldier->mfFleePositionMaxDistance * fPosMul);
			if (pNode)
			{
				mpMover->MoveToPos(pNode->GetPosition());
			}
			else
			{
				mpEnemy->ChangeState(STATE_HUNT);
			}
			mfTimer = mpEnemySoldier->mfFleePositionMaxTime;
			mbBackwards = false;
		}
		else if (cMath::RandRectf(0, 1) < mpEnemySoldier->mfFleeBackChance)
		{
			mfBackAngle = mpMover->GetCharBody()->GetYaw();
			mbBackwards = true;

			mfTimer = mpEnemySoldier->mfFleeBackTime;
			mfCheckBehindTime = 1.0f / 10.0f;
		}
		else
		{
			mpEnemy->ChangeState(STATE_HUNT);
		}
	}
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Flee::OnLeaveState(iGameEnemyState* apNextState)
{
	mpMover->ResetStuckCounter();
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Flee::OnUpdate(float afTimeStep)
{
	mfTimer -= afTimeStep;

	if (mbBackwards)
	{
		if (mfCheckBehindTime <= 0)
		{
			mfCheckBehindTime = 1.0f / 20.0f;

			bool bHit = mpEnemy->GetGroundFinder()->GetGround(mpMover->GetCharBody()->GetPosition(),
				mpMover->GetCharBody()->GetForward() * -1,
				NULL, NULL, 1.9f);
			if (bHit)
			{
				if (mbBackingFromBreakDoor)
				{
					if (mlPreviousState == STATE_FLEE)
						mpEnemy->ChangeState(STATE_HUNT);
					else
						mpEnemy->ChangeState(mlPreviousState);
				}
				else
				{
					mpEnemy->ChangeState(STATE_HUNT);
				}
			}
		}
		else
		{
			mfCheckBehindTime -= afTimeStep;
		}

		if (mfTimer <= 0)
		{
			if (mbBackingFromBreakDoor)
				mpEnemy->ChangeState(mlPreviousState);
			else
				mpEnemy->ChangeState(STATE_HUNT);
		}
		mpMover->GetCharBody()->Move(eCharDir_Forward, -1.0f, afTimeStep);
		mpMover->TurnToPos(mpEnemySoldier->mpWorstEnemyBody->GetFeetPosition());
	}
	else
	{
		//Move forward
		if (mpMover->IsMoving() == false || mpMover->GetStuckCounter() > 0.3f || mfTimer <= 0)
		{
			//Check if there is any enemies nearaby and if anyone is allready fighting
			if (mpEnemy->CheckForTeamMate(mpEnemySoldier->mfCallBackupRange * 1.5f, false) &&
				mpEnemy->CheckForTeamMate(8, true) == false)
			{
				float fEnemyDist = mpMover->DistanceToChar(mpEnemySoldier->mpWorstEnemyBody);
				//Log("Dist: %f\n",fPlayerDist);
				if (fEnemyDist > 8)
				{
					mpEnemy->ChangeState(STATE_CALLBACKUP);
				}
				else
				{
					mpEnemy->ChangeState(STATE_HUNT);
				}
				//Log("Back from flee!!\n");
			}
			else
			{
				//if(mpInit->mpMusicHandler->AttackerExist(mpEnemy))
				{
					mpEnemy->ChangeState(STATE_HUNT);
				}
				//else
				//{
				//	mpEnemy->ChangeState(STATE_IDLE);
				//}
			}
		}
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CALL BACKUP STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_CallBackup::OnEnterState(iGameEnemyState* apPrevState)
{
	//Animation
	mpEnemy->PlayAnim(mpEnemySoldier->msCallBackupAnimation, false, 0.2f);

	//Sound
	mpEnemySoldier->PlaySound(mpEnemySoldier->msCallBackupSound);

	//Iterate enemies and show them the player
	cVector3f vPostion = mpMover->GetCharBody()->GetFeetPosition();

	tGameEnemyIterator it = mpInit->mpMapHandler->GetGameEnemyIterator();
	while (it.HasNext())
	{
		iGameEnemy* pEnemy = it.Next();
		if (pEnemy->GetEnemyType() != mpEnemy->GetEnemyType()) continue;
		if (pEnemy == mpEnemy || pEnemy->IsActive() == false || pEnemy->GetHealth() <= 0) continue;

		cGameEnemy_Soldier* pSoldier = static_cast<cGameEnemy_Soldier*>(pEnemy);

		float fDist = cMath::Vector3Dist(pSoldier->GetMover()->GetCharBody()->GetPosition(),
			vPostion);

		if (fDist <= mpEnemySoldier->mfCallBackupRange)
		{
			pSoldier->ShowPlayer(mpEnemySoldier->GetLastPlayerPos()); // Will be enemy location instead if mbAlly is true
			// break; // Call only for one dog backup! Is what would happen if this is a dog... but no, it's a soldier, smarter than a dog!
		}
	}

	mpMover->Stop();
	mpMover->GetCharBody()->SetMoveSpeed(eCharDir_Forward, 0);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_CallBackup::OnLeaveState(iGameEnemyState* apNextState)
{
	mpMover->ResetStuckCounter();
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_CallBackup::OnUpdate(float afTimeStep)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_CallBackup::OnAnimationOver(const tString& asName)
{
	mpEnemy->ChangeState(STATE_HUNT);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// BREAK DOOR STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_BreakDoor::OnEnterState(iGameEnemyState* apPrevState)
{
	//Setup body
	mpEnemy->SetupBody();
	if (mpEnemySoldier->mfBreakDoorSpeed > 0)
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward, mpEnemySoldier->mfBreakDoorSpeed);


	//Animation
	mpEnemy->PlayAnim(mpEnemySoldier->msBreakDoorAnimation, false, 0.2f);

	mpEnemySoldier->PlaySound(mpEnemySoldier->msBreakDoorStartSound);

	mfDamageTimer = mpEnemySoldier->mfBreakDoorDamageTime;
	mfStopMoveTimer = mpEnemySoldier->mfBreakDoorDamageTime + 1.1f;
	mbAttacked = false;
	mbStopped = false;

	mlCount = 0;
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_BreakDoor::OnLeaveState(iGameEnemyState* apNextState)
{
	mpEnemySoldier->SetSkipSoundTriggerCount(2.0f);
	mpMover->ResetStuckCounter();
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_BreakDoor::OnUpdate(float afTimeStep)
{
	//Move forward
	if (mpEnemySoldier->mfBreakDoorSpeed > 0 && mlCount == 0 && mbAttacked == false)
	{
		//Skip this for now
		//mpMover->GetCharBody()->Move(eCharDir_Forward,1.0f,afTimeStep);
	}

	if (mfDamageTimer <= 0)
	{
		if (!mbStopped)
		{
			mpMover->Stop();
			mbStopped = true;
		}
	}
	else
	{
		mfDamageTimer -= afTimeStep;
	}

	if (mfDamageTimer <= 0)
	{
		if (mbAttacked == false)
		{
			cVector3f vPos = mpMover->GetCharBody()->GetPosition() +
				mpMover->GetCharBody()->GetForward() *
				mpEnemySoldier->mfBreakDoorDamageRange;

			cVector3f vRot = cVector3f(0, mpMover->GetCharBody()->GetYaw(), 0);
			cMatrixf mtxOffset = cMath::MatrixRotate(vRot, eEulerRotationOrder_XYZ);
			mtxOffset.SetTranslation(vPos);

			eAttackTargetFlag target = eAttackTargetFlag_Player | eAttackTargetFlag_Bodies;

			if (mpInit->mpAttackHandler->CreateShapeAttack(mpEnemySoldier->GetBreakDoorShape(),
				mtxOffset,
				mpMover->GetCharBody()->GetPosition(),
				cMath::RandRectf(mpEnemySoldier->mfBreakDoorMinDamage,
					mpEnemySoldier->mfBreakDoorMaxDamage),

				mpEnemySoldier->mfBreakDoorMinMass, mpEnemySoldier->mfBreakDoorMaxMass,
				mpEnemySoldier->mfBreakDoorMinImpulse, mpEnemySoldier->mfBreakDoorMaxImpulse,

				mpEnemySoldier->mlBreakDoorStrength,

				target, NULL))
			{
				mpEnemySoldier->PlaySound(mpEnemySoldier->msBreakDoorHitSound);

				cGameSwingDoor* pDoor = mpInit->mpAttackHandler->GetLastSwingDoor();
				if (pDoor)
				{
					/////////////////////////////
					//The door is unbreakable
					if (pDoor->GetToughness() - mpEnemySoldier->mlBreakDoorStrength >= 4)
					{
						cMatrixf mtxDoor = pDoor->GetBody(0)->GetWorldMatrix();
						cMatrixf mtxInvDoor = cMath::MatrixInverse(mtxDoor);

						cVector3f vDoorForward = mtxInvDoor.GetForward();
						cVector3f vEnemyForward = mpMover->GetCharBody()->GetForward();

						if (cMath::Vector3Dot(vDoorForward, vEnemyForward) < 0)
						{
							mpEnemy->AddDoorBreakCount(2);//8);
							//mpInit->mpEffectHandler->GetSubTitle()->Add("Cannot break this door! On BAD side",4);
						}
						else
						{
							mpEnemy->AddDoorBreakCount(2);
							//Just continue hitting it since it is just barricaded.
							//mpInit->mpEffectHandler->GetSubTitle()->Add("Cannot break this door! On GOOD side",4);
						}
					}
					/////////////////////////////
					//The door is breakable
					else
					{
						mpEnemy->AddDoorBreakCount(2);
					}

				}

			}
			mbAttacked = true;
		}
	}
	else
	{
		mfDamageTimer -= afTimeStep;
	}

}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_BreakDoor::OnAnimationOver(const tString& asName)
{
	if (mlCount == 0)
	{
		if (mpEnemySoldier->mbBreakDoorRiseAtEnd)
		{
			mpEnemy->PlayAnim("RiseRight", false, 0.2f);
			mlCount++;
		}
		else
		{
			//mpEnemy->ChangeState(mlPreviousState);
			mpEnemy->ChangeState(STATE_FLEE);
			mpEnemy->GetState(STATE_FLEE)->SetPreviousState(mlPreviousState);
		}
	}
	else
	{
		//mpEnemy->ChangeState(mlPreviousState);
		mpEnemy->ChangeState(STATE_FLEE);
		mpEnemy->GetState(STATE_FLEE)->SetPreviousState(mlPreviousState);
	}
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_BreakDoor::OnPostSceneDraw()
{
	cCamera3D* pCamera = static_cast<cCamera3D*>(mpInit->mpGame->GetScene()->GetCamera());

	cVector3f vPos = mpMover->GetCharBody()->GetPosition() +
		mpMover->GetCharBody()->GetForward() *
		mpEnemySoldier->mfBreakDoorDamageRange;

	cVector3f vRot = cVector3f(0, mpMover->GetCharBody()->GetYaw(), 0);
	cMatrixf mtxOffset = cMath::MatrixRotate(vRot, eEulerRotationOrder_XYZ);
	mtxOffset.SetTranslation(vPos);


	cMatrixf mtxCollider = cMath::MatrixMul(pCamera->GetViewMatrix(), mtxOffset);

	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetMatrix(eMatrix_ModelView, mtxCollider);

	cVector3f vSize = mpEnemySoldier->GetBreakDoorShape()->GetSize();
	mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(vSize * 0.5f, vSize * -0.5f,
		cColor(1, 0, 1, 1));
}

//-----------------------------------------------------------------------



//////////////////////////////////////////////////////////////////////////
// KNOCK DOWN STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_KnockDown::OnEnterState(iGameEnemyState* apPrevState)
{
	//Animation
	mpEnemy->PlayAnim("Idle", true, 0.7f);

	//Sound
	mpEnemy->PlaySound(mpEnemySoldier->msKnockDownSound);

	//Setup body
	mpEnemy->SetupBody();

	//Go to rag doll
	mpEnemy->GetMeshEntity()->AlignBodiesToSkeleton(false);
	mpEnemy->GetMeshEntity()->SetSkeletonPhysicsActive(true);
	mpEnemy->GetMeshEntity()->Stop();

	mpEnemy->GetMover()->GetCharBody()->SetEntity(NULL);
	mpEnemy->GetMover()->GetCharBody()->SetActive(false);

	mpEnemy->GetMover()->Stop();

	mfTimer = 2.0f;
	mbCheckAnim = false;

	mlPrevToughness = mpEnemy->GetToughness();
	mpEnemy->SetToughness(12);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_KnockDown::OnLeaveState(iGameEnemyState* apNextState)
{
	mpEnemy->SetToughness(mlPrevToughness);

	mpEnemy->GetMover()->GetCharBody()->SetEntity(mpEnemy->GetMeshEntity());
	mpEnemy->GetMover()->GetCharBody()->SetActive(true);
	mpMover->ResetStuckCounter();
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_KnockDown::OnUpdate(float afTimeStep)
{
	if (mbCheckAnim)
	{
		iCharacterBody* pCharBody = mpEnemy->GetMover()->GetCharBody();
		cBoundingVolume* pBV = pCharBody->GetBody()->GetBoundingVolume();

		////////////////////////////////////////////////
		//Add a force to all objects around dog.
		iPhysicsWorld* pWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

		std::list<iPhysicsBody*> lstBodies;
		cPhysicsBodyIterator bodyIt = pWorld->GetBodyIterator();
		while (bodyIt.HasNext())
		{
			lstBodies.push_back(bodyIt.Next());
		}

		//////////////////////////
		//Force Iteration
		std::list<iPhysicsBody*>::iterator it = lstBodies.begin();
		for (; it != lstBodies.end(); ++it)
		{
			iPhysicsBody* pBody = *it;

			if (pBody->GetCollideCharacter() == false) continue;
			if (pBody->IsActive() == false) continue;
			if (pBody == pCharBody->GetBody()) continue;

			if (cMath::CheckCollisionBV(*pBody->GetBoundingVolume(), *pBV))
			{
				cVector3f vDir = pBody->GetWorldPosition() - pCharBody->GetPosition();
				float fLength = vDir.Length();
				//vDir.y *= 0.1f;
				//if(vDir.x ==0 && vDir.z ==0) vDir.x = 0.3f;
				vDir.Normalise();

				if (fLength == 0) fLength = 0.001f;
				float fForce = (1 / fLength) * 2;
				if (fForce > 300) fForce = 300;
				if (mpInit->mpPlayer->GetState() == ePlayerState_Grab &&
					mpInit->mpPlayer->GetPushBody() == pBody)
				{
					fForce *= 40;
				}

				if (pBody->IsCharacter())
				{
					pBody->GetCharacterBody()->AddForce(vDir * fForce * 10 *
						pBody->GetCharacterBody()->GetMass());
				}
				else
				{
					pBody->AddForce(vDir * fForce * pBody->GetMass());
				}
			}
		}
	}
	else
	{
		mfTimer -= afTimeStep;

		if (mfTimer <= 0)
		{
			//Get the forward vector from root bone (the right vector)
			cNodeIterator StateIt = mpEnemy->GetMeshEntity()->GetRootNode()->GetChildIterator();
			cBoneState* pBoneState = static_cast<cBoneState*>(StateIt.Next());

			cVector3f vRight = cMath::MatrixInverse(pBoneState->GetWorldMatrix()).GetForward();

			//Play animation and fade physics
			float fFadeTime = 1.0f;
			mbCheckAnim = true;

			mpEnemy->GetMeshEntity()->Stop();
			if (cMath::Vector3Dot(vRight, cVector3f(0, 1, 0)) < 0)
				mpEnemy->PlayAnim("RiseRight", false, fFadeTime);
			else
				mpEnemy->PlayAnim("RiseLeft", false, fFadeTime);

			mpEnemy->GetMeshEntity()->FadeSkeletonPhysicsWeight(fFadeTime);

			//Calculate values
			cVector3f vPosition;
			cVector3f vAngles;
			cMatrixf mtxTransform = mpEnemy->GetMeshEntity()->CalculateTransformFromSkeleton(&vPosition, &vAngles);

			//Seems to work better...
			vPosition = mpEnemy->GetMeshEntity()->GetBoundingVolume()->GetWorldCenter();
			cVector3f vGroundPos = vPosition;

			bool bFoundGround = mpEnemy->GetGroundFinder()->GetGround(vPosition, cVector3f(0, -1, 0), &vGroundPos, NULL);

			//Log("Found ground: %d | %s -> %s\n",bFoundGround,vPosition.ToString().c_str(),
			//									vGroundPos.ToString().c_str());

			//Set body
			iCharacterBody* pCharBody = mpEnemy->GetMover()->GetCharBody();

			//vGroundPos -= pCharBody->GetEntityOffset().GetTranslation();
			float fYAngle = vAngles.y - mpEnemy->GetModelOffsetAngles().y;
			//cVector3f vAdd = cVector3f(0,0,pCharBody->GetEntityOffset().GetTranslation().z);
			//vAdd = cMath::MatrixMul(cMath::MatrixRotateY(fYAngle),vAdd);
			//vGroundPos += vAdd;
			pCharBody->SetFeetPosition(vGroundPos);
			pCharBody->SetYaw(fYAngle);
			pCharBody->SetEntity(mpEnemy->GetMeshEntity());
			pCharBody->SetActive(true);
			//pCharBody->Update(1.0f / 60.0f);
			//pCharBody->SetActive(false);

			for (int i = 0; i < 3; ++i)
			{
				pCharBody->Update(1.0f / 60.0f);

				mpEnemy->GetMeshEntity()->UpdateLogic(1.0f / 60.0f);
				mpEnemy->GetMeshEntity()->UpdateGraphics(NULL, 1.0f / 60.0f, NULL);
			}
		}
	}
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_KnockDown::OnAnimationOver(const tString& asName)
{
	iCharacterBody* pCharBody = mpEnemy->GetMover()->GetCharBody();

	if (mpEnemy->CheckForTeamMate(mpEnemySoldier->mfCallBackupRange * 1.5f, false) &&
		mpEnemy->CheckForTeamMate(14, true) == false)
	{
		pCharBody->SetActive(true);
		mpEnemy->ChangeState(STATE_FLEE);
	}
	else
	{
		pCharBody->SetActive(true);
		//mpEnemy->ChangeState(STATE_HUNT);
		mpEnemy->ChangeState(STATE_FLEE);
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// DEAD STATE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Dead::OnEnterState(iGameEnemyState* apPrevState)
{
	//Animation
	mpEnemy->PlayAnim("Idle", true, 0.7f);

	//Sound
	if (mpEnemy->IsLoading() == false)
		mpEnemy->PlaySound(mpEnemySoldier->msDeathSound);

	//Setup body
	mpEnemy->SetupBody();

	//Go to ragdoll
	if (mpEnemy->IsLoading() == false)
		mpEnemy->GetMeshEntity()->AlignBodiesToSkeleton(false);

	mpEnemy->GetMeshEntity()->SetSkeletonPhysicsActive(true);
	mpEnemy->GetMeshEntity()->Stop();

	mpEnemy->GetMover()->GetCharBody()->SetEntity(NULL);
	mpEnemy->GetMover()->GetCharBody()->SetActive(false);

	mpEnemy->GetMover()->Stop();

	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Dead::OnLeaveState(iGameEnemyState* apNextState)
{
}

//-----------------------------------------------------------------------

void cGameEnemyState_Soldier_Dead::OnUpdate(float afTimeStep)
{
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cGameEnemy_Soldier::cGameEnemy_Soldier(cInit* apInit, const tString& asName, TiXmlElement* apGameElem) : iGameEnemy(apInit, asName, apGameElem)
{
	LoadBaseProperties(apGameElem);

	//////////////////////////////
	//Special properties 

	mfLengthBodyToAss = cString::ToFloat(apGameElem->Attribute("LengthBodyToAss"), 0.5f);

	mfMinKnockDamage = cString::ToFloat(apGameElem->Attribute("MinKnockDamage"), 0);
	mfCertainKnockDamage = cString::ToFloat(apGameElem->Attribute("CertainKnockDamage"), 0);


	//////////////////////////////
	//State properties
	mfIdleFOV = cMath::ToRad(cString::ToFloat(apGameElem->Attribute("IdleFOV"), 0));
	msIdleFoundPlayerSound = cString::ToString(apGameElem->Attribute("IdleFoundPlayerSound"), "");
	mfIdleMinSeeChance = cString::ToFloat(apGameElem->Attribute("IdleMinSeeChance"), 0);
	mfIdleMinHearVolume = cString::ToFloat(apGameElem->Attribute("IdleMinHearVolume"), 0);
	msIdleSound = cString::ToString(apGameElem->Attribute("IdleSound"), "");
	mfIdleSoundMinInteraval = cString::ToFloat(apGameElem->Attribute("IdleSoundMinInteraval"), 0);
	mfIdleSoundMaxInterval = cString::ToFloat(apGameElem->Attribute("IdleSoundMaxInteraval"), 0);
	mfIdleCallBackupChance = cString::ToFloat(apGameElem->Attribute("IdleCallBackupChance"), 0);
	mvPreloadSounds.push_back(msIdleSound);

	msInvestigateSound = cString::ToString(apGameElem->Attribute("InvestigateSound"), "");
	mfInvestigateTime = cString::ToFloat(apGameElem->Attribute("InvestigateTime"), 0);
	mvPreloadSounds.push_back(msInvestigateSound);

	msAttentionSound = cString::ToString(apGameElem->Attribute("AttentionSound"), "");
	mfAttentionTime = cString::ToFloat(apGameElem->Attribute("AttentionTime"), 0);
	mfAttentionMinDist = cString::ToFloat(apGameElem->Attribute("AttentionMinDist"), 0);
	mvPreloadSounds.push_back(msAttentionSound);

	mfHuntFOV = cMath::ToRad(cString::ToFloat(apGameElem->Attribute("HuntFOV"), 0));
	mfHuntSpeed = cString::ToFloat(apGameElem->Attribute("HuntSpeed"), 0);
	mfHuntForLostPlayerTime = cString::ToFloat(apGameElem->Attribute("HuntForLostPlayerTime"), 0);
	mfHuntMinSeeChance = cString::ToFloat(apGameElem->Attribute("IdleMinSeeChance"), 0);
	mfHuntMinHearVolume = cString::ToFloat(apGameElem->Attribute("IdleMinHearVolume"), 0);

	mfAttackDistance = cString::ToFloat(apGameElem->Attribute("AttackDistance"), 0);
	mfAttackSpeed = cString::ToFloat(apGameElem->Attribute("AttackSpeed"), 0);
	mfAttackJumpTime = cString::ToFloat(apGameElem->Attribute("AttackJumpTime"), 0);
	mfAttackDamageTime = cString::ToFloat(apGameElem->Attribute("AttackDamageTime"), 0);
	mvAttackDamageSize = cString::ToVector3f(apGameElem->Attribute("AttackDamageSize"), 0);
	mfAttackDamageRange = cString::ToFloat(apGameElem->Attribute("AttackDamageRange"), 0);
	mfAttackMinDamage = cString::ToFloat(apGameElem->Attribute("AttackMinDamage"), 0);
	mfAttackMaxDamage = cString::ToFloat(apGameElem->Attribute("AttackMaxDamage"), 0);
	msAttackStartSound = cString::ToString(apGameElem->Attribute("AttackStartSound"), "");
	msAttackHitSound = cString::ToString(apGameElem->Attribute("AttackHitSound"), "");
	mfAttackMinMass = cString::ToFloat(apGameElem->Attribute("AttackMinMass"), 0);
	mfAttackMaxMass = cString::ToFloat(apGameElem->Attribute("AttackMaxMass"), 0);
	mfAttackMinImpulse = cString::ToFloat(apGameElem->Attribute("AttackMinImpulse"), 0);
	mfAttackMaxImpulse = cString::ToFloat(apGameElem->Attribute("AttackMaxImpulse"), 0);
	mlAttackStrength = cString::ToInt(apGameElem->Attribute("AttackStrength"), 0);
	mvPreloadSounds.push_back(msAttackStartSound);
	mvPreloadSounds.push_back(msAttackHitSound);

	msBreakDoorAnimation = cString::ToString(apGameElem->Attribute("BreakDoorAnimation"), "");
	mfBreakDoorSpeed = cString::ToFloat(apGameElem->Attribute("BreakDoorSpeed"), 0);
	mfBreakDoorDamageTime = cString::ToFloat(apGameElem->Attribute("BreakDoorDamageTime"), 0);
	mvBreakDoorDamageSize = cString::ToVector3f(apGameElem->Attribute("BreakDoorDamageSize"), 0);
	mfBreakDoorDamageRange = cString::ToFloat(apGameElem->Attribute("BreakDoorDamageRange"), 0);
	mfBreakDoorMinDamage = cString::ToFloat(apGameElem->Attribute("BreakDoorMinDamage"), 0);
	mfBreakDoorMaxDamage = cString::ToFloat(apGameElem->Attribute("BreakDoorMaxDamage"), 0);
	msBreakDoorStartSound = cString::ToString(apGameElem->Attribute("BreakDoorStartSound"), "");
	msBreakDoorHitSound = cString::ToString(apGameElem->Attribute("BreakDoorHitSound"), "");
	mfBreakDoorMinMass = cString::ToFloat(apGameElem->Attribute("BreakDoorMinMass"), 0);
	mfBreakDoorMaxMass = cString::ToFloat(apGameElem->Attribute("BreakDoorMaxMass"), 0);
	mfBreakDoorMinImpulse = cString::ToFloat(apGameElem->Attribute("BreakDoorMinImpulse"), 0);
	mfBreakDoorMaxImpulse = cString::ToFloat(apGameElem->Attribute("BreakDoorMaxImpulse"), 0);
	mlBreakDoorStrength = cString::ToInt(apGameElem->Attribute("BreakDoorStrength"), 0);
	mbBreakDoorRiseAtEnd = cString::ToBool(apGameElem->Attribute("BreakDoorRiseAtEnd"), false);
	mvPreloadSounds.push_back(msBreakDoorStartSound);
	mvPreloadSounds.push_back(msBreakDoorHitSound);

	msKnockDownSound = cString::ToString(apGameElem->Attribute("KnockDownSound"), "");
	mvPreloadSounds.push_back(msKnockDownSound);

	msDeathSound = cString::ToString(apGameElem->Attribute("DeathSound"), "");
	mvPreloadSounds.push_back(msDeathSound);

	mfFleePositionChance = cString::ToFloat(apGameElem->Attribute("FleePositionChance"), 0);
	mfFleePositionMaxTime = cString::ToFloat(apGameElem->Attribute("FleePositionMaxTime"), 0);
	mfFleePositionMinDistance = cString::ToFloat(apGameElem->Attribute("FleePositionMinDistance"), 0);
	mfFleePositionMaxDistance = cString::ToFloat(apGameElem->Attribute("FleePositionMaxDistance"), 0);
	mfFleeBackChance = cString::ToFloat(apGameElem->Attribute("FleeBackChance"), 0);
	mfFleeBackTime = cString::ToFloat(apGameElem->Attribute("FleeBackTime"), 0);
	mfFleeBackSpeed = cString::ToFloat(apGameElem->Attribute("FleeBackSpeed"), 0);

	msCallBackupAnimation = cString::ToString(apGameElem->Attribute("CallBackupAnimation"), "");
	msCallBackupSound = cString::ToString(apGameElem->Attribute("CallBackupSound"), "");
	mfCallBackupRange = cString::ToFloat(apGameElem->Attribute("CallBackupRange"), 0);
	mvPreloadSounds.push_back(msCallBackupSound);


	//////////////////////////////
	//Set up states
	AddState(hplNew(cGameEnemyState_Soldier_Idle, (STATE_IDLE, mpInit, this)));
	AddState(hplNew(cGameEnemyState_Soldier_Hunt, (STATE_HUNT, mpInit, this)));
	AddState(hplNew(cGameEnemyState_Soldier_Attack, (STATE_ATTACK, mpInit, this)));
	AddState(hplNew(cGameEnemyState_Soldier_Flee, (STATE_FLEE, mpInit, this)));
	AddState(hplNew(cGameEnemyState_Soldier_KnockDown, (STATE_KNOCKDOWN, mpInit, this)));
	AddState(hplNew(cGameEnemyState_Soldier_Dead, (STATE_DEAD, mpInit, this)));
	AddState(hplNew(cGameEnemyState_Soldier_Patrol, (STATE_PATROL, mpInit, this)));
	AddState(hplNew(cGameEnemyState_Soldier_Investigate, (STATE_INVESTIGATE, mpInit, this)));
	AddState(hplNew(cGameEnemyState_Soldier_BreakDoor, (STATE_BREAKDOOR, mpInit, this)));
	AddState(hplNew(cGameEnemyState_Soldier_CallBackup, (STATE_CALLBACKUP, mpInit, this)));
	AddState(hplNew(cGameEnemyState_Soldier_MoveTo, (STATE_MOVETO, mpInit, this)));
	AddState(hplNew(cGameEnemyState_Soldier_Attention, (STATE_ATTENTION, mpInit, this)));
}

//-----------------------------------------------------------------------

cGameEnemy_Soldier::~cGameEnemy_Soldier()
{

}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameEnemy_Soldier::OnLoad()
{
	//Create attack shape
	iPhysicsWorld* pPhysicsWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	mpAttackShape = pPhysicsWorld->CreateBoxShape(mvAttackDamageSize, NULL);
	mpBreakDoorShape = pPhysicsWorld->CreateBoxShape(mvBreakDoorDamageSize, NULL);

	//Set up shape
	ChangeState(STATE_IDLE);
}

//-----------------------------------------------------------------------

void cGameEnemy_Soldier::OnUpdate(float afTimeStep)
{
	if (IsActive() == false) return;

	///////////////////////////////////
	//Regenerate health:
	if (mfHealth > 0)
	{
		if (mpInit->mDifficulty != eGameDifficulty_Easy &&
			mfHealth <= mfMaxHealth * 0.5f)
		{
			mfHealth += afTimeStep * (10.0f / 60.0f); //10 heal units / min
		}
	}

	///////////////////////////////////
	//Check for ass in wall
	if (mfHealth > 0 && mpMover->GetCharBody()->IsActive())
	{
		float fMaxMove = afTimeStep * 2;
		static int lAssCount = 0;
		lAssCount++;
		if (lAssCount % 2 == 0)
		{
			iCharacterBody* pCharBody = mpMover->GetCharBody();

			cVector3f vPos, vNormal;
			mFindGround.GetGround(pCharBody->GetPosition(), pCharBody->GetForward() * -1,
				&vPos, &vNormal, mfLengthBodyToAss);
			float fDist = cMath::Vector3Dist(pCharBody->GetPosition(), vPos);
			if (fDist < mfLengthBodyToAss)
			{
				float fAdd = mfLengthBodyToAss - fDist;
				if (fAdd > fMaxMove) fAdd = fMaxMove;

				cVector3f vAdd = pCharBody->GetForward() * fAdd;
				pCharBody->SetPosition(pCharBody->GetPosition() + vAdd);
			}
		}
	}
}

//-----------------------------------------------------------------------

void cGameEnemy_Soldier::ShowPlayer(const cVector3f& avPlayerFeetPos)
{
	if (mlCurrentState == STATE_IDLE || mlCurrentState == STATE_PATROL ||
		mlCurrentState == STATE_INVESTIGATE)
	{
		mvLastPlayerPos = avPlayerFeetPos;
		ChangeState(STATE_HUNT);
	}
}

//-----------------------------------------------------------------------

bool cGameEnemy_Soldier::MoveToPos(const cVector3f& avFeetPos)
{
	if (mlCurrentState == STATE_IDLE || mlCurrentState == STATE_PATROL)
	{
		SetTempPosition(avFeetPos);
		ChangeState(STATE_MOVETO);
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------

bool cGameEnemy_Soldier::IsFighting()
{
	if (mfHealth <= 0 || IsActive() == false) return false;
	if (mlCurrentState == STATE_IDLE || mlCurrentState == STATE_PATROL ||
		mlCurrentState == STATE_INVESTIGATE) return false;

	return true;
}

//-----------------------------------------------------------------------
