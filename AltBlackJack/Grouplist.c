#include <stdlib.h>
#include "Grouplist.h"
#include "GameSys.h"
#include <stdio.h>

#pragma region Static Func

/// <summary>
/// void*로 저장되어 있는 객체를 역참조해 내부 groupProperty를 반환함
/// </summary>
static GroupProperty* get_innerProp(void* element, InstanceTag type) {
	switch (type) {
	case Card_class:
		return &((Card*)element)->groupProp;
	case SpriteObj_class:
		return &((SpriteObj*)element)->groupProp;
	case FontTextObj_class:
		return &((FontTextObj*)element)->groupProp;
	}

	return NULL;
}

#pragma endregion

void GroupProperty_Initialize(GroupProperty* prop) {
	prop->affiliated = NULL;
	prop->next = NULL;
	prop->previous = NULL;
}

GroupMeta* Group_Create(InstanceTag valueType)
{
	GroupMeta* meta;
	meta = malloc(sizeof(GroupMeta));
	if (meta == NULL) { printf("failed list meta instantiate"); }

	meta->valueType = valueType;
	meta->front = NULL;
	meta->rear = NULL;
	meta->count = 0;

	return meta;
}

int Group_Add(GroupMeta* group, void* element, InstanceTag type) {
	if (group->valueType != type) return 0; //자료형 불일치
	if (get_innerProp(element, type)->affiliated != NULL) return 0; //이미 어떤 그룹에 소속됨

	GroupProperty property = { group, NULL, NULL };

	if (group->front == NULL || group->rear == NULL) { //grouplist가 빈 경우
		group->front = group->rear = element;
	}
	else { //원소가 1개 이상 존재한 경우
		property.previous = group->rear;
		get_innerProp(group->rear, type)->next = element;
		group->rear = element;
	}
	group->count++;

	get_innerProp(group->rear, type)->affiliated = property.affiliated;
	get_innerProp(group->rear, type)->next= property.next;
	get_innerProp(group->rear, type)->previous = property.previous;

	return 1;
}

void Group_Exclude(GroupProperty* property) {
	GroupMeta* group = property->affiliated;

	if (property->affiliated == NULL) return; //소속 없음

	if (group->count <= 1) { //리스트에 한개 밖에 없는 경우
		group->front = NULL;
		group->rear = NULL;
	}
	else if (property->previous == NULL) { //index 0, 맨 앞을 고른 경우
		group->front = property->next;
		get_innerProp(property->next, group->valueType)->previous = NULL;
	}
	else if (property->next == NULL) { //마지막을 고른 경우
		group->rear = property->previous;
		get_innerProp(property->previous, group->valueType)->next = NULL;
	}
	else { //앞 뒤에 원소가 존재하는 경우
		get_innerProp(property->previous, group->valueType)->next = property->next;
		get_innerProp(property->next, group->valueType)->previous = property->previous;
	}

	//모든 연결 해제
	property->affiliated = NULL;
	property->previous = NULL;
	property->next = NULL;
	group->count--;
}

void* Group_ExcludeByIndex(GroupMeta* group, int index) {
	void* ptr;
	if (index == 0) {
		ptr = group->front;
	}
	else if (index == group->count - 1) ptr = group->rear;
	else {
		ptr = group->front;
		for (int i = 0; i < index; i++) ptr = get_innerProp(ptr, group->valueType)->next;
	}

	Group_Exclude(get_innerProp(ptr, group->valueType));

	return ptr;
}

void Group_ClearAll(GroupMeta* group) {
	void* next;
	void* temp;
	temp = group->front;
	while (temp != NULL) {
		next = get_innerProp(temp, group->valueType)->next;

		switch (group->valueType) {
		case Card_class:
			Card_Free((Card*)temp);
		case SpriteObj_class:
			SpriteObj_Free((SpriteObj*)temp);
		default:
			free(temp);
		}

		temp = next;
	}

	group->count = 0;
	group->front = NULL;
	group->rear = NULL;
	//담는 자료형 유형은 보존됨
}

void Group_FreeAll(GroupMeta* group) {
	Group_ClearAll(group);
	free(group);
}