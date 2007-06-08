// ai.h: interface for the GC_PlayerAI class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "Level.h" // FIXME!
#include "Player.h"

class GC_Object;
class GC_RigidBodyStatic;
struct VehicleState;

//----------------------------------------------------------

struct AIITEMINFO
{
	GC_Object *object;
	AIPRIORITY priority;
};

struct AIWEAPSETTINGS
{
	BOOL  bNeedOutstrip;       // FALSE, ���� ���������� ������ (gauss, ...)
	float fMaxAttackAngle;     // ������������ ���������� ����
	float fProjectileSpeed;    // �������� �������
	float fAttackRadius_min;   // ����������� ������ �����
	float fAttackRadius_max;   // ������������ ������ �����
	float fAttackRadius_crit;  // ����������� ������ �����, ����� ����� �������
	float fDistanceMultipler;  // ��������� ���������� ����
};

//----------------------------------------------------------

class CAttackList
{
	// ������������� ���� (�����, ������...)
	struct tagAttackNode
	{
		tagAttackNode      *_nextNode;
		tagAttackNode      *_prevNode;
		GC_RigidBodyStatic *_target;
	} *_firstTarget, *_lastTarget;

	static MemoryManager<tagAttackNode> s_anAllocator;

protected:
	tagAttackNode* FindObject(GC_RigidBodyStatic *object);
	void RemoveFromList(tagAttackNode *pNode);  // ������� ���� �� ������

public:
	CAttackList();
	CAttackList(CAttackList &al);
	virtual ~CAttackList();

	GC_RigidBodyStatic* Pop(BOOL bRemoveFromList = TRUE);  // ������� ���� �� ������ ������
	void PushToBegin(GC_RigidBodyStatic *target);          // ��������� ���� � ������ ������
	void PushToEnd  (GC_RigidBodyStatic *target);          // ��������� ���� � �����  ������

	void Clean();                                   // ���������� ��� ������ �������
	void ClearList() { while (!IsEmpty()) Pop(); }  // �������� ������

	inline BOOL IsEmpty() {return (NULL == _firstTarget);}

public:
	CAttackList& operator= (CAttackList &al);
};


template<class T> class JobManager;

class GC_PlayerAI : public GC_Player
{
	DECLARE_SELF_REGISTRATION(GC_PlayerAI);

	static JobManager<GC_PlayerAI> _jobManager;

	/*
     ���� ������� �� ������ ����� � ������ �����, ������� ����� ���� ������.

	 ���� ������ ���� � ���������� ���� � ������.
	 � �������� �������� ����� ���� ��������� �������������.
	 ���� ���� ������, ���� ����� �� �����.

     � ������ ����� �� �������, ������� ���������� ���������� ��� ����,
	 ����� ������� � ����� ����������.
	----------------------------------------------------------*/

	// ���� ����
	struct PathNode
	{
		vec2d coord;
	};


	//
	// ������� ����
	//

	std::list<PathNode> _path;  // ������ �����
	CAttackList  _AttackList;   // ������ �����



	//-------------------------------------------------------------------------
	// Desc: ������ ���� �� �������� �����, ��������� ��������� ����
	//  dst_x, dst_y - ���������� ����� ����������
	//  max_depth    - ������������ ������� ������ � �������
	//  bTest        - ���� true, �� ����������� ������ ��������� ����
	// Return: ��������� ���� ��� -1 ���� ���� �� ������
	//-------------------------------------------------------------------------
	float CreatePath(float dst_x, float dst_y, float max_depth, bool bTest);


	//-------------------------------------------------------------------------
	// Name: CreatePath()
	// Desc: ������� ������� ���� � ������ �����.
	//-------------------------------------------------------------------------
	void ClearPath();


	//-------------------------------------------------------------------------
	// Desc: ��������� � ������� ���� �������������� ���� ��� ��������� �����
	//       ������� ����������.
	//-------------------------------------------------------------------------
	void SmoothPath();


	//-------------------------------------------------------------------------
	// Desc: �������� ������������ ������ ����.
	//-------------------------------------------------------------------------
	bool CheckCell(const FieldCell &cell);

	struct TargetDesc
	{
		GC_Vehicle *target;
        bool bIsVisible;
	};


	// ��������� ��
	enum aiState_l2
	{
		L2_PATH_SELECT,   // ��������� ��������� �� ������
		L2_PICKUP,        // ���� �� ���������
		L2_ATTACK,        // ���������� �, ���� ��������, ������� ����
	} _aiState_l2;

	// ��������� �������� ������
	enum aiState_l1
	{
		L1_NONE,           // ��� ���� �� �����
		L1_PATH_END,       // ��������� ����� ����
		L1_STICK,          // ��������
	} _aiState_l1;

protected:
	SafePtr<GC_PickUp>          _pickupCurrent;
	SafePtr<GC_RigidBodyStatic> _target;  // ������� ����
	AIWEAPSETTINGS _weapSettings; // ��������� ������

	bool IsTargetVisible(GC_RigidBodyStatic *target, GC_RigidBodyStatic** ppObstacle = NULL);
	void LockTarget(GC_RigidBodyStatic *target);
	void FreeTarget();
	AIPRIORITY GetTargetRate(GC_Vehicle *target);

	bool FindTarget(/*out*/ AIITEMINFO &info);   // return true ���� ���� �������
	bool FindItem(/*out*/ AIITEMINFO &info);     // return true ���� ���-�� �������

	// �������� ������� ��� ��������� �������� ��������
	float _desired_offset;
	float _current_offset;

	// ������� ������
	ObjectType _otFavoriteWeapon;

	// ��������
	int _accuracy;

protected:
	void RotateTo(VehicleState *pState, const vec2d &x, bool bForv, bool bBack);
	void TowerTo (VehicleState *pState, const vec2d &x, bool bFire);

	// ��������� ���������� ������ ���� ��� �������� �� ����������
	// target - ����
	// Vp      - �������� �������
	void CalcOutstrip(GC_Vehicle *target, float Vp, vec2d &fake);

	void ProcessAction();

	void SetL1(GC_PlayerAI::aiState_l1 new_state); // ����������� ��������� l1
	void SetL2(GC_PlayerAI::aiState_l2 new_state); // ����������� ��������� l2

	void SelectState();
	void DoState(VehicleState *pVehState);

	virtual void Serialize(SaveFile &f);

public:
	GC_PlayerAI();
	GC_PlayerAI(FromFile);
	virtual ~GC_PlayerAI();

	virtual void OnRespawn();
	virtual void OnDie();

protected:
	virtual void GetControl(VehicleState *pState, float dt);

	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
