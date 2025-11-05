#include <stdio.h>
#include <windows.h>
#include "resource.h"
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "Msimg32.lib") //TransparentBlt 사용

/*
 ### 객체 생성 기준 ###
 - 값을 전체 복사해서 관리하지 않는다
 - 함수에서 생성했는데 외부에서도 지속적으로 사용하고 싶다
 - 전역 변수같이 하드 코딩으로 만들어진 저장공간에 계속 존재하지 않는다. 즉, 생성과 파괴가 잦다
		-> 리스트에 보관되면 객체임
*/

typedef enum {
	Spade = 1,
	Heart = 2,
	Diamond = 3,
	Club = 4
} Suit;

//객체
typedef struct Card{
	Suit suit;
	int value;

	struct Card* stacked_card; //이 카드 위에 있는 카드
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
} SpriteObj;

typedef enum {
	NoneInstance, //구성 원소의 data값이 비었다는 표시
	Card_class,
	SpriteObj_class
} InstanceTag;

typedef union {
	Card* card;
	SpriteObj* sprite;
	//여기에 애니메이션 관련 변수 추가a
} Instance;

//객체
typedef struct ListElement{
	Instance data;
	struct ListElement* next;
	struct ListElement* previous;
} ListElement;

//객체
typedef struct {
	InstanceTag valueType;

	ListElement* front;
	ListElement* rear;
	int count;
} ListMeta;

/// <summary>
/// 카드가 뽑혔을 때, 스프라이트와 카드 객체를 실체로서 다루기 위한 오브젝트
/// 일단은 객체가 아니라 구조체로 관리해봄.
/// </summary>
typedef struct CardGameObj {
	Card* card;

	//카드 더미의 가장 윗 카드를 표현하기 위한 오브젝트
	SpriteObj* bg;
	SpriteObj* num;

	//가장 밑에 깔린 카드를 표현하기 위한 오브젝트
	SpriteObj* rootCard_bg;
	SpriteObj* rootCard_num;

	short Initialized; //스프라이트들 초기화 여부 반환
} CardGameObj;

ListMeta Pooling_List; //Listelement 를 위한 풀

ListMeta* List_Create(InstanceTag valueType);

/// <summary>
/// source list에 자료를 추가합니다
/// </summary>
/// <returns>리스트가 담는 자료형과 일치하지 않으면 0을 반환</returns>
int List_AddNew(ListMeta* source, Instance instance, InstanceTag type);

/// <summary>
/// 기존 리스트 원소를 다른 리스트 메타에 추가함
/// </summary>
int List_AddElement(ListMeta* meta, ListElement* element, InstanceTag type);

/// <summary>
/// 특정 리스트 원소를 리스트 요소의 연결에서 제거함
/// 리스트에서 빼낸 원소를 그대로 다른 리스트에 옮길 수 있도록 하기 위함
/// </summary>
/// <returns>파라미터에 넣은 element 그 자체가 나옵니다</returns>
void List_Extract(ListMeta* meta, ListElement* element);

ListElement* List_ExtractByIndex(ListMeta* meta, int index);

/// <summary>
/// 리스트 원소만 제거함, 내부 데이터의 객체는 유지함 (메모리 누수 주의)
/// </summary>
void List_FreeElement(ListElement* element);

/// <summary>
/// 리스트 원소와 그 내부 데이터의 객체도 제거함
/// </summary>
void List_FreeElement_wData(InstanceTag tag, ListElement* element);

/// <summary>
/// 리스트 전체와 그 구성요소를 메모리 해제함
/// </summary>
void List_FreeALL(ListMeta* source);

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

SpriteObj* SpriteObj_Instantiate(bitResource* resource, int index, int x, int y);

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
void CardGameObj_Initialize(CardGameObj* gameobj);

/// <summary>
/// 현재 카드 상태에 따라 스프라이트 상태를 변경합니다
/// </summary>
void CardGameObj_Update(CardGameObj* gameobj);

/// <summary>
/// 내부 스프라이트와 카드를 모두 메모리 해제 함
/// </summary>
void CardGameObj_Free(CardGameObj* gameobj);

HDC hdc;
HDC memDC; //비트맵 출력을 위한 임시 캔버스
HBITMAP oldHbitmap;

HBRUSH GameBG;
bitResource Card_BG;
bitResource Card_Num[2]; //0 어두운 것, 1 밝은 것

//스프라이트 확대 수준
int Scale = 4;
int offsetX = 8, offsetY = 32;

void Pool_SetUp()
{
	Pooling_List.valueType = NoneInstance;
	Pooling_List.front = NULL;
	Pooling_List.rear = NULL;
	Pooling_List.count = 0;
}

void Pool_Clear() {
	List_FreeALL(&Pooling_List);
}

void Load_bitResource()
{
	hdc = GetWindowDC(GetForegroundWindow()); // Handle to Device Context
	memDC = CreateCompatibleDC(hdc);

	GameBG = CreateSolidBrush(RGB(42, 23, 59));

	Card_Num[0].Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP2));
	GetObject(Card_Num[0].Hbitmap, sizeof(BITMAP), &Card_Num[0].bmp);
	Card_Num[0].spriteCount = 17; Card_Num[0].width = 5; Card_Num[0].height = 6;

	Card_Num[1].Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP3));
	GetObject(Card_Num[1].Hbitmap, sizeof(BITMAP), &Card_Num[1].bmp);
	Card_Num[1].spriteCount = 17; Card_Num[1].width = 5; Card_Num[1].height = 6;

	Card_BG.Hbitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BITMAP1));
	GetObject(Card_BG.Hbitmap, sizeof(BITMAP), &Card_BG.bmp);
	Card_BG.spriteCount = 5; Card_BG.width = 36; Card_BG.height = 48;

	oldHbitmap = (HBITMAP)SelectObject(memDC, Card_BG.Hbitmap); //기존 변수 저장
}

void Unload_bitResource()
{
	SelectObject(memDC, oldHbitmap); //기존 변수 복구
	ReleaseDC(GetForegroundWindow(), hdc);
	DeleteDC(memDC);

	DeleteObject(GameBG);
	DeleteObject(Card_Num[0].Hbitmap);
	DeleteObject(Card_Num[1].Hbitmap);
	DeleteObject(Card_BG.Hbitmap);
}

int main() {
	CardGameObj Hands[4];
	for (int i = 0; i < 4; i++) { //구조체 초기화 작업
		Hands[i].card = NULL;
		Hands[i].bg = NULL;
		Hands[i].num = NULL;
		Hands[i].rootCard_bg = NULL;
		Hands[i].rootCard_num = NULL;
	}

	Load_bitResource();

	system("title BlackJack Game"); //콘솔창 정보 수정
	system("mode con:cols=120 lines=32"); //960,512 offset(+8,+32)

	CONSOLE_CURSOR_INFO cursor = { 0, }; //커서 가리기
	cursor.dwSize = 100;
	cursor.bVisible = FALSE;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor);

	srand(time(NULL)); //시드 초기화

	ListMeta* Deck;
	Deck = List_Create(Card_class);

	ListMeta* Sprites;
	Sprites = List_Create(SpriteObj_class); // ! 스프라이트 객체를 제거했을 때, 일시적으로 안보이게 하고 싶을 때, 렌더링 리스트에서 제거 된건지 확인할 수가 없음

	//덱 세팅
	for (int i = 1; i <= 13; i++) { //스페이드 13개 카드 덱에 세팅
		Card* card = Card_Instantiate(Spade, i);
		Instance a;
		a.card = card;
		List_AddNew(Deck, a, Card_class);
	}
	for (int i = 1; i <= 13; i++) { //하트 13개 카드 덱에 세팅
		Card* card = Card_Instantiate(Heart, i);
		Instance a;
		a.card = card;
		List_AddNew(Deck, a, Card_class);
	}

	//카드 뽑기
	for (int i = 0; i < 4; i++) {
		//카드 한장 뽑아 손패에 저장하고, 리스트 제거
		ListElement* temp = List_ExtractByIndex(Deck, rand() % Deck->count);
		Hands[i].card = temp->data.card;
		List_FreeElement(temp);

		Instance a;
		a.sprite = SpriteObj_Instantiate(&Card_BG, 0,i*(Card_BG.width+2), 32);
		a.sprite->children = SpriteObj_Instantiate(&Card_Num, Hands[i].card->value - 1, 3, 3); //카드의 숫자를 카드 본체의 자식 객체로 지정
		
		Hands[i].bg = a.sprite;
		Hands[i].num = a.sprite->children;

		List_AddNew(Sprites, a, SpriteObj_class);
	}


	
	Sleep(500);

	//그래픽 루프 전, 게임오브젝트 업데이트
	for (int i = 0; i < 4; i++) {
		CardGameObj_Update(&Hands[i]);
	}

	// 이 코드는 그래픽 루프에 포함하시오
	RECT rc = { offsetX, offsetY, 960 + offsetX, 512 + offsetY };
	FillRect(hdc, &rc, GameBG);

	ListElement* listLoop;
	listLoop = Sprites->front;
	while (listLoop != NULL) {
		SpriteObj* tempS = listLoop->data.sprite;
		SpriteObj_Print(tempS);

		listLoop = listLoop->next;
	}

	Sleep(5000);

	List_FreeALL(Deck);
	List_FreeALL(Sprites);

	/*
	Sleep(300);
	for (int i = 0; i < 15/scale; i++)
		for (int j = 0; j < 8/scale; j++) StretchBlt(hdc, bmp.bmWidth * i * scale + offset_x, bmp.bmHeight * scale * j + offset_y, bmp.bmWidth * scale, bmp.bmHeight * scale, memDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

	Sleep(10000);
	printf("Canvas would be... x: %d y: %d", 960 / scale, 512 / scale);
	*/

	/*
	int x = 30, y = 200;
	int vx = 50, vy = -50;
	int i;

	HPEN hWhitePen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
	HPEN hOldPen = (HPEN)SelectObject(hdc, hWhitePen);

	// 흰색 수평선 그리기
	MoveToEx(hdc, 30, 200, NULL);
	LineTo(hdc, 800, 200);

	SelectObject(hdc, hOldPen);
	DeleteObject(hWhitePen);

	for (i = 0; i < 100000; i++) { //이거 거의 3초만에 다 연산함 충분히 빠름
		vy = vy + 10;   // g = 9.8m/s
		x += vx;
		y += vy;

		Ellipse(hdc, x % 800, y % 200, (x%800 + 10), (y%200 + 10));
		//Sleep(100);  // take a break for 100/1000 sec
	}
	*/
	
	//getchar(); // 종료대기, 또는 system("pause");

	//리스트, 카드 등 객체들 알아서 해제해야 한다
	Unload_bitResource();
	return 0;
}

ListMeta* List_Create(InstanceTag valueType)
{
	ListMeta* meta;
	meta = malloc(sizeof(ListMeta));
	if (meta == NULL) printf("failed list meta instantiate");

	meta->valueType = valueType;
	meta->front = NULL;
	meta->rear = NULL;
	meta->count = 0;

	return meta;
}

/// <summary>
/// source list에 자료를 추가합니다
/// </summary>
/// <returns>리스트가 담는 자료형과 일치하지 않으면 0을 반환</returns>
int List_AddNew(ListMeta* meta, Instance instance, InstanceTag type) {
	if (meta->valueType != type) return 0;

	ListElement* element;

	if (Pooling_List.count <= 0) {
		element = malloc(sizeof(ListElement));
		if (element == NULL) printf("failed list element instantiate");
	}
	else {
		element = List_ExtractByIndex(&Pooling_List, 0);
	}

	element->data = instance;

	return List_AddElement(meta, element, type);
}

int List_AddElement(ListMeta* meta, ListElement* element, InstanceTag type) {
	if (meta->valueType != type) return 0;

	element->next = NULL;
	element->previous = NULL;
	if (meta->front == NULL || meta->rear == NULL) {
		meta->front = meta->rear = element;
	}
	else {
		element->previous = meta->rear;
		meta->rear->next = element;
		meta->rear = element;
	}
	meta->count++;

	return 1;
}

void List_Extract(ListMeta* meta, ListElement* element) {
	if (meta->count <= 1) { //리스트에 한개 밖에 없는 경우
		meta->front = NULL;
		meta->rear = NULL;
	}
	else if (element->previous == NULL) { //index 0, 맨 앞을 고른 경우
		meta->front = element->next;
		element->next->previous = NULL;
	}
	else if (element->next == NULL) { //마지막을 고른 경우
		meta->rear = element->previous;
		element->previous->next = NULL;
	}
	else { //앞 뒤에 원소가 존재하는 경우
		element->previous->next = element->next;
		element->next->previous = element->previous;
	}

	//모든 연결 해제
	element->previous = NULL;
	element->next = NULL;
	meta->count--;
}

ListElement* List_ExtractByIndex(ListMeta* meta, int index) {

	ListElement* element;
	if (index == 0) {
		element = meta->front;
	}
	else if (index == meta->count - 1) element = meta->rear;
	else {
		element = meta->front;
		for (int i = 0; i < index; i++) element = element->next;
	}

	List_Extract(meta, element);

	return element;
}

void List_FreeElement(ListElement* element) {
	if (Pooling_List.count < 16) List_AddElement(&Pooling_List, element, NoneInstance);
	else free(element);
}

void List_FreeElement_wData(InstanceTag tag, ListElement* element) {
	switch (tag) { //데이터는 내부의 객체는 모두 정리
	case Card_class:
		free(element->data.card);
		break;

	case SpriteObj_class:
		SpriteObj_Free(element->data.sprite);
		break;
	}
	List_FreeElement(element); //리스트 원소는 풀링
}

/// <summary>
/// 리스트 전체를 메모리 해제함
/// </summary>
void List_FreeALL(ListMeta* source) {
	if (source->count <= 0) {
		free(source);
		return;
	}

	ListElement* p;
	ListElement* temp;
	p = source->front;
	while (p != NULL) {
		temp = p;
		p = p->next;

		List_FreeElement_wData(source->valueType, temp);
	}
	free(source);
}

Card* Card_Instantiate(Suit suit, int value) {
	Card* card;
	card = malloc(sizeof(Card));
	if (card == NULL) printf("failed card instantiate");

	card->suit = suit;
	card->value = value;
	card->stacked_card = NULL;

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

SpriteObj* SpriteObj_Instantiate(bitResource* resource, int index, int x, int y) {
	SpriteObj* spr;
	spr = malloc(sizeof(SpriteObj));
	if (spr == NULL) printf("failed spriteObj instantiate");

	spr->resource = resource;
	spr->sprite_index = index;
	spr->x = x;
	spr->y = y;

	spr->children = NULL;

	return spr;
}

void SpriteObj_Free(SpriteObj* sprite) {
	if (sprite->children != NULL) SpriteObj_Free(sprite->children);
	else free(sprite);
}

void SpriteObj_Print(SpriteObj* tempS) {
	static int preX = 0, preY = 0;
	// 재귀 함수 형태로 자식 객체도 이어서 프린트 할때,
	// 자식 객체의 좌표는 부모의 좌표에서 상대적으로 계산하므로, 부모의 절대 좌표를 기억하기 위해 사용함.

	int x, y; //Dest X,Y 계산용
	x = (tempS->x + preX);
	y = (tempS->y + preY);

	SelectObject(memDC, tempS->resource->Hbitmap);
	TransparentBlt(hdc, offsetX + x * Scale, offsetY + y * Scale, tempS->resource->width * Scale, tempS->resource->height * Scale,
		memDC, tempS->resource->width * tempS->sprite_index, 0, tempS->resource->width, tempS->resource->height,
		RGB(255, 0, 255));

	if (tempS->children != NULL) {
		preX = x; preY = y;
		SpriteObj_Print(tempS->children);
	}
	else {
		preX = 0; preY = 0;
	}
}
void CardGameObj_Initialize(CardGameObj* gameobj)
{

}
void CardGameObj_Update(CardGameObj* gameobj)
{
	if (gameobj->card == NULL) return;

	Card* upper = Card_getUpper(gameobj->card);
	if (upper != gameobj->card) { //스택이 된 카드들이라면, 가장 밑에 깔린 카드를 표현
		if (gameobj->rootCard_bg == NULL) { //객체 생성과 초기화는 했는데, 출력 리스트에 추가하지 않았음
			gameobj->rootCard_bg = SpriteObj_Instantiate(&Card_BG, 4, 0, 0);
			if(gameobj->rootCard_num == NULL) gameobj->rootCard_num = SpriteObj_Instantiate(&Card_Num[0], 0, 3, 3);
			gameobj->rootCard_bg->children = gameobj->rootCard_num;
		}

		switch (gameobj->card->suit) {
		case Spade:
			gameobj->rootCard_bg->sprite_index = 0;
			gameobj->rootCard_num->resource = &Card_Num[0];
			break;
		case Club:
			gameobj->rootCard_bg->sprite_index = 1;
			gameobj->rootCard_num->resource = &Card_Num[0];
			break;
		case Diamond:
			gameobj->rootCard_bg->sprite_index = 2;
			gameobj->rootCard_num->resource = &Card_Num[1];
			break;
		case Heart:
			gameobj->rootCard_bg->sprite_index = 3;
			gameobj->rootCard_num->resource = &Card_Num[1];
			break;
		}
		gameobj->rootCard_num->sprite_index = gameobj->card->value - 1;
	}
	else if (gameobj->rootCard_bg != NULL) { //밑에 깔린 카드가 사라지는 경우 구현 -> 이 경우는 게임에서 일어날 수 없지만, 일단 구현함
		SpriteObj_Free(gameobj->rootCard_bg);
		gameobj->rootCard_bg = NULL;
		gameobj->rootCard_num = NULL;
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
	gameobj->num->sprite_index = gameobj->card->value - 1;
}

void CardGameObj_Free(CardGameObj* gameobj)
{
}
