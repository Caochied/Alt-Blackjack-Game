#include "GameSys.h"
#include "resource.h"
#include <stdlib.h>

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

void(*Key_Z)(int) = NULL; //(키 이벤트)
void(*Key_X)(int) = NULL; //(키 이벤트)
void(*Key_Horizontal)(int, int) = NULL; //(키 이벤트, 방향)
void(*Key_Vertical)(int, int) = NULL; //(키 이벤트, 방향)

//스프라이트 확대 수준
int Scale = 4;
int offsetX = 8, offsetY = 32;

void Load_bitResource()
{
	for (int i = 0; i < RENDERING_LAYERSIZE; i++) {
		RenderList[i] = Group_Create(SpriteObj_class);
	}

	hdc = GetWindowDC(GetForegroundWindow()); // Handle to Device Context
	renderDC = CreateCompatibleDC(hdc);
	renderBmp = CreateCompatibleBitmap(hdc, 960, 512);
	bmpDC = CreateCompatibleDC(hdc);

	GameBG = CreateSolidBrush(RGB(42, 23, 59));

	//테스트 리소스
	Star_crumb.Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP4));
	GetObject(Star_crumb.Hbitmap, sizeof(BITMAP), &Star_crumb.bmp);
	Star_crumb.spriteCount = 2; Star_crumb.width = 16; Star_crumb.height = 16;

	Card_Num[0].Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP2));
	GetObject(Card_Num[0].Hbitmap, sizeof(BITMAP), &Card_Num[0].bmp);
	Card_Num[0].spriteCount = 17; Card_Num[0].width = 5; Card_Num[0].height = 6;

	Card_Num[1].Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP3));
	GetObject(Card_Num[1].Hbitmap, sizeof(BITMAP), &Card_Num[1].bmp);
	Card_Num[1].spriteCount = 17; Card_Num[1].width = 5; Card_Num[1].height = 6;

	Card_BG.Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP1));
	GetObject(Card_BG.Hbitmap, sizeof(BITMAP), &Card_BG.bmp);
	Card_BG.spriteCount = 5; Card_BG.width = 36; Card_BG.height = 48;

	old_bmp = (HBITMAP)SelectObject(bmpDC, Card_BG.Hbitmap); //기존 변수 저장
	old_render = (HBITMAP)SelectObject(renderDC, renderBmp);
}

void Unload_bitResource()
{
	for (int i = 0; i < RENDERING_LAYERSIZE; i++) {
		Group_FreeAll(RenderList[i]);
	}

	SelectObject(bmpDC, old_bmp); //기존 변수 복구
	SelectObject(renderDC, old_render); //기존 변수 복구
	ReleaseDC(GetForegroundWindow(), hdc);
	DeleteObject(renderBmp);
	DeleteDC(renderDC);
	DeleteDC(bmpDC);

	DeleteObject(GameBG);
	DeleteObject(Star_crumb.Hbitmap);
	DeleteObject(Card_Num[0].Hbitmap);
	DeleteObject(Card_Num[1].Hbitmap);
	DeleteObject(Card_BG.Hbitmap);
}

void Input_KeyPress()
{
	static int keyflags = 0; //onKeyDown 와 onKeyUp 구분하기 위한 플래그
	// 최하위비트부터 순서대로
	// -1 : Left
	// -2 : Right
	// -3 : Up
	// -4 : Down
	// -5 : Z, Confirm
	// -6 : X, Cancel

	if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
		if (keyflags & 0x0001) {
			if (Key_Horizontal != NULL) Key_Horizontal(0, -1);
		}
		else { //이전에 눌린적이 없으면, onKeyDown 호출
			keyflags = keyflags | 0x0001;
			if (Key_Horizontal != NULL) Key_Horizontal(1, -1);
		}
	}
	else if (keyflags & 0x0001) { //이전에 눌린적이 있으면, onKeyUp 호출
		keyflags = keyflags & ~0x0001;
		if (Key_Horizontal != NULL) Key_Horizontal(-1, -1);
	}

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
		if (keyflags & 0x0002) {
			if (Key_Horizontal != NULL) Key_Horizontal(0, 1);
		}
		else {
			keyflags = keyflags | 0x0002;
			if (Key_Horizontal != NULL) Key_Horizontal(1, 1);
		}
	}
	else if (keyflags & 0x0002) {
		keyflags = keyflags & ~0x0002;
		if (Key_Horizontal != NULL) Key_Horizontal(-1, 1);
	}

	if (GetAsyncKeyState(VK_UP) & 0x8000) {
		if (keyflags & 0x0004) {
			if (Key_Vertical != NULL) Key_Vertical(0, 1);
		}
		else {
			keyflags = keyflags | 0x0004;
			if (Key_Vertical != NULL) Key_Vertical(1, 1);
		}
	}
	else if (keyflags & 0x0004) {
		keyflags = keyflags & ~0x0004;
		if (Key_Vertical != NULL) Key_Vertical(-1, 1);
	}

	if (GetAsyncKeyState(VK_DOWN) & 0x8000) {
		if (keyflags & 0x0008) {
			if (Key_Vertical != NULL) Key_Vertical(0, -1);
		}
		else {
			keyflags = keyflags | 0x0008;
			if (Key_Vertical != NULL) Key_Vertical(1, -1);
		}
	}
	else if (keyflags & 0x0008) {
		keyflags = keyflags & ~0x0008;
		if (Key_Vertical != NULL) Key_Vertical(-1, -1);
	}

	if (GetAsyncKeyState(0x5A) & 0x8000) { // ! Z key
		if (keyflags & 0x0010) {
			if (Key_Z != NULL) Key_Z(0);
		}
		else {
			keyflags = keyflags | 0x0010;
			if (Key_Z != NULL) Key_Z(1);
		}
	}
	else if (keyflags & 0x0010) {
		keyflags = keyflags & ~0x0010;
		if (Key_Z != NULL) Key_Z(-1);
	}

	if (GetAsyncKeyState(0x59) & 0x8000) { // ! X key
		if (keyflags & 0x0020) {
			if (Key_X != NULL) Key_X(0);
		}
		else {
			keyflags = keyflags | 0x0020;
			if (Key_X != NULL) Key_X(1);
		}
	}
	else if (keyflags & 0x0020) {
		keyflags = keyflags & ~0x0020;
		if (Key_X != NULL) Key_X(-1);
	}
}

Card* Card_Instantiate(Suit suit, int value) {
	Card* card;
	card = malloc(sizeof(Card));
	if (card == NULL) return NULL;//printf("failed card instantiate");

	card->suit = suit;
	card->value = value;
	card->stacked_card = NULL;

	GroupProperty_Initialize(&(card->groupProp));

	return card;
}

/// <summary>
/// 이 카드더미의 가장 위에 놓인 카드를 반환합니다
/// </summary>
Card* Card_getUpper(Card* card) {
	Card* c = card;
	while (c->stacked_card != NULL) c = c->stacked_card;
	return c;
}

/// <summary>
/// stackingCard부터 위에 있는 카드 더미들 TgtPile 가장 위에 올립니다
/// </summary>
/// <returns>실패 시 0을 반환</returns>
int Card_StackUp(Card* TgtPile, Card* stackingCard) {
	//여기에 게임 조건 : 무색카드 겹치기 금지 여부 검사 추가하시오
	Card* upperCard = Card_getUpper(TgtPile);

	if (upperCard->value <= stackingCard->value) return 0;

	upperCard->stacked_card = stackingCard;
	return 1;
}

void Card_Free(Card* card) {
	if (card->stacked_card != NULL) Card_Free(card->stacked_card);

	Group_Exclude(&card->groupProp);
	free(card);
}

SpriteObj* SpriteObj_Instantiate(const bitResource* resource, int index, int x, int y) {
	SpriteObj* spr;
	spr = malloc(sizeof(SpriteObj));
	if (spr == NULL) return NULL; //printf("failed spriteObj instantiate");

	spr->resource = resource;
	spr->sprite_index = index;
	spr->x = x;
	spr->y = y;

	spr->children = NULL;

	GroupProperty_Initialize(&(spr->groupProp));

	return spr;
}

void SpriteObj_Free(SpriteObj* sprite) {
	if (sprite->children != NULL) SpriteObj_Free(sprite->children);
	else {
		Group_Exclude(&sprite->groupProp);
		free(sprite);
	}
}

void SpriteObj_Print(SpriteObj* tempS) {
	static int preX = 0, preY = 0;
	// 재귀 함수 형태로 자식 객체도 이어서 프린트 할때,
	// 자식 객체의 좌표는 부모의 좌표에서 상대적으로 계산하므로, 부모의 절대 좌표를 기억하기 위해 사용함.

	int x, y; //Dest X,Y 계산용
	x = (tempS->x + preX);
	y = (tempS->y + preY);

	SelectObject(bmpDC, tempS->resource->Hbitmap);
	TransparentBlt(renderDC, x * Scale, y * Scale, tempS->resource->width * Scale, tempS->resource->height * Scale,
		bmpDC, tempS->resource->width * tempS->sprite_index, 0, tempS->resource->width, tempS->resource->height,
		RGB(255, 0, 255));

	if (tempS->children != NULL) {
		preX = x; preY = y;

		if (tempS->children->groupProp.affiliated != NULL) {
			Group_Exclude(&tempS->children->groupProp);
			//printf("\nSpriteObj의 자식 객체가 이중 렌더링 상태임");
		}

		SpriteObj_Print(tempS->children);
	}
	else {
		preX = 0; preY = 0;
	}
}
void CardGameObj_Initialize(CardGameObj* gameobj, int renderLayer, int x, int y)
{
	if (gameobj->Initialized == 1) return;

	gameobj->Initialized = 1;
	gameobj->RenderLayer = RenderList[renderLayer];

	gameobj->card = NULL;

	gameobj->bg = SpriteObj_Instantiate(&Card_BG, 0, x, y);
	gameobj->num = SpriteObj_Instantiate(&Card_Num[0], 0, 3, 3);
	gameobj->bg->children = gameobj->num;

	// ! 루트 카드는 가장 윗 카드에 종속된 스프라이트처럼 보이지만,
	// ! 가시성을 조절할 수 있어야 하므로, 직접 렌더링 그룹에서 관리된다
	gameobj->rootCard_bg = SpriteObj_Instantiate(&Card_BG, 4, x, y-9);
	gameobj->rootCard_num = SpriteObj_Instantiate(&Card_Num[0], 0, 3, 3);
	gameobj->rootCard_bg->children = gameobj->rootCard_num;

	gameobj->rootCard_suit = SpriteObj_Instantiate(&Card_Num[0], 13, 25, 0);
	gameobj->rootCard_num->children = gameobj->rootCard_suit;

	Group_Add(gameobj->RenderLayer, gameobj->bg, SpriteObj_class);
}
void CardGameObj_Update(CardGameObj* gameobj)
{
	if (gameobj->card == NULL) return;

	Card* upper = Card_getUpper(gameobj->card);
	if (upper != gameobj->card) { //스택이 된 카드들이라면, 가장 밑에 깔린 카드를 표현
		Group_Add(gameobj->RenderLayer, gameobj->rootCard_bg, SpriteObj_class);

		// 가장 윗 카드 위치를 따라감
		gameobj->rootCard_bg->x = gameobj->bg->x;
		gameobj->rootCard_bg->y = gameobj->bg->y - 9;
		gameobj->rootCard_num->resource = &Card_Num[0];

		gameobj->rootCard_suit->sprite_index = (int)gameobj->card->suit + 12;
		gameobj->rootCard_num->sprite_index = gameobj->card->value - 1;
	}
	else {
		Group_Exclude(&gameobj->rootCard_bg->groupProp);
	}

	switch (upper->suit) {
	case Spade:
		gameobj->bg->sprite_index = 0;
		gameobj->num->resource = &Card_Num[0];
		break;
	case Club:
		gameobj->bg->sprite_index = 1;
		gameobj->num->resource = &Card_Num[0];
		break;
	case Diamond:
		gameobj->bg->sprite_index = 2;
		gameobj->num->resource = &Card_Num[1];
		break;
	case Heart:
		gameobj->bg->sprite_index = 3;
		gameobj->num->resource = &Card_Num[1];
		break;
	}
	gameobj->num->sprite_index = upper->value - 1;
}

void CardGameObj_Free(CardGameObj* gameobj)
{
	gameobj->Initialized = 0;

	Card_Free(gameobj->card);
	gameobj->card = NULL;

	SpriteObj_Free(gameobj->bg);
	SpriteObj_Free(gameobj->rootCard_bg);
	// ! gameobj->num, gameobj->rootCard_num
	// ! 함수에 의해 자동으로 메모리 해제
}
