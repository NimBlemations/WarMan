#include "HudModel_WeaponRanged.h"

#include "Init.h"
#include "Player.h"
#include "PlayerHelper.h"
#include "AttackHandler.h"
#include "GameEntity.h"
#include "GameEnemy.h"
#include "MapHandler.h"
#include "EffectHandler.h"

#include "GlobalInit.h"


//////////////////////////////////////////////////////////////////////////
// MELEE RAY CALLBACK
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cRangedRayCallback::Reset()
{
	mpClosestBody = NULL;
}

//-----------------------------------------------------------------------

bool cRangedRayCallback::OnIntersect(iPhysicsBody* pBody, cPhysicsRayParams* apParams)
{
	if (pBody->GetCollide() == false) return true;
	if (pBody->IsCharacter()) return true;

	if (apParams->mfDist < mfShortestDist || mpClosestBody == NULL)
	{
		mpClosestBody = pBody;
		mfShortestDist = apParams->mfDist;
		mvPosition = apParams->mvPoint;
		mvNormal = apParams->mvNormal;
	}

	return true;
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// HUD MODEL MELEE WEAPON
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cHudModel_WeaponRanged::cHudModel_WeaponRanged() : iHudModel(ePlayerHandType_WeaponRanged)
{
	ResetExtraData();

	if (gpInit->mbHasHaptics)
	{
		mpLowLevelHaptic = gpInit->mpGame->GetHaptic()->GetLowLevel();
		mpHHitForce = mpLowLevelHaptic->CreateSinusWaveForce(cVector3f(0, 1, 0), 0.63f, 5);
		mpHHitForce->SetActive(false);
	}
	else
	{
		mpLowLevelHaptic = NULL;
	}
}

//-----------------------------------------------------------------------

void cHudModel_WeaponRanged::LoadData(TiXmlElement* apRootElem)
{
	////////////////////////////////////////////////
	//Load the MAIN element.
	TiXmlElement* pRangedElem = apRootElem->FirstChildElement("RANGED");
	if (pRangedElem == NULL) {
		Error("Couldn't load RANGED element from XML document\n");
		return;
	}

	mvHapticSize = cString::ToVector3f(pRangedElem->Attribute("HapticSize"), 0);
	mvHapticRot = cString::ToVector3f(pRangedElem->Attribute("HapticRotate"), 0);
	mfHapticScale = cString::ToFloat(pRangedElem->Attribute("HapticScale"), 2);

	mvHapticRot.x = cMath::ToRad(mvHapticRot.x);
	mvHapticRot.y = cMath::ToRad(mvHapticRot.y);
	mvHapticRot.z = cMath::ToRad(mvHapticRot.z);


	mbDrawDebug = cString::ToBool(pRangedElem->Attribute("DrawDebug"), false);

	////////////////////////////////////////////////
	//Go through the ATTACK elements.
	TiXmlElement* pAttackElem = apRootElem->FirstChildElement("ATTACK");
	for (; pAttackElem != NULL; pAttackElem = pAttackElem->NextSiblingElement("ATTACK"))
	{
		cRangedWeaponAttack rangedAttack;

		rangedAttack.mStart = GetPoseFromElem("StartPose", pAttackElem);
		rangedAttack.mEnd = GetPoseFromElem("EndPose", pAttackElem);
		rangedAttack.mfAttackLength = cString::ToFloat(pAttackElem->Attribute("AttackLength"), 0);
		rangedAttack.mfTimeOfAttack = cString::ToFloat(pAttackElem->Attribute("TimeOfAttack"), 0);

		rangedAttack.mfMaxImpulse = cString::ToFloat(pAttackElem->Attribute("MaxImpulse"), 0);
		rangedAttack.mfMinImpulse = cString::ToFloat(pAttackElem->Attribute("MinImpulse"), 0);
		rangedAttack.mfMinMass = cString::ToFloat(pAttackElem->Attribute("MinMass"), 0);
		rangedAttack.mfMaxMass = cString::ToFloat(pAttackElem->Attribute("MaxMass"), 0);
		rangedAttack.mfMinDamage = cString::ToFloat(pAttackElem->Attribute("MinDamage"), 0);
		rangedAttack.mfMaxDamage = cString::ToFloat(pAttackElem->Attribute("MaxDamage"), 0);

		rangedAttack.msShootSound = cString::ToString(pAttackElem->Attribute("ShootSound"), "");
		rangedAttack.msEmptySound = cString::ToString(pAttackElem->Attribute("EmptySound"), "");
		rangedAttack.msHitSound = cString::ToString(pAttackElem->Attribute("HitSound"), "");

		rangedAttack.mvSpinMul = cString::ToVector3f(pAttackElem->Attribute("SpinMul"), 0);

		rangedAttack.mfDamageRange = cString::ToFloat(pAttackElem->Attribute("DamageRange"), 0);
		rangedAttack.mvDamageSize = cString::ToVector3f(pAttackElem->Attribute("DamageSize"), 0);

		rangedAttack.mfAttackRange = cString::ToFloat(pAttackElem->Attribute("AttackRange"), 0);

		rangedAttack.mfAttackSpeed = cString::ToFloat(pAttackElem->Attribute("AttackSpeed"), 0);
		rangedAttack.mlAttackStrength = cString::ToInt(pAttackElem->Attribute("AttackStrength"), 0);

		rangedAttack.msHitPS = cString::ToString(pAttackElem->Attribute("HitPS"), "");
		rangedAttack.mlHitPSPrio = cString::ToInt(pAttackElem->Attribute("HitPSPrio"), 0);

		//Get largest side and use that to make bounding box.
		float fMax = rangedAttack.mvDamageSize.x;
		if (fMax < rangedAttack.mvDamageSize.y) fMax = rangedAttack.mvDamageSize.y;
		if (fMax < rangedAttack.mvDamageSize.z) fMax = rangedAttack.mvDamageSize.z;

		rangedAttack.mBV.SetSize(fMax * kSqrt2f);

		mvAttacks.push_back(rangedAttack);
	}
}

//-----------------------------------------------------------------------

bool cHudModel_WeaponRanged::UpdatePoseMatrix(cMatrixf& aPoseMtx, float afTimeStep)
{
	////////////////////////
	//Idle and waiting for movement
	if (mlAttackState <= 1)
	{
		return false;
	}
	////////////////////
	//Movement
	else
	{
		aPoseMtx = cMath::MatrixSlerp(mfTime, m_mtxPrevPose, m_mtxNextPose, true);

		float fMul = 1.0f;
		//if(mlAttackState == 2 && mpInit->mDifficulty== eGameDifficulty_Easy) fMul = 1.6f;

		mfTime += mfMoveSpeed * afTimeStep * fMul;

		//Attack
		if (mlAttackState == 4 && mfTime >= mvAttacks[mlCurrentAttack].mfTimeOfAttack && mbAttacked == false)
		{
			Attack();
			mbAttacked = true;
		}

		//Time is up
		if (mfTime >= 1.0f)
		{
			mfTime = 1.0f;


			switch (mlAttackState)
			{
			case 2:
				mlAttackState = 3;
				break;
			case 4:
				mlAttackState = 5;
				mbAttacked = false;

				m_mtxPrevPose = m_mtxNextPose;
				m_mtxNextPose = mEquipPose.ToMatrix();

				mfMoveSpeed = 2;
				mfTime = 0;

				break;
			case 5:
				if (mbButtonDown)	mlAttackState = 1;
				else				mlAttackState = 0;

				break;
			}

		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------

void cHudModel_WeaponRanged::OnAttackDown()
{
	if (mfTime <= 0)
	{
		mlCurrentAttack = 0; // I don't really have any other attacks :/
		mfTime = mvAttacks[mlCurrentAttack].mfAttackSpeed;
		Attack();
		mbAttacked = true;

		mbButtonDown = true;
	}
}

//-----------------------------------------------------------------------

void cHudModel_WeaponRanged::OnAttackUp()
{
	mbButtonDown = false;
}

//-----------------------------------------------------------------------

void cHudModel_WeaponRanged::PlaySound(const tString& asSound)
{
	cSoundHandler* pSoundHandler = mpInit->mpGame->GetSound()->GetSoundHandler();

	pSoundHandler->PlayGui(asSound, false, 1.0f);
}

//-----------------------------------------------------------------------

void cHudModel_WeaponRanged::LoadExtraEntites()
{
	iPhysicsWorld* pWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

	for (size_t i = 0; i < mvAttacks.size(); ++i)
	{
		//Attack shapes
		mvAttacks[i].mpCollider = pWorld->CreateBoxShape(mvAttacks[i].mvDamageSize, NULL);

		//Preload particle system
		mpInit->PreloadParticleSystem(mvAttacks[i].msHitPS);

		//Preload sounds
		mpInit->PreloadSoundEntityData(mvAttacks[i].msHitSound);
		mpInit->PreloadSoundEntityData(mvAttacks[i].msShootSound);
		mpInit->PreloadSoundEntityData(mvAttacks[i].msEmptySound);
	}


}

//-----------------------------------------------------------------------

void cHudModel_WeaponRanged::DestroyExtraEntities()
{
	iPhysicsWorld* pWorld = mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

	for (size_t i = 0; i < mvAttacks.size(); ++i)
	{
		if (mvAttacks[i].mpCollider)
			pWorld->DestroyShape(mvAttacks[i].mpCollider);
	}
}

//-----------------------------------------------------------------------

void cHudModel_WeaponRanged::PostSceneDraw()
{
	if (mbDrawDebug == false) return;

	cCamera3D* pCamera = static_cast<cCamera3D*>(mpInit->mpGame->GetScene()->GetCamera());
	float fAttackRange = mvAttacks[mlCurrentAttack].mfAttackRange;

	cVector3f vPos = pCamera->GetPosition() + pCamera->GetForward() * fAttackRange;
	mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawSphere(vPos, 0.1f, cColor(1, 0, 1, 1));

	//return;

	float fDamageRange = mvAttacks[mlCurrentAttack].mfDamageRange;
	cVector3f vCenter = pCamera->GetPosition() + pCamera->GetForward() * fDamageRange;

	cMatrixf mtxDamage = cMath::MatrixRotate(
		cVector3f(pCamera->GetPitch(), pCamera->GetYaw(), pCamera->GetRoll()),
		eEulerRotationOrder_XYZ);
	mtxDamage.SetTranslation(vCenter);

	bool bCollide = false;
	/*{
		cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
		iPhysicsWorld *pPhysicsWorld = pWorld->GetPhysicsWorld();

		bCollide = pPhysicsWorld->CheckShapeWorldCollision(NULL,mvAttacks[mlCurrentAttack].mpCollider,
															mtxDamage,NULL,false,false,NULL,false);
	}*/



	cMatrixf mtxCollider = cMath::MatrixMul(pCamera->GetViewMatrix(), mtxDamage);

	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetMatrix(eMatrix_ModelView, mtxCollider);

	cVector3f vSize = mvAttacks[mlCurrentAttack].mvDamageSize;

	if (bCollide)
		mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(vSize * 0.5f, vSize * -0.5f,
			cColor(0, 1, 0, 1));
	else
		mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(vSize * 0.5f, vSize * -0.5f,
			cColor(1, 0, 1, 1));
}

//-----------------------------------------------------------------------

bool cHudModel_WeaponRanged::IsAttacking()
{
	if (mlAttackState > 1) return true;

	return false;
}

//-----------------------------------------------------------------------

void cHudModel_WeaponRanged::ResetExtraData()
{
	mlAttackState = 0;
	mfTime = 0;

	mlCurrentAttack = 0;

	mbButtonDown = false;
	mbAttacked = false;

	m_mtxPrevPose = cMatrixf::Identity;
	m_mtxNextPose = cMatrixf::Identity;

	mfMoveSpeed = 1.0f;
}

//-----------------------------------------------------------------------

void cHudModel_WeaponRanged::Attack()
{
	mpInit->mbWeaponAttacking = true;
	//Log("----------------- BEGIN ATTACK WITH WEAPON ------------ \n");

	////////////////////////////////
	//Set up
	float fDamageRange = mvAttacks[mlCurrentAttack].mfDamageRange;

	float fMaxImpulse = mvAttacks[mlCurrentAttack].mfMaxImpulse;
	float fMinImpulse = mvAttacks[mlCurrentAttack].mfMinImpulse;

	float fMaxMass = mvAttacks[mlCurrentAttack].mfMaxMass;
	float fMinMass = mvAttacks[mlCurrentAttack].mfMinMass;


	cCamera3D* pCamera = mpInit->mpPlayer->GetCamera();
	cVector3f vCenter = pCamera->GetPosition() + pCamera->GetForward() * fDamageRange;

	cBoundingVolume tempBV = mvAttacks[mlCurrentAttack].mBV;
	tempBV.SetPosition(vCenter);

	cVector3f vSpinMul = cVector3f(0, 1.0f, 0.0f);
	vSpinMul = pCamera->GetRight() * vSpinMul.x +
		pCamera->GetUp() * vSpinMul.y +
		pCamera->GetForward() * vSpinMul.z;

	cMatrixf mtxDamage = cMath::MatrixRotate(
		cVector3f(pCamera->GetPitch(), pCamera->GetYaw(), pCamera->GetRoll()),
		eEulerRotationOrder_XYZ);
	mtxDamage.SetTranslation(vCenter);

	cCollideData collideData;
	collideData.SetMaxSize(1);

	bool bHit = false;

	cWorld3D* pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	iPhysicsWorld* pPhysicsWorld = pWorld->GetPhysicsWorld();

	tVector3fList lstPostions;

	////////////////////////////////
	//Iterate Enemies
	tGameEnemyIterator enemyIt = mpInit->mpMapHandler->GetGameEnemyIterator();
	while (enemyIt.HasNext())
	{
		iGameEnemy* pEnemy = enemyIt.Next();
		iPhysicsBody* pBody = pEnemy->GetMover()->GetCharBody()->GetBody();
		float fMass = pBody->GetMass();

		if (pEnemy->GetMover()->GetCharBody()->IsActive() == false) continue;

		if (cMath::CheckCollisionBV(tempBV, *pBody->GetBV()))
		{
			/*if(pPhysicsWorld->CheckShapeCollision(pBody->GetShape(),pBody->GetLocalMatrix(),
				mvAttacks[mlCurrentAttack].mpCollider,
				mtxDamage,collideData,1)==false)
			{
				continue;
			}*/
			if (pEnemy->GetMeshEntity()->CheckColliderShapeCollision(pPhysicsWorld,
				mvAttacks[mlCurrentAttack].mpCollider,
				mtxDamage, &lstPostions, NULL) == false)
			{
				continue;
			}

			//Calculate force
			float fForceSize = 0;
			if (fMass > fMaxMass) fForceSize = 0;
			else if (fMass <= fMinMass) fForceSize = fMaxImpulse;
			else {
				float fT = (fMass - fMinMass) / (fMaxMass - fMinMass);
				fForceSize = fMinImpulse * fT + fMaxImpulse * (1 - fT);
			}

			cVector3f vForceDir = pCamera->GetForward();
			vForceDir.Normalise();

			//Add force to bodies
			for (int i = 0; i < pEnemy->GetBodyNum(); ++i)
			{
				iPhysicsBody* pBody = pEnemy->GetBody(i);

				pBody->AddImpulse(vForceDir * fForceSize * 0.5f);

				cVector3f vTorque = vSpinMul * fMass * fForceSize * 0.5f;
				pBody->AddTorque(vTorque);
			}

			//Calculate damage
			float fDamage = cMath::RandRectf(mvAttacks[mlCurrentAttack].mfMinDamage,
				mvAttacks[mlCurrentAttack].mfMaxDamage);

			pEnemy->Damage(fDamage, mvAttacks[mlCurrentAttack].mlAttackStrength);

			//Get closest position
			float fClosestDist = 9999.0f;
			cVector3f vClosestPostion = vCenter;
			for (tVector3fListIt it = lstPostions.begin(); it != lstPostions.end(); ++it)
			{
				cVector3f& vPos = *it;

				float fDist = cMath::Vector3DistSqr(pCamera->GetPosition(), vPos);
				if (fDist < fClosestDist)
				{
					fClosestDist = fDist;
					vClosestPostion = vPos;
				}
			}

			//Particle system
			if (pEnemy->GetHitPS() != "")
			{
				pWorld->CreateParticleSystem("Hit", pEnemy->GetHitPS(), 1,
					cMath::MatrixTranslate(vClosestPostion));
			}

			lstPostions.clear();

			bHit = true;
		}
	}

	std::set<iPhysicsBody*> m_setHitBodies;

	////////////////////////////////
	//Iterate bodies
	float fClosestHitDist = 9999.0f;
	cVector3f vClosestHitPos;
	iPhysicsMaterial* pClosestHitMat = NULL;
	cPhysicsBodyIterator it = pPhysicsWorld->GetBodyIterator();
	while (it.HasNext())
	{
		iPhysicsBody* pBody = it.Next();

		float fMass = pBody->GetMass();

		if (pBody->IsActive() == false) continue;
		if (pBody->GetCollide() == false) continue;
		if (pBody->IsCharacter()) continue;


		if (cMath::CheckCollisionBV(tempBV, *pBody->GetBV()))
		{
			if (pPhysicsWorld->CheckShapeCollision(pBody->GetShape(), pBody->GetLocalMatrix(),
				mvAttacks[mlCurrentAttack].mpCollider,
				mtxDamage, collideData, 1) == false)
			{
				continue;
			}

			cVector3f vHitPos = collideData.mvContactPoints[0].mvPoint;

			//Check if collision is in line of sight
			{
				mRayCallback.Reset();
				cVector3f vRayStart = pCamera->GetPosition();
				cVector3f vRayEnd = vHitPos;

				pPhysicsWorld->CastRay(&mRayCallback, vRayStart, vRayEnd, true, true, true, false);

				if (mRayCallback.mpClosestBody &&
					mRayCallback.mpClosestBody != pBody)
				{
					continue;
				}
			}

			m_setHitBodies.insert(pBody);

			//Deal damage and force
			HitBody(pBody);

			//Check if this is the closest hit body
			float fDist = cMath::Vector3DistSqr(vHitPos, pCamera->GetPosition());
			if (fDist < fClosestHitDist)
			{
				fClosestHitDist = fDist;
				vClosestHitPos = collideData.mvContactPoints[0].mvPoint;

				pClosestHitMat = pBody->GetMaterial();
			}

			bHit = true;
		}
	}

	////////////////////////////////////////////
	//Check with ray and see a closer material can be found.
	{
		float fAttackRange = mvAttacks[mlCurrentAttack].mfAttackRange;

		mRayCallback.Reset();
		cVector3f vRayStart = pCamera->GetPosition();
		cVector3f vRayEnd = pCamera->GetPosition() + pCamera->GetForward() * fAttackRange;

		pPhysicsWorld->CastRay(&mRayCallback, vRayStart, vRayEnd, true, true, true, false);

		if (mRayCallback.mpClosestBody)
		{
			//Use ray cast to check hit as well
			//Check first if body has not allready been hit.
			if (m_setHitBodies.find(mRayCallback.mpClosestBody) == m_setHitBodies.end())
			{
				HitBody(mRayCallback.mpClosestBody);
			}

			float fDist = cMath::Vector3DistSqr(mRayCallback.mvPosition, pCamera->GetPosition());
			if (fDist < fClosestHitDist)
			{
				fClosestHitDist = fDist;
				vClosestHitPos = mRayCallback.mvPosition;
				pClosestHitMat = mRayCallback.mpClosestBody->GetMaterial();
			}
		}
	}

	////////////////////////////////////////////
	//Check the closest material and play sounds and effects depending on it.
	if (pClosestHitMat)
	{
		bHit = true;

		cMatrixf mtxPosition = cMath::MatrixTranslate(vClosestHitPos);

		cSurfaceData* pData = pClosestHitMat->GetSurfaceData();
		cSurfaceImpactData* pImpact = pData->GetHitDataFromSpeed(mvAttacks[mlCurrentAttack].mfAttackSpeed);
		if (pImpact)
		{
			cSoundEntity* pSound = pWorld->CreateSoundEntity("Hit", pImpact->GetSoundName(), true);
			if (pSound) pSound->SetPosition(vClosestHitPos);

			if (mvAttacks[mlCurrentAttack].mlHitPSPrio <= pImpact->GetPSPrio())
			{
				if (pImpact->GetPSName() != "")
					pWorld->CreateParticleSystem("Hit", pImpact->GetPSName(), 1, mtxPosition);
			}
			else
			{
				if (mvAttacks[mlCurrentAttack].msHitPS != "")
					pWorld->CreateParticleSystem("Hit", mvAttacks[mlCurrentAttack].msHitPS, 1, mtxPosition);
			}
		}
	}
	//Log("----------------- END ATTACK WITH WEAPON ------------ \n");

	/////////////////////////
	//Play hit sound
	if (bHit)
	{
		PlaySound(mvAttacks[mlCurrentAttack].msHitSound);

		if (mpInit->mbHasHaptics)
		{
			if (mpHHitForce->IsActive())	mpHHitForce->SetActive(false);

			mpHHitForce->SetActive(true);
			mpHHitForce->SetTimeControl(false, 0.3f, 0.2f, 0, 0.1f);
		}
	}

	mpInit->mbWeaponAttacking = false;
}

//-----------------------------------------------------------------------

void cHudModel_WeaponRanged::HitBody(iPhysicsBody* apBody)
{
	iGameEntity* pEntity = (iGameEntity*)apBody->GetUserData();

	if (pEntity && pEntity->GetType() == eGameEntityType_Enemy) return;

	cCamera3D* pCamera = mpInit->mpPlayer->GetCamera();

	cVector3f vSpinMul = mvAttacks[mlCurrentAttack].mvSpinMul;
	vSpinMul = pCamera->GetRight() * vSpinMul.x +
		pCamera->GetUp() * vSpinMul.y +
		pCamera->GetForward() * vSpinMul.z;

	float fMass = apBody->GetMass();

	float fMaxImpulse = mvAttacks[mlCurrentAttack].mfMaxImpulse;
	float fMinImpulse = mvAttacks[mlCurrentAttack].mfMinImpulse;

	float fMaxMass = mvAttacks[mlCurrentAttack].mfMaxMass;
	float fMinMass = mvAttacks[mlCurrentAttack].mfMinMass;

	//Calculate force
	float fForceSize = 0;
	if (fMass > fMaxMass) fForceSize = 0;
	else if (fMass <= fMinMass) fForceSize = fMaxImpulse;
	else {
		float fT = (fMass - fMinMass) / (fMaxMass - fMinMass);
		fForceSize = fMinImpulse * fT + fMaxImpulse * (1 - fT);
	}

	//Calculate damage
	float fDamage = cMath::RandRectf(mvAttacks[mlCurrentAttack].mfMinDamage,
		mvAttacks[mlCurrentAttack].mfMaxDamage);

	cVector3f vForceDir = pCamera->GetForward();

	if (fMass > 0 && fForceSize > 0)
	{
		vForceDir.Normalise();

		//pBody->AddForce(vForceDir * fForceSize);
		apBody->AddImpulse(vForceDir * fForceSize);

		cVector3f vTorque = vSpinMul * fMass * fForceSize;

		//vTorque = cMath::MatrixMul(pBody->GetInertiaMatrix(),vTorque);

		apBody->AddTorque(vTorque);
	}

	if (pEntity)
	{
		pEntity->SetLastImpulse(vForceDir * fForceSize);
		pEntity->Damage(fDamage, mvAttacks[mlCurrentAttack].mlAttackStrength);
	}
}

//-----------------------------------------------------------------------
