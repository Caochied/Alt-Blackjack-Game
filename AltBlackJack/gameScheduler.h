#pragma once
#include "Grouplist.h"
#include <string.h>

#define ANIM_INSTANCECUT_TAG_LENGTH 16
#define ANIM_INSTANCECUT_QUEUE_LENGTH 8 

typedef enum {
	Anim_EASE_LINEAR,
	Anim_EASE_IN,
	Anim_EASE_OUT
	// ! Ease in/out 수식은 생성형 AI의 도움을 받음
}AnimType;

// 객체임
// Tween 기능과 동일함
typedef struct IntAnimator{
	int* tgtVar; //실제 변화시킬 변수
	float calVar; //내부 연산용 변수

	float elapsed;
	float duration;
	AnimType animType;
	
	int start_value;
	int pre_value;
	int end_value;

	char instanceCut_Tag[ANIM_INSTANCECUT_TAG_LENGTH];
	void (*onEndAnim)();

	GroupProperty groupProp;
}IntAnim;

// 객체임
typedef struct SpriteAnimator {
	int* tgtVar;

	int cur_frame;
	short loop; //0이 아니면, Anim_InstanceCut으로 종료되기 전까지 반복합니다

	/// <summary>
	/// tgtVar를 24프레임 마다 변경함
	/// </summary>
	int* anim_code; // 값 복사시, 동적 메모리 할당 존재
	int anim_length;

	char instanceCut_Tag[ANIM_INSTANCECUT_TAG_LENGTH];
	void (*onEndAnim)();

	GroupProperty groupProp;
}SpriteAnim;

extern GroupMeta* IntAnim_Stream;
extern GroupMeta* SpriteAnim_Stream;

void Init_AnimStream();
void Free_AnimStream();

/// <summary>
/// Anim Update리스트에 한번 포함시키면 자동 실행됨. 종료시 알아서 메모리 해제됨
/// </summary>
IntAnim* IntAnim_Create(int* tgtVar, float duration, AnimType style, int startValue, int endValue, void (*onEnd), char* cutTag);

void IntAnim_Update(IntAnim* anim, double deltatime);

SpriteAnim* SpriteAnim_Create(int* spriteIndex, int animFrame[], int animlength, void (*onEnd), char* cutTag, short loop);

void SpriteAnim_Update(SpriteAnim* anim);

/// <summary>
/// 모든 애니메이션 종류의 스트림에서,
/// 해당 tag를 가진 애니메이터들을 즉시 종료합니다
/// 종료된 애니메이터가 존재하면 0이 아닌 값을 반환합니다
/// </summary>
int Anim_InstanceCut(char* tag);
// ! Anim 포인터를 변수로 저장하고 있으면, 자동으로 메모리 해제되는 특성으로 인해
// ! 메모리 오류가 발생하기 때문에, 간접적으로 제거하는 구문을 제공함
