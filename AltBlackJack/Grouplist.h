#pragma once
typedef enum {
	Card_class,
	SpriteObj_class
} InstanceTag;

//객체
typedef struct GroupProperty {
	struct GroupListMeta* affiliated; //이 원소가 소속된 그룹리스트

	//GroupProperty를 내부변수로 가진 객체를 직접 포인터로 저장함
	void* next;
	void* previous;
} GroupProperty;

//객체
typedef struct GroupListMeta{
	InstanceTag valueType;

	//GroupProperty를 내부변수로 가진 객체를 직접 포인터로 저장함
	void* front;
	void* rear;
	int count;
} GroupMeta;

void GroupProperty_Initialize(GroupProperty* prop);

GroupMeta* Group_Create(InstanceTag valueType);

/// <summary>
/// 반드시, grouplist.h에서 지원하는 구조체만 element로 넣어야 함
/// </summary>
/// <returns>리스트가 담는 자료형과 일치하지 않으면 0을 반환</returns>
int Group_Add(GroupMeta* group, void* element, InstanceTag type);

/// <summary>
/// 객체를 group에서의 연결를 끊고 제외함
/// 주의 :: GroupProperty는 객체가 아닙니다 - &GroupProp으로 주소값을 반드시 넣으시오
/// </summary>
void Group_Exclude(GroupProperty* property);

/// <summary>
/// 그룹의 index번째 객체를 반환함
/// </summary>
void* Group_ExcludeByIndex(GroupMeta* group, int index);

/// <summary>
/// 그룹 원소들을 모두 제거, 메모리 해제함
/// </summary>
void Group_FreeAll(GroupMeta* group);