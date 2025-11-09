#include <stdlib.h>
#include "Grouplist.h"
#include "GameSys.h"

int Group_Add(GroupMeta* group, void* element, InstanceTag type) {
	if (group->valueType != type) return 0;

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

	return 1;
}

/// <summary>
/// void*로 저장되어 있는 객체를 역참조해 내부 groupProperty를 반환함
/// </summary>
static GroupProperty* get_innerProp(void* element, InstanceTag type) {
	switch (type) {
	case Card_class:
		return &((Card*)element)->groupProp;
	}
}

void Group_Exclude(GroupProperty* element) {
	GroupMeta* group = element->affiliated;

	if (group->count <= 1) { //리스트에 한개 밖에 없는 경우
		group->front = NULL;
		group->rear = NULL;
	}
	else if (element->previous == NULL) { //index 0, 맨 앞을 고른 경우
		group->front = element->next;
		get_innerProp(element->next, group->valueType)->previous = NULL;
	}
	else if (element->next == NULL) { //마지막을 고른 경우
		group->rear = element->previous;
		get_innerProp(element->next, group->valueType)->next = NULL;
	}
	else { //앞 뒤에 원소가 존재하는 경우
		get_innerProp(element->previous, group->valueType)->next = element->next;
		get_innerProp(element->next, group->valueType)->previous = element->previous;
	}

	//모든 연결 해제
	element->previous = NULL;
	element->next = NULL;
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

	Group_Exclude(group, get_innerProp(ptr, group->valueType));

	return ptr;
}

void Group_FreeAll(GroupMeta* group) {
	if (group->count <= 0) {
		free(group);
		return;
	}

	void* next;
	void* temp;
	temp = group->front;
	while (temp != NULL) {
		next = get_innerProp(temp, group->valueType)->next;

		switch (group->valueType) {
		default:
			free(temp);
		}

		temp = next;
	}
	free(group);
}