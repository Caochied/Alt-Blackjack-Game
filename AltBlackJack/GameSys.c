#include "GameSys.h"
#include "gameScheduler.h"
#include "resource.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

GroupMeta* RenderList_Sprite[SPRITE_RENDERING_LAYERSIZE];
GroupMeta* RenderList_Text;

HDC hdc;
HDC renderDC; //렌더링을 위한 버퍼
HBITMAP renderBmp;

HDC bmpDC; //비트맵 출력을 위한 임시 캔버스

static HBITMAP old_bmp;
static HBITMAP old_render;

static int SpFont_Sizes[SP_FONT_COUNT]; //각 글자마다의 크기

HBRUSH BlankPlane;
static HBRUSH TestLine;
bitResource rSpFonts[2];
bitResource rStar_crumb;
bitResource rCard_BG;
bitResource rCard_Num[2]; //0 어두운 것, 1 밝은 것
bitResource rLisette;
bitResource rDali;
bitResource rCardPointer;
bitResource rTipBox;

void(*Key_Z)(int) = NULL;
void(*Key_X)(int) = NULL;
void(*Key_C)(int) = NULL;
void(*Key_Horizontal)(int, int) = NULL;
void(*Key_Vertical)(int, int) = NULL;

//스프라이트 확대 수준
int Scale = 4;
int offsetX = 8, offsetY = 32;
//렌더링 시점 위치, 렌더링 스케일 단위 기준
int CameraX = 0, CameraY = 0;

double deltaTime = 0; //실제 코드의 elasped clock
double frameTime = 0; double frameRate = (1.0 / 120.0) / CLOCKS_PER_SEC;
int animTime = 0; int animRate = 5; //스프라이트 교체에 대한 연산은 - 24프레임

GameState SceneState = Game_Idle;

int Player_HP;
int Discarded_Goal; //카드 제거 목표 갯수
short FleeUsed = 0;
GroupMeta* Deck;
GroupMeta* Discarded;
CardGameObj Hands[HANDS_MAX];
int Pointing_Hand; //방향키로 포인팅된 카드
int Picked_Pile = -1; //스택 기능을 사용할 때, 들어올려진 카드
static int Selected_Option; //선택지 지정 상황에서 사용되는 변수
static short visible_TipBox = 0;

void Game_MainLoop() {
	clock_t end_time;
	clock_t start_time;

	while (1) {
		start_time = clock();
		if (SceneState == Game_Quit) break;

		if (frameTime > frameRate) { // ! 프레임별 연산
			if (animTime >= animRate) { // ! 스프라이트 애니메이션 24fps
				SpriteAnim* loop;
				loop = SpriteAnim_Stream->front;

				while (loop != NULL) {
					SpriteAnim* next = (SpriteAnim*)loop->groupProp.next;
					SpriteAnim_Update(loop);
					loop = next;
				}
				animTime = 0;
			}
			else animTime += (int)(frameTime*animRate / frameRate); //통상적으론 1이지만, 프레임이 낮아져 60프레임이 되면, 2씩 오른다. 그외에 프레임이 낮을때 문제가 있겠지만 귀찮으니...

			IntAnim* loop;
			loop = IntAnim_Stream->front;

			while (loop != NULL) {
				IntAnim* next = (IntAnim*)loop->groupProp.next;
				IntAnim_Update(loop, frameTime);
				loop = next;
			}

			//그래픽 루프 전, 게임오브젝트 업데이트
			for (int i = 0; i < 4; i++) {
				CardGameObj_Update(&Hands[i]);
			}

			RECT rc = { 0, 0, 960, 512 };
			FillRect(renderDC, &rc, BlankPlane);

			RECT horizontal_line = { 0, 255, 960, 257};
			RECT vertical_line = { 479, 0, 481, 512 };
			FillRect(renderDC, &horizontal_line, TestLine);
			FillRect(renderDC, &vertical_line, TestLine);

			SpriteObj* listLoop;
			for (int i = 0; i < SPRITE_RENDERING_LAYERSIZE; i++) {
				listLoop = (SpriteObj*)RenderList_Sprite[i]->front;
				while (listLoop != NULL) {
					SpriteObj_Print(listLoop);
					listLoop = (SpriteObj*)listLoop->groupProp.next;
				}
			}

			SpTextObj* listLoop_t = (SpTextObj*)RenderList_Text->front;
			while (listLoop_t != NULL) {
				SpTextObj_Print(listLoop_t);
				listLoop_t = (SpTextObj*)listLoop_t->groupProp.next;
			}

			//버퍼에 담긴 결과 출력
			BitBlt(hdc, offsetX, offsetY, 960, 512, renderDC, 0, 0, SRCCOPY);

			// ! 키입력 받기
			Input_KeyPress();

			frameTime = 0; // ! 프레임 초기화
			//printf("\b\b\b\b\b\b\b\b\b\b\b\bfps : %.2lf", CLOCKS_PER_SEC / deltaTime);
		}

		end_time = clock();

		deltaTime = (double)(end_time - start_time) / CLOCKS_PER_SEC;
		frameTime += deltaTime;
	}
}

void Load_bitResource()
{
	for (int i = 0; i < SPRITE_RENDERING_LAYERSIZE; i++) {
		RenderList_Sprite[i] = Group_Create(SpriteObj_class);
	}
	RenderList_Text = Group_Create(SpTextObj_class);

	Init_AnimStream();

	hdc = GetWindowDC(GetForegroundWindow()); // Handle to Device Context
	renderDC = CreateCompatibleDC(hdc);
	renderBmp = CreateCompatibleBitmap(hdc, 960, 512);
	bmpDC = CreateCompatibleDC(hdc);

	/*
	* 폰트 설정 너무 어려워ㅜ
	// ! 리소스에서 폰트 로드
	HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_FONT1), RT_FONT);
	HGLOBAL hFontData = LoadResource(NULL, hRes);
	void* pFontData = LockResource(hFontData);
	DWORD fontSize = SizeofResource(NULL, hRes);

	// ! OS에 임시로 폰트 등록
	DWORD nFonts;
	renderFontRes = AddFontMemResourceEx(pFontData, fontSize, NULL, &nFonts);

	// ! Font 변수 생성
	LOGFONT lf = { 0 };
	lf.lfHeight = 18; //폰트 크기
	lstrcpy(lf.lfFaceName, TEXT("LanaPixel")); // 리소스 폰트 이름
	HFONT hFont = CreateFontIndirect(&lf);
	*/

	BlankPlane = CreateSolidBrush(RGB(GamePalette[0][0], GamePalette[0][1], GamePalette[0][2]));
	TestLine = CreateSolidBrush(RGB(GamePalette[1][0], GamePalette[1][1], GamePalette[1][2]));

	//테스트 리소스
	rStar_crumb.Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP4));
	GetObject(rStar_crumb.Hbitmap, sizeof(BITMAP), &rStar_crumb.bmp);
	rStar_crumb.spriteCount = 2; rStar_crumb.row = 1; rStar_crumb.width = 16; rStar_crumb.height = 16;

	rSpFonts[0].Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP7));
	GetObject(rSpFonts[0].Hbitmap, sizeof(BITMAP), &rSpFonts[0].bmp);
	rSpFonts[0].spriteCount = SP_FONT_COUNT; rSpFonts[0].row = 6; rSpFonts[0].width = 11; rSpFonts[0].height = 8;

	rSpFonts[1].Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP9));
	GetObject(rSpFonts[1].Hbitmap, sizeof(BITMAP), &rSpFonts[1].bmp);
	rSpFonts[1].spriteCount = SP_FONT_COUNT; rSpFonts[1].row = 6; rSpFonts[1].width = 11; rSpFonts[1].height = 8;
	
	//! 폰트마다 크기가 다르므로, 각 글자마다 크기지정
	for (int i = 0; i < SP_FONT_COUNT; i++) SpFont_Sizes[i] = 5;

	SpFont_Sizes[0] = 3;
	//! size = 3
	SpFont_Sizes[1] = SpFont_Sizes[73] = 2;
	//! size = 4
	SpFont_Sizes[7] = 
		SpFont_Sizes[8] = SpFont_Sizes[9] = 
		SpFont_Sizes[12] = SpFont_Sizes[42] =
		SpFont_Sizes[17] = SpFont_Sizes[59] =
		SpFont_Sizes[60] = SpFont_Sizes[61] = 
		SpFont_Sizes[70] = SpFont_Sizes[74] = 
		SpFont_Sizes[76] = SpFont_Sizes[84] = 3;
	//! size = 5
	SpFont_Sizes[13] = SpFont_Sizes[41] =
		SpFont_Sizes[82] = SpFont_Sizes[83] = 4;
	//! size = 7
	SpFont_Sizes[3] = SpFont_Sizes[5] =
		SpFont_Sizes[6] = SpFont_Sizes[10] =
		SpFont_Sizes[11] = SpFont_Sizes[45] =
		SpFont_Sizes[47] = SpFont_Sizes[49] =
		SpFont_Sizes[52] = SpFont_Sizes[54] =
		SpFont_Sizes[56] = SpFont_Sizes[57] = 
		SpFont_Sizes[62] = SpFont_Sizes[63] =
		SpFont_Sizes[87] = SpFont_Sizes[88] = 6;
	//! size = 9
	SpFont_Sizes[55] = SpFont_Sizes[77] = 8;
	//! size = 10
	SpFont_Sizes[32] = 9;

	rCard_Num[0].Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP2));
	GetObject(rCard_Num[0].Hbitmap, sizeof(BITMAP), &rCard_Num[0].bmp);
	rCard_Num[0].spriteCount = 17; rCard_Num[0].row = 1; rCard_Num[0].width = 5; rCard_Num[0].height = 6;

	rCard_Num[1].Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP3));
	GetObject(rCard_Num[1].Hbitmap, sizeof(BITMAP), &rCard_Num[1].bmp);
	rCard_Num[1].spriteCount = 17; rCard_Num[1].row = 1; rCard_Num[1].width = 5; rCard_Num[1].height = 6;

	rCard_BG.Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP1));
	GetObject(rCard_BG.Hbitmap, sizeof(BITMAP), &rCard_BG.bmp);
	rCard_BG.spriteCount = 5; rCard_BG.row = 1; rCard_BG.width = 36; rCard_BG.height = 48;

	rLisette.Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP6));
	GetObject(rLisette.Hbitmap, sizeof(BITMAP), &rLisette.bmp);
	rLisette.spriteCount = 4; rLisette.row = 1; rLisette.width = 96; rLisette.height = 128;

	rDali.Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP5));
	GetObject(rDali.Hbitmap, sizeof(BITMAP), &rDali.bmp);
	rDali.spriteCount = 7; rDali.row = 1; rDali.width = 96; rDali.height = 128;

	rCardPointer.Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP8));
	GetObject(rCardPointer.Hbitmap, sizeof(BITMAP), &rCardPointer.bmp);
	rCardPointer.spriteCount = 1; rCardPointer.row = 1; rCardPointer.width = 38; rCardPointer.height = 4;

	rTipBox.Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP10));
	GetObject(rTipBox.Hbitmap, sizeof(BITMAP), &rTipBox.bmp);
	rTipBox.spriteCount = 2; rTipBox.row = 2; rTipBox.width = 240; rTipBox.height = 16;

	old_bmp = (HBITMAP)SelectObject(bmpDC, rCard_BG.Hbitmap); //기존 변수 저장
	old_render = (HBITMAP)SelectObject(renderDC, renderBmp);
	//old_font = (HFONT)SelectObject(renderDC, hFont);
}

void Unload_bitResource()
{
	for (int i = 0; i < SPRITE_RENDERING_LAYERSIZE; i++) {
		Group_FreeAll(RenderList_Sprite[i]);
	}
	Group_FreeAll(RenderList_Text);

	//기존 변수 복구
	SelectObject(bmpDC, old_bmp);
	SelectObject(renderDC, old_render); 
	//SelectObject(renderDC, old_font);

	ReleaseDC(GetForegroundWindow(), hdc);
	DeleteObject(renderBmp);
	DeleteDC(renderDC);
	DeleteDC(bmpDC);
	//DeleteObject(renderFont);
	//RemoveFontMemResourceEx(renderFontRes); //폰트 핸들 제거

	DeleteObject(BlankPlane);
	DeleteObject(rStar_crumb.Hbitmap);
	DeleteObject(rSpFonts[0].Hbitmap);
	DeleteObject(rSpFonts[1].Hbitmap);
	DeleteObject(rCard_Num[0].Hbitmap);
	DeleteObject(rCard_Num[1].Hbitmap);
	DeleteObject(rCard_BG.Hbitmap);
	DeleteObject(rLisette.Hbitmap);
	DeleteObject(rDali.Hbitmap);
	DeleteObject(rCardPointer.Hbitmap);
	DeleteObject(rTipBox.Hbitmap);
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
	// -7 : C, Menu

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

	if (GetAsyncKeyState(0x58) & 0x8000) { // ! X key
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

	if (GetAsyncKeyState(0x43) & 0x8000) { // ! C key
		if (keyflags & 0x0040) {
			if (Key_C != NULL) Key_C(0);
		}
		else {
			keyflags = keyflags | 0x0040;
			if (Key_C != NULL) Key_C(1);
		}
	}
	else if (keyflags & 0x0040) {
		keyflags = keyflags & ~0x0040;
		if (Key_C != NULL) Key_C(-1);
	}
}

SpriteObj* TipBox_BG;
SpTextObj* TipBox_Text;
SpriteObj* Warning_BG;
SpTextObj* Warning_Text;

SpTextObj* Score_Text;
SpTextObj* LeftDeck_Text;
SpTextObj* HP_Text;
SpTextObj* DMG_Text;
SpriteObj* Lisette_Sprite;
SpriteObj* Dali_Sprite;
SpriteObj* CardPointer_Sprite;

void Start_InGame()
{
	for (int i = 0; i < 4; i++) { //구조체 초기화 작업
		CardGameObj_Initialize(&Hands[i], 0, 42 + i * (rCard_BG.width + 4), 91);
	}

	Deck = Group_Create(Card_class);
	Discarded = Group_Create(Card_class);

	//덱 세팅
	for (int i = 1; i <= 13; i++) { //스페이드 13개 카드 덱에 세팅
		Card* card = Card_Instantiate(Spade, i);
		Group_Add(Deck, card, Card_class);
	}
	for (int i = 1; i <= 13; i++) { //하트 13개 카드 덱에 세팅
		Card* card = Card_Instantiate(Heart, i);
		Group_Add(Deck, card, Card_class);
	}
	for (int i = 1; i <= 13; i++) { //클로버 13개 카드 덱에 세팅
		Card* card = Card_Instantiate(Club, i);
		Group_Add(Deck, card, Card_class);
	}
	for (int i = 1; i <= 13; i++) { //다이아 13개 카드 덱에 세팅
		Card* card = Card_Instantiate(Diamond, i);
		Group_Add(Deck, card, Card_class);
	}

	Player_HP = 10;
	Discarded_Goal = Deck->count;

	Init_InGameUI();
	Init_AnimStream();
	OnTurnStart();
}

void OnTurnStart() {
	StateGo_Idle();

	//! 매턴, 2장을 뽑음
	Try_CardDraw(1);
	Try_CardDraw(1);
}

void OnTurnEnd(int isFlee) {
	if (isFlee == 0) {
		Player_HP -= Hands_GetDMG();
	}

	//! 손패 해제 및 정리 작업
	for (int i = 0; i < HANDS_MAX; i++) {
		if (Hands[i].card != NULL) {
			Card* c = Hands[i].card;
			Card* temp;
			while (c != NULL) {
				temp = c->stacked_card;

				c->stacked_card = NULL;
				if (isFlee == 0) {
					Group_Add(Discarded, c, Card_class);
					Discarded_Goal -= 1;
				}
				else Group_Add(Deck, c, Card_class);

				c = temp;
			}
			Hands[i].card = NULL;
		}
	}

	Update_InGameUI();

	//! 게임 오버 조건
	if (Discarded_Goal < 0) {
		Discarded_Goal = 0;
		StateGo_Wait();
		TipBox_Show("You Win - survived attacks");
	}
	else if (Player_HP <= 0) {
		StateGo_Wait();
		TipBox_Show("You Lost");
	}
	else OnTurnStart();
}

void Init_InGameUI() {
	TipBox_BG = SpriteObj_Instantiate(&rTipBox, 0, 0, 128);
	TipBox_Text = SpTextObj_Create("00", 3, 120, 0, SpText_UpperMiddle, 1);

	Warning_BG = SpriteObj_Instantiate(&rTipBox, 1, 0, 56);
	Warning_Text = SpTextObj_Create("00", 4, 120, 60, SpText_UpperMiddle, 1);

	Score_Text = SpTextObj_Create("00 - 00", 3, 120, 83, SpText_LowerMiddle, 0);
	LeftDeck_Text = SpTextObj_Create("Deck 00", 3, 80, 83, SpText_LowerRight, 0);
	HP_Text = SpTextObj_Create("HP 10", 3, 160, 83, SpText_LowerLeft, 0);
	DMG_Text = SpTextObj_Create("-00", 2, 161, 76, SpText_LowerLeft, 0);

	Lisette_Sprite = SpriteObj_Instantiate(&rLisette, 0, -34, 0);
	Dali_Sprite = SpriteObj_Instantiate(&rDali, 0, 173, 0);
	CardPointer_Sprite = SpriteObj_Instantiate(&rCardPointer, 0, 0, 0);

	Group_Add(RenderList_Text, Score_Text, SpTextObj_class);
	Group_Add(RenderList_Text, LeftDeck_Text, SpTextObj_class);
	Group_Add(RenderList_Text, HP_Text, SpTextObj_class);
	Group_Add(RenderList_Sprite[0], Lisette_Sprite, SpriteObj_class);
	Group_Add(RenderList_Sprite[0], Dali_Sprite, SpriteObj_class);
	//sGroup_Add(RenderList_Sprite[1], CardPointer_Sprite, SpriteObj_class);
}
void Update_InGameUI() {
	char temp_str[16];
	sprintf_s(temp_str, 16, "%d - %d", Hands_GetScore(1), Hands_GetScore(0));
	SpTextObj_Edit(Score_Text, temp_str);

	sprintf_s(temp_str, 16, "Deck %d/%d", Deck->count,Discarded_Goal);
	SpTextObj_Edit(LeftDeck_Text, temp_str);

	sprintf_s(temp_str, 16, "HP %d", Player_HP);
	SpTextObj_Edit(HP_Text, temp_str);

	if (Hands_GetDMG() > 0)
	{
		if(Hands_isBursted() == 0) sprintf_s(temp_str, 16, "-%d", Hands_GetDMG());
		else sprintf_s(temp_str, 16, "-%d (BURST -5)", Hands_GetDMG());
		SpTextObj_Edit(DMG_Text, temp_str);

		Group_Add(RenderList_Text, DMG_Text, SpTextObj_class);
	}
	else Group_Exclude(&DMG_Text->groupProp);
}
void Free_InGameUI() {
	Group_Exclude(&TipBox_Text->groupProp);
	Group_Exclude(&Score_Text->groupProp);
	Group_Exclude(&LeftDeck_Text->groupProp);
	Group_Exclude(&HP_Text->groupProp);
	Group_Exclude(&DMG_Text->groupProp);
	Group_Exclude(&Lisette_Sprite->groupProp);
	Group_Exclude(&Dali_Sprite->groupProp);
	Group_Exclude(&CardPointer_Sprite->groupProp);

	SpTextObj_Free(TipBox_Text);
	SpriteObj_Free(TipBox_BG);
	SpTextObj_Free(Warning_Text);
	SpriteObj_Free(Warning_BG);
	SpTextObj_Free(Score_Text);
	SpTextObj_Free(LeftDeck_Text);
	SpTextObj_Free(HP_Text);
	SpTextObj_Free(DMG_Text);
	SpriteObj_Free(Lisette_Sprite);
	SpriteObj_Free(Dali_Sprite);
	SpriteObj_Free(CardPointer_Sprite);
}


void CardGameObj_Initialize(CardGameObj* gameobj, int renderLayer, int x, int y)
{
	if (gameobj->Initialized == 1) return;

	gameobj->Initialized = 1;
	gameobj->RenderLayer = RenderList_Sprite[renderLayer];

	gameobj->card = NULL;

	gameobj->placeX = x;
	gameobj->placeY = y;

	gameobj->bg = SpriteObj_Instantiate(&rCard_BG, 0, x, y);
	gameobj->num = SpriteObj_Instantiate(&rCard_Num[0], 0, 3, 3);
	gameobj->bg->children = gameobj->num;

	// ! 루트 카드는 가장 윗 카드에 종속된 스프라이트처럼 보이지만,
	// ! 가시성을 조절할 수 있어야 하므로, 직접 렌더링 그룹에서 관리된다
	gameobj->rootCard_bg = SpriteObj_Instantiate(&rCard_BG, 4, x, y - 9);
	gameobj->rootCard_num = SpriteObj_Instantiate(&rCard_Num[0], 0, 3, 3);
	gameobj->rootCard_bg->children = gameobj->rootCard_num;

	gameobj->rootCard_suit = SpriteObj_Instantiate(&rCard_Num[0], 13, 25, 0);
	gameobj->rootCard_num->children = gameobj->rootCard_suit;

	Group_Add(gameobj->RenderLayer, gameobj->bg, SpriteObj_class);
}
void CardGameObj_Update(CardGameObj* gameobj)
{
	if (gameobj->card == NULL) { // ! 카드가 없으면, 렌더링 큐에서 스프라이트 제거
		Group_Exclude(&gameobj->bg->groupProp);
		Group_Exclude(&gameobj->rootCard_bg->groupProp);
		return;
	}
	else {
		Group_Add(gameobj->RenderLayer, gameobj->bg, SpriteObj_class);
	}

	Card* upper = Card_getUpper(gameobj->card);
	if (upper != gameobj->card) { //스택이 된 카드들이라면, 가장 밑에 깔린 카드를 표현

		Group_Add(gameobj->RenderLayer, gameobj->rootCard_bg, SpriteObj_class);

		gameobj->bg->y = gameobj->placeY + 7;
		// 가장 윗 카드 위치를 따라감
		gameobj->rootCard_bg->x = gameobj->bg->x;
		gameobj->rootCard_bg->y = gameobj->bg->y - 9;
		gameobj->rootCard_num->resource = &rCard_Num[0];

		gameobj->rootCard_suit->sprite_index = (int)gameobj->card->suit + 12;
		gameobj->rootCard_num->sprite_index = gameobj->card->value - 1;
	}
	else {
		gameobj->bg->y = gameobj->placeY;
		gameobj->rootCard_bg->y = gameobj->placeY;
		Group_Exclude(&gameobj->rootCard_bg->groupProp);
	}

	gameobj->bg->x = gameobj->placeX;

	switch (upper->suit) {
	case Spade:
		gameobj->bg->sprite_index = 0;
		gameobj->num->resource = &rCard_Num[0];
		break;
	case Club:
		gameobj->bg->sprite_index = 1;
		gameobj->num->resource = &rCard_Num[0];
		break;
	case Diamond:
		gameobj->bg->sprite_index = 2;
		gameobj->num->resource = &rCard_Num[1];
		break;
	case Heart:
		gameobj->bg->sprite_index = 3;
		gameobj->num->resource = &rCard_Num[1];
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

void TipBox_Show(char* _content) {
	SpTextObj_Edit(TipBox_Text, _content);
	//TipBox_BG->children = TipBox_Text->textline; //구현 레이어가 달라서 하면 안돼!!!! ㅅㅂ

	if (visible_TipBox == 0) {
		Anim_InstanceCut("tipbox_anim");
		Group_Add(RenderList_Text, TipBox_Text, SpTextObj_class);
		Group_Add(RenderList_Sprite[SPRITE_RENDERING_LAYERSIZE - 1], TipBox_BG, SpriteObj_class);

		Group_Add(IntAnim_Stream, IntAnim_Create(&TipBox_Text->y, 0.08f, Anim_EASE_OUT, 131, 131 - TipBox_Text->TextHeight - 1 - 3, NULL, "tipbox_anim")
			, IntAnim_class);
		Group_Add(IntAnim_Stream, IntAnim_Create(&TipBox_BG->y, 0.08f, Anim_EASE_OUT, 128, 128 - TipBox_Text->TextHeight - 1 - 3, NULL, "tipbox_anim")
			, IntAnim_class);
		//Group_Add(IntAnim_Stream, IntAnim_Create(&CameraY, 0.08f, Anim_EASE_OUT, 0, 3, NULL, "tipbox_anim"), IntAnim_class);
	}

	visible_TipBox = 1;
}

static void TipBox_groupExcluding()
{
	Group_Exclude(&TipBox_Text->groupProp);
	Group_Exclude(&TipBox_BG->groupProp);
}

void TipBox_Hide() {
	if (visible_TipBox == 1) {
		Anim_InstanceCut("tipbox_anim");
		Group_Add(IntAnim_Stream, IntAnim_Create(&TipBox_Text->y, 0.12f, Anim_EASE_IN, 131 - TipBox_Text->TextHeight-1-3, 131,
			NULL, "tipbox_anim"), IntAnim_class);
		Group_Add(IntAnim_Stream, IntAnim_Create(&TipBox_BG->y, 0.12f, Anim_EASE_IN, 128 - TipBox_Text->TextHeight-1-3, 128,
			TipBox_groupExcluding, "tipbox_anim"), IntAnim_class);
		//Group_Add(IntAnim_Stream, IntAnim_Create(&CameraY, 0.12f, Anim_EASE_IN, 3, 0,TipBox_groupExcluding, "tipbox_anim"), IntAnim_class);
	}
	else TipBox_groupExcluding();

	visible_TipBox = 0;
}

static void Warning_Hide() {
	Group_Exclude(&Warning_BG->groupProp);
	Group_Exclude(&Warning_Text->groupProp);
}

void Warning_Show(char* _content) {
	Anim_InstanceCut("warning_anim");
	SpTextObj_Edit(Warning_Text, _content);

	Group_Add(RenderList_Sprite[SPRITE_RENDERING_LAYERSIZE - 1], Warning_BG, SpriteObj_class);
	Group_Add(RenderList_Text, Warning_Text, SpTextObj_class);
	int zitter[120] = { 0 };
	for (int i = 0; i < 12; i++) {
		zitter[i * 2] = -2;
		zitter[i * 2 + 1] = 2;
	}

	Group_Add(SpriteAnim_Stream, SpriteAnim_Create(&Warning_BG->x, zitter, 120, Warning_Hide, "warning_anim"), SpriteAnim_class);
}

#pragma region Scene State

void StateGo_Wait()
{
	SceneState = Game_Wait;
	Key_Z = NULL;
	Key_X = NULL;
	Key_Horizontal = NULL;
	Key_Vertical = NULL;

	Update_InGameUI();
}

void StateGo_Idle()
{
	SceneState = Game_Idle;
	Key_Z = StateTry_StackUp;
	Key_X = InGame_General_Undo;
	Key_Horizontal = SelectHands;
	Key_Vertical = SelectAction;

	Update_InGameUI();
	TipBox_Hide();
}

void StateGo_StackUp()
{
	if (Pointing_Hand == -1) return;
	if (Hands[Pointing_Hand].card == NULL) return;

	SceneState = Game_StackUp;
	Key_Z = Try_CardStack;
	Key_X = InGame_General_Undo;
	Key_Horizontal = SelectHands;
	Key_Vertical = NULL; //TODO 여기에 카드 전체 펼치기 기능을 넣는건?

	Picked_Pile = Pointing_Hand;
	Hands[Picked_Pile].placeY -= 8;

	CardPointer_Hide();
	TipBox_Show("Select a card slot to stack up the pile");
}

// ! 상태 진입시, 변경한 사항 정리
static void StateOut_StackUp()
{
	Hands[Picked_Pile].placeY += 8;
	Picked_Pile = -1;
}

void StateGo_Draw() {
	SceneState = Game_Draw;
	Key_Z = Try_CardDraw;
	Key_X = InGame_General_Undo;
	Key_Horizontal = NULL;
	Key_Vertical = SelectAction;

	CardPointer_Hide();
	TipBox_Show("Press Z to draw a card");
}

void StateGo_Stand() {
	SceneState = Game_Stand;
	Key_Z = ConfirmTurnEnd; //TODO stand, flee 실제 기능 함수 추가
	Key_X = InGame_General_Undo;
	Key_Horizontal = SelectTurnEnd;
	Key_Vertical = SelectAction;

	CardPointer_Hide();
	Selected_Option = 0;
	SelectTurnEnd(1, 0); //선택지 시각 효과 초기화
}

void ConfirmTurnEnd(int event) {
	if (event != -1) return; //KeyUp에 반응
	if (Selected_Option >= 2) return;

	TipBox_Hide();

	if (Selected_Option == 1) FleeUsed = 1;
	OnTurnEnd(Selected_Option);
}

void SelectTurnEnd(int event, int direc) {
	if (event != 1) return;

	if(FleeUsed == 0) Selected_Option = (Selected_Option + 2 + direc) % 2;
	else Selected_Option = 0;
	//여기에 선택지 시각적 효과 보이기
	switch (Selected_Option) {
	case 0:
		if (FleeUsed == 0) TipBox_Show("< STAND > - FLEE");
		else TipBox_Show("< STAND >");
		break;
	case 1:
		TipBox_Show("STAND - < FLEE >");
		break;
	}
}

void Try_CardStack(int event) {
	if (event != 1) return;

	if (Pointing_Hand == -1) return;
	if (Hands[Picked_Pile].card == NULL) return;
	
	if (Card_StackUp(Hands[Pointing_Hand].card, Hands[Picked_Pile].card)) {
		Hands[Picked_Pile].card = NULL;
		InGame_General_Undo(1);
		return;
	}
	else {
		Warning_Show("Incorrect card stacking");
	}
}

void Try_CardDraw(int event) {
	if (event != 1) return;

	int index = -1;
	for (int i = 0; i < HANDS_MAX; i++) {
		if (Hands[i].card == NULL) {
			index = i;
			break;
		}
	}

	if (index == -1) {
		Warning_Show("Hands are full");
		return;
	}
	if (Deck->count <= 0) {
		if (Discarded->count > 0) {
			GroupMeta* temp;
			temp = Deck;
			Deck = Discarded;
			Discarded = temp;
		}
		else {
			Warning_Show("Deck is empty");
			return;
		}
	}

	Hands[index].card = (Card*)Group_ExcludeByIndex(Deck, rand() % Deck->count);
	Update_InGameUI();
}

void SelectHands(int event, int direc) {
	if (event != 1) return; //오직 KeyDown에만 반응

	for (int i = 0; i < HANDS_MAX; i++) { //direc 방향으로 선택 가능한 카드를 찾음 (중간에 빈 구조체가 있을 수 있으니까)
		Pointing_Hand = (Pointing_Hand + direc + HANDS_MAX) % HANDS_MAX;
		if (SceneState == Game_StackUp && Pointing_Hand == Picked_Pile) continue; //스택 기능을 사용중일 때, 자기 자신을 선택할 수는 없음
		if (Hands[Pointing_Hand].card != NULL)
		{
			Group_Add(RenderList_Sprite[1], CardPointer_Sprite, SpriteObj_class);
			CardPointer_Sprite->x = Hands[Pointing_Hand].rootCard_bg->x-1;
			CardPointer_Sprite->y = Hands[Pointing_Hand].rootCard_bg->y - 5;
			return;
		}
	}

	CardPointer_Hide();
}

void CardPointer_Hide() {
	Pointing_Hand = -1;
	Group_Exclude(&CardPointer_Sprite->groupProp);
}

void StateTry_StackUp(int event)
{
	if (event != 1) return;
	if (SceneState != Game_Idle) return;
	StateGo_StackUp();
}

void SelectAction(int event, int direc) {
	if (event != 1 || direc == 0) return;
	if (SceneState == Game_Draw || SceneState == Game_Stand) {
		InGame_General_Undo(1);
		return;
	}
	if (SceneState != Game_Idle) return;

	if (direc == 1) // ! Up
	{
		StateGo_Draw();
		//게임 모드 변경 -> Draw
	}
	else if (direc == -1)
	{
		StateGo_Stand();
		//게임 모드 변경 -> Stand
	}
}

void InGame_General_Undo(int event) {
	if (event != 1) return;

	switch (SceneState) {
	case Game_StackUp:
		StateOut_StackUp();
		StateGo_Idle();
		break;
	case Game_Draw:
		StateGo_Idle();
		break;
	case Game_Stand:
		StateGo_Idle();
		break;
	}
}
#pragma endregion

#pragma region Game Score

int Hands_GetScore(int aceIsOne) {
	int sum = 0;
	Card* temp;
	for (int i = 0; i < HANDS_MAX; i++) {
		if (Hands[i].card == NULL) continue;
		temp = Card_getUpper(Hands[i].card);

		if (temp->value == 1 && aceIsOne == 0) sum += 11;
		else if(temp->value >= 10) sum += 10;
		else sum += temp->value;
	}

	return sum;
}

int Hands_isBursted() {
	int min = Hands_GetScore(0);
	if (min > Hands_GetScore(1)) min = Hands_GetScore(1);

	if (min > 21) return 1;
	else return 0;
}

int Hands_GetDMG() {
	int min = Hands_GetScore(0);
	if (min > 21) min = Hands_GetScore(1);

	min -= 21;

	if (min <= 0) return -min;
	else //버스트를 피할 수 없는 경우
	{
		return min + BURST_DMG;
	}
}

#pragma endregion

#pragma region Game Variable

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
	if (spr == NULL) {
		return NULL; //printf("failed spriteObj instantiate");
	}

	spr->resource = resource;
	spr->sprite_index = index;
	spr->palette_offset = 0;
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
	SpriteObj_Print_wScale(tempS, Scale);
}

void SpriteObj_Print_wScale(SpriteObj* tempS, int _scale) {
	static int preX = 0, preY = 0;
	// 재귀 함수 형태로 자식 객체도 이어서 프린트 할때,
	// 자식 객체의 좌표는 부모의 좌표에서 상대적으로 계산하므로, 부모의 절대 좌표를 기억하기 위해 사용함.

	int x, y; //Dest X,Y 계산용
	x = (tempS->x + preX);
	y = (tempS->y + preY);

	int minCols = tempS->resource->spriteCount / tempS->resource->row;
	if (tempS->resource->spriteCount % tempS->resource->row != 0) minCols++;

	int sp_x = tempS->sprite_index % minCols, sp_y = tempS->sprite_index / minCols;

	SelectObject(bmpDC, tempS->resource->Hbitmap);

	TransparentBlt(renderDC, x * _scale - CameraX * Scale, y * _scale - CameraY * Scale, tempS->resource->width * _scale, tempS->resource->height * _scale,
		bmpDC, tempS->resource->width * sp_x, tempS->resource->height * sp_y, tempS->resource->width, tempS->resource->height,
		RGB(255, 0, 255));

	if (tempS->children != NULL) {
		preX = x; preY = y;

		if (tempS->children->groupProp.affiliated != NULL) {
			Group_Exclude(&tempS->children->groupProp);
			//printf("\nSpriteObj의 자식 객체가 이중 렌더링 상태임");
		}

		SpriteObj_Print_wScale(tempS->children, _scale);
	}
	else {
		preX = 0; preY = 0;
	}
}

SpriteObj* SpriteObj_CreateText(char* _content, int x, int y, int* _lineCount, int* _maxlineLength, int color) {
	// ! 글자 스프라이트 생성
	int cuml_lineChar = 0; //이 줄에 누적된 x 자리
	int max_lineLength = 0; //텍스트 정렬을 위해 가로 최대 길이 수집
	int lineCount = 1;
	int next_x = 0, next_y = 0;
	SpriteObj *temp, *prev, *start;
	start = NULL;
	prev = NULL;
	//TODO 현재 이 코드는 왼쪽 정렬로만 작동함, AlignStyle은 전체 텍스트 크기에 대해서 정렬은 지원하지만, 글 내부 텍스트에 대해 정렬을 지원하지는 않음
	for (int i = 0; i < strlen(_content); i++) {
		if (_content[i] >= 32 && _content[i] <= 126) {
			temp = SpriteObj_Instantiate(&rSpFonts[color], _content[i] - 32, next_x, next_y);

			if (prev != NULL) prev->children = temp;
			else start = temp; //이 글자가 첫 글자라면,
			prev = temp;

			next_x = SpFont_Sizes[_content[i]-32];
			next_y = 0;
			cuml_lineChar += next_x;
		}
		else if (_content[i] == '\n') {
			if (max_lineLength < cuml_lineChar) max_lineLength = cuml_lineChar;
			next_x = -cuml_lineChar + next_x;
			next_y = SP_FONT_LINEHEIGHT;
			cuml_lineChar = 0;
			lineCount++;
		}
	}
	if (max_lineLength < cuml_lineChar) max_lineLength = cuml_lineChar;

	if (_lineCount != NULL) *_lineCount = lineCount;
	if (_maxlineLength != NULL) *_maxlineLength = max_lineLength;
	if (start == NULL) {
		return NULL;
	}

	start->x = x;
	start->y = y;

	return start;
}
// TODO 이거 그냥 CreateText랑 합칠 수 있을 것 같은데
void SpriteObj_EditText(SpriteObj* spText, char* _content, int* _lineCount, int* _maxlineLength, int color) {
	// ! 글자 스프라이트 생성
	int cuml_lineChar = 0; //이 줄에 누적된 x 자리
	int max_lineLength = 0; //텍스트 정렬을 위해 가로 최대 길이 수집
	int lineCount = 1;
	int next_x = 0, next_y = 0;
	SpriteObj* temp, * prev;
	temp = spText;
	prev = NULL;
	//TODO 현재 이 코드는 왼쪽 정렬로만 작동함, AlignStyle은 전체 텍스트 크기에 대해서 정렬은 지원하지만, 글 내부 텍스트에 대해 정렬을 지원하지는 않음
	for (int i = 0; i < strlen(_content); i++) {
		if (_content[i] >= 32 && _content[i] <= 126) {
			if (temp == NULL) {
				temp = SpriteObj_Instantiate(&rSpFonts[color], _content[i] - 32, next_x, next_y);

				if (prev != NULL) prev->children = temp;
			}
			else {
				temp->resource = &rSpFonts[color];
				temp->sprite_index = _content[i] - 32;
				temp->x = next_x;
				temp->y = next_y;
			}
			prev = temp;
			temp = temp->children;


			next_x = SpFont_Sizes[_content[i] - 32];
			next_y = 0;
			cuml_lineChar += next_x;
		}
		else if (_content[i] == '\n') {
			if (max_lineLength < cuml_lineChar) max_lineLength = cuml_lineChar;
			next_x = -cuml_lineChar + next_x;
			next_y = SP_FONT_LINEHEIGHT;
			cuml_lineChar = 0;
			lineCount++;
		}
	}
	if (max_lineLength < cuml_lineChar) max_lineLength = cuml_lineChar;

	if (_lineCount != NULL) *_lineCount = lineCount;
	if (_maxlineLength != NULL) *_maxlineLength = max_lineLength;

	if (temp != NULL) {
		prev->children = NULL;
		SpriteObj_Free(temp); //남은 문자 스프라이트 처리
	}
}

SpTextObj* SpTextObj_Create(char* _content, int _Scale, int x, int y, SpTextAlign _align, int color) {
	if (_content == NULL) return NULL;
	if (strlen(_content) <= 0) return NULL;
	
	SpTextObj* spText;
	spText = malloc(sizeof(SpTextObj));
	if (spText == NULL) return NULL; //printf("failed spTextObj instantiate");

	spText->color = color;
	int lineCount = 1, TextWidth = 0;
	spText->textline = SpriteObj_CreateText(_content, x, y, &lineCount, &TextWidth, color);

	spText->TextWidth = (int)(TextWidth * (float)_Scale / Scale);
	spText->TextHeight = (int)(lineCount * SP_FONT_LINEHEIGHT * (float)_Scale / Scale);
	spText->textScale = _Scale;

	spText->x = x;
	spText->y = y;
	spText->alignStyle = _align;
	GroupProperty_Initialize(&(spText->groupProp));

	return spText;
}

void SpTextObj_Edit(SpTextObj* spText, char* _content) {
	int lineCount = 1, TextWidth = 0;
	SpriteObj_EditText(spText->textline, _content, &lineCount, &TextWidth, spText->color);

	spText->TextWidth = (int)(TextWidth * (float)spText->textScale / Scale);
	spText->TextHeight = (int)(lineCount * SP_FONT_LINEHEIGHT * (float)spText->textScale / Scale);
}

void SpTextObj_Print(SpTextObj* spText) {
	SpriteObj* lineStart = spText->textline;

	spText->textline->x = spText->x;
	spText->textline->y = spText->y;

	switch (spText->alignStyle) {
	case SpText_UpperMiddle:
		spText->textline->x -= spText->TextWidth / 2;
		break;
	case SpText_UpperRight:
		spText->textline->x -= spText->TextWidth;
		break;
	case SpText_LowerLeft:
		spText->textline->y -= spText->TextHeight;
		break;
	case SpText_LowerMiddle:
		spText->textline->y -= spText->TextHeight;
		spText->textline->x -= spText->TextWidth / 2;
		break;
	case SpText_LowerRight:
		spText->textline->y -= spText->TextHeight;
		spText->textline->x -= spText->TextWidth;
		break;
	}

	spText->textline->x = (int)(spText->textline->x * (float)Scale / spText->textScale);
	spText->textline->y = (int)(spText->textline->y * (float)Scale / spText->textScale);

	SpriteObj_Print_wScale(lineStart, spText->textScale);
}

void SpTextObj_Free(SpTextObj* spText) {
	SpriteObj_Free(spText->textline);
	free(spText);
}

#pragma endregion