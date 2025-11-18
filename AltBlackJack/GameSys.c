#include "GameSys.h"
#include "resource.h"
#include <stdlib.h>
#include <string.h>

GroupMeta* RenderList_Sprite[SPRITE_RENDERING_LAYERSIZE];
GroupMeta* RenderList_Text;

HDC hdc;
HDC renderDC; //렌더링을 위한 버퍼
HBITMAP renderBmp;
HFONT renderFont;
HANDLE renderFontRes; //폰트를 시스템에 일시 등록하기 위해 사용한 핸들

HDC bmpDC; //비트맵 출력을 위한 임시 캔버스

static HBITMAP old_bmp;
static HBITMAP old_render;
static HFONT old_font;

HBRUSH GameBG;
bitResource Star_crumb;
bitResource Card_BG;
bitResource Card_Num[2]; //0 어두운 것, 1 밝은 것

void(*Key_Z)(int) = NULL;
void(*Key_X)(int) = NULL;
void(*Key_C)(int) = NULL;
void(*Key_Horizontal)(int, int) = NULL;
void(*Key_Vertical)(int, int) = NULL;

//스프라이트 확대 수준
int Scale = 4;
int offsetX = 8, offsetY = 32;

GameState SceneState = Game_Idle;

GroupMeta* Deck;
CardGameObj Hands[HANDS_MAX];
int Pointing_Hand; //방향키로 포인팅된 카드
int Picked_Pile = -1; //스택 기능을 사용할 때, 들어올려진 카드
static int Selected_Option; //선택지 지정 상황에서 사용되는 변수

void Load_bitResource()
{
	for (int i = 0; i < SPRITE_RENDERING_LAYERSIZE; i++) {
		RenderList_Sprite[i] = Group_Create(SpriteObj_class);
	}
	RenderList_Text = Group_Create(FontTextObj_class);

	hdc = GetWindowDC(GetForegroundWindow()); // Handle to Device Context
	renderDC = CreateCompatibleDC(hdc);
	renderBmp = CreateCompatibleBitmap(hdc, 960, 512);
	bmpDC = CreateCompatibleDC(hdc);

	// 폰트 설정 코드는 이해를 못하겠습니다 ㅎㅎ;;
	// ! 리소스에서 폰트 로드
	HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(GetForegroundWindow(), GWLP_HINSTANCE);
	HRSRC hRes = FindResource(hInst, MAKEINTRESOURCE(IDR_FONT1), RT_FONT);
	HGLOBAL hFontData = LoadResource(hInst, hRes);
	void* pFontData = LockResource(hFontData);
	DWORD fontSize = SizeofResource(hInst, hRes);

	// ! OS에 임시로 폰트 등록
	DWORD nFonts;
	renderFontRes = AddFontMemResourceEx(pFontData, fontSize, NULL, &nFonts);

	// ! Font 변수 생성
	LOGFONT lf = { 0 };
	lf.lfHeight = 18; //폰트 크기
	lstrcpy(lf.lfFaceName, TEXT("MyFontName")); // 리소스 폰트 이름
	HFONT hFont = CreateFontIndirect(&lf);

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
	old_font = (HFONT)SelectObject(renderDC, hFont);
}

void Unload_bitResource()
{
	for (int i = 0; i < SPRITE_RENDERING_LAYERSIZE; i++) {
		Group_FreeAll(RenderList_Sprite[i]);
	}

	//기존 변수 복구
	SelectObject(bmpDC, old_bmp);
	SelectObject(renderDC, old_render); 
	SelectObject(renderDC, old_font);

	ReleaseDC(GetForegroundWindow(), hdc);
	DeleteObject(renderBmp);
	DeleteDC(renderDC);
	DeleteDC(bmpDC);
	DeleteObject(renderFont);
	RemoveFontMemResourceEx(renderFontRes); //폰트 핸들 제거

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

#pragma region Scene State

void StateGo_Idle()
{
	SceneState = Game_Idle;
	Key_Z = StateTry_StackUp;
	Key_X = InGame_General_Undo;
	Key_Horizontal = SelectHands;
	Key_Vertical = SelectAction;
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
	Hands[Picked_Pile].bg->y += 6;
}

// ! 상태 진입시, 변경한 사항 정리
static void StateOut_StackUp()
{
	Hands[Picked_Pile].bg->y -= 6;
	Picked_Pile = -1;
}

void StateGo_Draw() {
	SceneState = Game_Draw;
	Key_Z = Try_CardDraw;
	Key_X = InGame_General_Undo;
	Key_Horizontal = NULL;
	Key_Vertical = SelectAction;
}

void StateGo_Stand() {
	SceneState = Game_Stand;
	Key_Z = NULL; //TODO stand, flee 실제 기능 함수 추가
	Key_X = InGame_General_Undo;
	Key_Horizontal = ConfirmTurnEnd;
	Key_Vertical = SelectTurnEnd;

	Selected_Option = 0;
	SelectTurnEnd(1, 0); //선택지 시각 효과 초기화
}

void ConfirmTurnEnd(int event) {
	if (event != -1) return; //KeyUp에 반응
	if (Selected_Option >= 2) return;

	switch (Selected_Option) {
	case 0: //STAND
		break;
	case 1: //FLEE
		break;
	}
}

void SelectTurnEnd(int event, int direc) {
	if (event != 1) return;

	Selected_Option = (Selected_Option + 2 + direc) % 2;
	//여기에 선택지 시각적 효과 보이기
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
		//TODO 경고 피드백 띄우기
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
		//TODO 경고 피드백
		return;
	}
	if (Deck->count <= 0) {
		//TODO 경고 피드백
		return;
	}

	Hands[index].card = (Card*)Group_ExcludeByIndex(Deck, rand() % Deck->count);
}

void SelectHands(int event, int direc) {
	// TODO 이 영역은 테스트 코드 입니다
	static SpriteObj* star = NULL;
	if (star == NULL) {
		star = SpriteObj_Instantiate(&Star_crumb, 0, 8, 8); // ! 렌더링 큐에 포함되면, 프로그램 종료시, 자동으로 메모리 해제됨
		//애니메이터 생성해서 추가해주기
	}
	// TODO #############################

	if (event != 1) return; //오직 KeyDown에만 반응

	for (int i = 0; i < HANDS_MAX; i++) { //direc 방향으로 선택 가능한 카드를 찾음 (중간에 빈 구조체가 있을 수 있으니까)
		Pointing_Hand = (Pointing_Hand + direc + HANDS_MAX) % HANDS_MAX;
		if (SceneState == Game_StackUp && Pointing_Hand == Picked_Pile) continue; //스택 기능을 사용중일 때, 자기 자신을 선택할 수는 없음
		if (Hands[Pointing_Hand].card != NULL)
		{
			Group_Add(RenderList_Sprite[1], star, SpriteObj_class);
			star->x = Hands[Pointing_Hand].bg->x + 8;
			star->y = Hands[Pointing_Hand].bg->y - 16;
			return;
		}
	}

	Pointing_Hand = -1;
	Group_Exclude(&star->groupProp);

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
		if (temp->value == 1 && aceIsOne != 0) {
			sum += 11;
		}
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
	if (min > Hands_GetScore(1)) min = Hands_GetScore(1);

	min -= 21;

	if (min >= 0) return min;
	else //버스트를 피할 수 없는 경우
	{
		return -min + BURST_DMG;
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

FontTextObj* FontTextObj_Instantiate(char* _content, int _x, int _y, int _align, int _color, int _size) {
	FontTextObj* fontT;
	fontT = malloc(sizeof(FontTextObj));
	if (fontT == NULL) return NULL; //printf("failed FontTextObj instantiate");

	strcpy_s(fontT->content, FONT_TEXT_MAX_STR_LEN, _content);
	fontT->x = _x; fontT->y = _y;
	fontT->align = _align;
	fontT->color = _color;
	fontT->sizeScale = _size;
}

void FontTextObj_Print(FontTextObj* text) {
	SetTextAlign(renderDC, text->align);
	SetBkColor(renderDC, TRANSPARENT);
	SetTextColor(renderDC, RGB(255, 255, 255)); //TODO 나중에 색상 변수대로 처리 추가

	TextOut(renderDC, text->x, text->y, text->content, strlen(text->content));
}

void CardGameObj_Initialize(CardGameObj* gameobj, int renderLayer, int x, int y)
{
	if (gameobj->Initialized == 1) return;

	gameobj->Initialized = 1;
	gameobj->RenderLayer = RenderList_Sprite[renderLayer];

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

#pragma endregion