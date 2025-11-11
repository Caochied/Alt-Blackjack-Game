#pragma once
#include "Grouplist.h"
#include <windows.h>
#define RENDERING_LAYERSIZE 3

typedef enum {
	Spade = 1,
	Heart = 2,
	Diamond = 3,
	Club = 4
} Suit;

//객체
typedef struct Card {
	Suit suit;
	int value;

	struct Card* stacked_card; //이 카드 위에 있는 카드

	GroupProperty groupProp;
} Card;

// 비트맵 및 스프라이트 정보를 관리
// 전역 변수에서 고정적으로 관리되는 정보이므로 객체가 아님
typedef struct {
	HBITMAP Hbitmap; //객체임 -> DeleteObject(Hbitmap); 작성해야함
	BITMAP bmp; // 초기화 방법 -> GetObject(Hbitmap, sizeof(BITMAP), &bmp);

	int spriteCount; //이 이미지에 담긴 스프라이트 갯수

	//스프라이트 1개당 이미지 크기
	int width;
	int height;
} bitResource;

//객체
typedef struct SpriteObject {
	bitResource* resource;
	int sprite_index; //사용할 스프라이트 번호
	int x; int y; //화면 상 위치, 자식 객체인 경우, 부모 객체의 위치에서 상대적인 좌표로 계산함

	struct SpriteObject* children; //이 스프라이트에 종속된 추가적인 스프라이트

	GroupProperty groupProp;
} SpriteObj;

/// <summary>
/// 카드가 뽑혔을 때, 스프라이트와 카드 객체를 실체로서 다루기 위한 오브젝트
/// 일단은 객체가 아니라 구조체로 관리해봄.
/// </summary>
typedef struct CardGameObj {
	Card* card;

	GroupMeta* RenderLayer; //렌더링 시, 어떤 레이어에서 출력해야 하는지 저장

	//카드 더미의 가장 윗 카드를 표현하기 위한 오브젝트
	SpriteObj* bg;
	SpriteObj* num;

	//가장 밑에 깔린 카드를 표현하기 위한 오브젝트
	SpriteObj* rootCard_bg;
	SpriteObj* rootCard_num;
	SpriteObj* rootCard_suit;

	short Initialized; //스프라이트들 초기화 여부 반환
} CardGameObj;


GroupMeta* RenderList[RENDERING_LAYERSIZE];

HDC hdc;
HDC renderDC; //렌더링을 위한 버퍼
HBITMAP renderBmp;
HDC bmpDC; //비트맵 출력을 위한 임시 캔버스
static HBITMAP old_bmp;
static HBITMAP old_render;

HBRUSH GameBG;
bitResource Star_crumb;
bitResource Card_BG;
bitResource Card_Num[2]; //0 어두운 것, 1 밝은 것

//스프라이트 확대 수준
static int Scale = 4;
static int offsetX = 8, offsetY = 32;

/// <summary>
/// 스프라이트 리소스와 HDC, 렌더링 LIST 같은 전역 변수를 초기화 합니다
/// </summary>
void Load_bitResource();
void Unload_bitResource();

Card* Card_Instantiate(Suit suit, int value);

/// <summary>
/// 이 카드더미의 가장 위에 놓인 카드를 반환합니다
/// </summary>
Card* Card_getUpper(Card* card);

/// <summary>
/// stackingCard부터 위에 있는 카드 더미들 TgtPile 가장 위에 올립니다
/// </summary>
/// <returns>실패 시 0을 반환</returns>
int Card_StackUp(Card* TgtPile, Card* stackingCard);

/// <summary>
/// 카드 더미 전체를 메모리 해제 합니다
/// </summary>
void Card_Free(Card* card);

SpriteObj* SpriteObj_Instantiate(const bitResource* resource, int index, int x, int y);

/// <summary>
/// 스프라이트 오브젝트와 그것의 자녀 객체까지 정리함.
/// </summary>
void SpriteObj_Free(SpriteObj* sprite);

/// <summary>
/// 스프라이트와 자녀 객체의 정보까지 한번에 출력함
/// </summary>
void SpriteObj_Print(SpriteObj* sprite);

/// <summary>
/// 스프라이트를 할당하고, 초기화 함
/// </summary>
void CardGameObj_Initialize(CardGameObj* gameobj, int renderLayer, int x, int y);

/// <summary>
/// 현재 카드 상태에 따라 스프라이트 상태를 변경합니다
/// </summary>
void CardGameObj_Update(CardGameObj* gameobj);

/// <summary>
/// 내부 스프라이트와 카드를 모두 메모리 해제 함
/// </summary>
void CardGameObj_Free(CardGameObj* gameobj);