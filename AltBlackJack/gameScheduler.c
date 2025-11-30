#pragma once
#include "Grouplist.h"
#include "gameScheduler.h"
#include <stdlib.h>

GroupMeta* IntAnim_Stream;
GroupMeta* SpriteAnim_Stream;

/*
static char instanceCut_Queue[ANIM_INSTANCECUT_QUEUE_LENGTH][ANIM_INSTANCECUT_TAG_LENGTH];
static int instanceCut_Queue_count = 0;
*/

void Init_AnimStream() {
	IntAnim_Stream = Group_Create(IntAnim_class);
	SpriteAnim_Stream = Group_Create(SpriteAnim_class);
}
void Free_AnimStream() {
	Group_FreeAll(IntAnim_Stream);
	Group_FreeAll(SpriteAnim_Stream);
}

static float cal_AnimProgress(float elapsed, float duration, AnimType type) {
	float t = elapsed / duration;

	switch (type) {
	case Anim_EASE_IN:
		return t * t;               // 느리게 시작
	case Anim_EASE_OUT:
		return 1 - (1 - t) * (1 - t); // 느리게 끝
	case Anim_EASE_LINEAR:
	default:
		return t;                   // 선형
	}
}

IntAnim* IntAnim_Create(int* _tgtVar, float _duration, AnimType style, int startValue, int endValue, void(*onEnd), char* cutTag) {
	IntAnim* anim;
	anim = malloc(sizeof(IntAnim));
	if (anim == NULL) return NULL;//printf("failed card instantiate");

	anim->tgtVar = _tgtVar;
	anim->elapsed = 0;
	anim->duration = _duration;
	anim->animType = style;
	anim->start_value = startValue;
	anim->end_value = endValue;
	anim->onEndAnim = onEnd;
	strcpy_s(anim->instanceCut_Tag, ANIM_INSTANCECUT_TAG_LENGTH, cutTag);

	GroupProperty_Initialize(&anim->groupProp);

	return anim;
}

void IntAnim_Update(IntAnim* anim, double deltatime) {
	anim->elapsed += deltatime;
	if (anim->elapsed > anim->duration) anim->elapsed = anim->duration;

	anim->calVar = anim->start_value + (anim->end_value - anim->start_value) *
		cal_AnimProgress(anim->elapsed, anim->duration, anim->animType);

	*(anim->tgtVar) = (int)anim->calVar;

	if (anim->elapsed >= anim->duration) { //onEnd
		Group_Exclude(&anim->groupProp);
		if (anim->onEndAnim != NULL) anim->onEndAnim();
		free(anim);
	}
}

SpriteAnim* SpriteAnim_Create(int* spriteIndex, int animFrame[], int animlength, void (*onEnd), char* cutTag, short loop) {
	SpriteAnim* anim;
	anim = malloc(sizeof(SpriteAnim));
	if (anim == NULL) return NULL;//printf("failed card instantiate");

	anim->tgtVar = spriteIndex;
	anim->cur_frame = 0;
	anim->anim_code = malloc(sizeof(int) * animlength);
	anim->loop = loop;
	if (anim->anim_code == NULL) return NULL; //어휴 에러검사 귀찮아 ㅅㅂ
	
	for (int i = 0; i < animlength; i++) {
		anim->anim_code[i] = animFrame[i];
	}
	anim->anim_length = animlength;
	anim->onEndAnim = onEnd;
	strcpy_s(anim->instanceCut_Tag, ANIM_INSTANCECUT_TAG_LENGTH, cutTag);

	GroupProperty_Initialize(&anim->groupProp);

	return anim;
}

void SpriteAnim_Update(SpriteAnim* anim) {
	if (anim->cur_frame >= anim->anim_length) {
		*(anim->tgtVar) = anim->anim_code[anim->anim_length - 1];

		if (anim->loop != 0) {
			anim->cur_frame = 0;
		}
		else {
			Group_Exclude(&anim->groupProp); //! 미친 콜백하기 전에 제외 꼭 해라 좆된다 -> 콜백중에 재차 자신을 instancecut하는 구문이 존재하여 한무재귀함수 버그가 터진사례가 있었음
			if (anim->onEndAnim != NULL) anim->onEndAnim();

			free(anim->anim_code);
			free(anim);
			return;
		}
	}

	*(anim->tgtVar) = anim->anim_code[anim->cur_frame];
	anim->cur_frame++;
}

int Anim_InstanceCut(char* tag) {
	int flag = 0;
	void* loop;
	void* next_temp;
	//태그 맞는거 찾아서 업데이트 하니까, 메모리 해제되버려서 다음 원소 못 찾아서 미리 저장해야 함
	loop = IntAnim_Stream->front;

	while (loop != NULL) {
		next_temp = ((IntAnim*)loop)->groupProp.next;

		if (strcmp(((IntAnim*)loop)->instanceCut_Tag, tag) == 0) {
			((IntAnim*)loop)->elapsed = ((IntAnim*)loop)->duration;
			IntAnim_Update((IntAnim*)loop, 1.0);
			flag = 1;
		}

		loop = next_temp;
	}

	loop = SpriteAnim_Stream->front;

	while (loop != NULL) {
		next_temp = ((SpriteAnim*)loop)->groupProp.next;

		if (strcmp(((SpriteAnim*)loop)->instanceCut_Tag, tag) == 0) {
			((SpriteAnim*)loop)->cur_frame = ((SpriteAnim*)loop)->anim_length;
			((SpriteAnim*)loop)->loop = 0;
			SpriteAnim_Update((SpriteAnim*)loop);
			flag = 1;
		}

		loop = next_temp;
	}

	return flag;
}