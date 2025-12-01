#pragma once
#include "Grouplist.h"
#include <windows.h>
#include <time.h>

#define SPRITE_RENDERING_LAYERSIZE 4
#define SP_FONT_COUNT 95
#define SP_FONT_LINEHEIGHT 9

#define HANDS_MAX 4
#define BURST_DMG 5

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
	
	int row; //행 갯수
	//스프라이트 1개당 이미지 크기
	int width;
	int height;
} bitResource;

//객체
typedef struct SpriteObject {
	bitResource* resource;
	int sprite_index; //사용할 스프라이트 번호
	int x; int y; //화면 상 위치, 자식 객체인 경우, 부모 객체의 위치에서 상대적인 좌표로 계산함
	// TODO 로드된 비트맵에서 DIB section을 따로 생성해줘야 palette swap이 가능하다 - 안만들거임 ㅅㅂ
	int palette_offset; //색상 밝기 조절 (어둡게- ~ 밝게+)

	struct SpriteObject* children; //이 스프라이트에 종속된 추가적인 스프라이트

	GroupProperty groupProp;
} SpriteObj;

typedef enum {
	SpText_UpperLeft,
	SpText_UpperMiddle,
	SpText_UpperRight,
	SpText_LowerLeft,
	SpText_LowerMiddle,
	SpText_LowerRight
} SpTextAlign;

typedef struct SpTextObject {
	SpriteObj* textline;
	int color;
	int x; int y; //현재 스케일 단위 상의, 좌표값을 의미함
	SpTextAlign alignStyle;
	int textScale;

	// 고유의 크기로 생성된 글자들이, 현재 렌더링 스케일 단위에서 몇 픽셀 크기인지 나타냄
	// ! 따로 수정하지 말것
	int TextWidth;
	// ! 따로 수정하지 말것
	int TextHeight;

	GroupProperty groupProp;
} SpTextObj;

/// <summary>
/// 카드가 뽑혔을 때, 스프라이트와 카드 객체를 실체로서 다루기 위한 오브젝트
/// 일단은 객체가 아니라 구조체로 관리해봄.
/// </summary>
typedef struct CardGameObj {
	Card* card;

	int placeX, placeY;

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

#pragma region GameEngineVar
/// <summary>
/// 0 : BG
/// 1 : GameSprite
/// 2 : Card
/// Last : UI
/// </summary>
extern GroupMeta* RenderList_Sprite[SPRITE_RENDERING_LAYERSIZE];
//작은 텍스트를 처리하는 레이어
extern GroupMeta* RenderList_Text;

extern HDC hdc;
extern HDC renderDC; //렌더링을 위한 버퍼
extern HBITMAP renderBmp;
extern HDC bmpDC; //비트맵 출력을 위한 임시 캔버스

static const int GamePalette[6][3] 
= { {0x2a, 0x17, 0x3b}, {0x3f, 0x2c, 0x5f},
	{0x44, 0x3f, 0x7b}, {0x4c, 0x5c, 0x87},
	{0x69, 0x80, 0x9e}, {0x95, 0xc5, 0xac} }; //투명 제외한 게임 컬러 팔레트

extern HBRUSH BlankPlane;
extern bitResource rSpFonts[2];
extern bitResource rStar_crumb;
extern bitResource rCard_BG;
extern bitResource rCard_Num[2]; //0 어두운 것, 1 밝은 것
extern bitResource rLisette;
extern bitResource rDali;
extern bitResource rCardPointer;
extern bitResource rTipBox;
extern bitResource rGameBG_Glow;
extern bitResource rEnemy;

extern void(*Key_Z)(int); //(키 이벤트)
extern void(*Key_X)(int); //(키 이벤트)
extern void(*Key_C)(int); //(키 이벤트)
extern void(*Key_Horizontal)(int, int); //(키 이벤트, 방향)
extern void(*Key_Vertical)(int, int); //(키 이벤트, 방향)

//스프라이트 확대 수준
extern int Scale;
extern int offsetX, offsetY;
extern int CameraX, CameraY;

extern double frameTime;
extern double frameRate;
#pragma endregion

#pragma region CardGameVar
typedef enum {
	Game_Quit = 0,
	Game_Lobby = 1,
	Game_Idle = 201, //인게임 기본 상태
	Game_StackUp, // 카드를 들어올린 상태
	Game_Draw, // 방향키 위를 눌러, 카드를 뽑을지 선택하는 상태
	Game_Stand, // 방향키 아래를 눌러, 턴을 넘길지 선택하는 상태
	Game_Wait // 연출을 위해, 입력을 제한하는 상태
} GameState;

extern GameState SceneState;
extern void(*SceneOut_Action)(); //현재 씬에서 다른 씬으로 transition하기 위해 호출해야 할 뒷정리 함수

extern GroupMeta* Deck;
extern CardGameObj Hands[HANDS_MAX];
extern int Pointing_Hand;
extern int Picked_Pile;
#pragma endregion

void Game_MainLoop();

/// <summary>
/// 스프라이트 리소스와 HDC, 렌더링 LIST 같은 전역 변수를 초기화 합니다
/// </summary>
void Load_bitResource();
void Unload_bitResource();

void Input_KeyPress(); //windows.h 같은 외부 헤더와 겹치지 않기 위한 똥꼬쇼

void SceneGo_Lobby();
void SceneOut_Lobby();

void SceneGo_InGame();
void SceneOut_InGame();

void RestartGame();
void Start_Game();

void OnTurnStart();
void OnTurnEnd(int isFlee);

void Init_InGameUI();
void Update_InGameUI();
void Free_InGameUI();

void TipBox_Show(char* _content);
void TipBox_Hide();

void Warning_Show(char* content);

void Shake_Camera_forPerfect();

//고정값의 카메라 흔들림을 제공함
void Shake_Camera(int scale);

void CharAnim_Idle_Play();
void CharAnim_Idle_Stop();

//TODO 인게임의 상태 변경 함수들이 진정으로 외부에 노출될 필요가 있나?? 나중에 Static으로 바꾸자
//TODO 메인메뉴 - 인게임 같은 씬 변경정도는 되어야 외부 노출을 해야지!!
//TOdO 아 시발 함수 선언 순서에 따라 정의 없음 에러가 뜨니까 지금까지 이렇게 적었던거구나

void StateGo_Lobby();

/// <summary>
/// Anim의 OnEnd를 기다리기 위한 상태 - 아무 조작도 할 수 없습니다
/// </summary>
void StateGo_Wait(); 

void StateGo_Idle();

void StateGo_StackUp();

void StateGo_Draw();

void StateGo_Stand();

void Try_CardStack(int event);

void Try_CardDraw(int event);

void ConfirmTurnEnd(int event);

/// <summary>
/// 게임 Idle, StackUp 중에 손패를 좌우 방향키로 선택할 수 있도록 함
/// </summary>
/// <param name="event">key input event</param>
/// <param name="direc">key direction -1 or 1</param>
void SelectHands(int event, int direc);

void CardPointer_Hide();

/// <summary>
/// 선택된 손패에 대해 StackUp 상태 전환을 시도함
/// </summary>
void StateTry_StackUp(int event);

/// <summary>
/// Idle 상태에서 상하 방향키로, Draw, Stand 상태로 바꿀 수 있도록 함
/// </summary>
/// <param name="event">key input event</param>
/// <param name="direc">key direction -1 or 1</param>
void SelectAction(int event, int direc);

/// <summary>
/// STAND 상태에 들어갔을때, FLEE를 사용할지, 그대로 턴을 넘길지 고름
/// </summary>
void SelectTurnEnd(int event, int direc);

/// <summary>
/// 인게임에서 뒤로가기 기능을 이행함
/// </summary>
void InGame_General_Undo(int event);

/// <summary>
/// 현재 손패의 점수를 계산함
/// </summary>
/// <param name="aceIsOne">0이 아니라면, Ace를 1점으로 계산함</param>
int Hands_GetScore(int aceIsOne);

/// <summary>
/// 버스트 판정을 피할 수 없는지 확인함
/// </summary>
int Hands_isBursted();

/// <summary>
/// 현재 손패로 받을 최소 데미지를 계산함
/// </summary>
/// <param name="NeverBurst">0이 아니라면, 피해를 감수해서라도 버스트를 무조건 피한다는 가정하에 피해를 계산합니다</param>
int Hands_GetDMG();

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

void SpriteObj_Print_wScale(SpriteObj* sprite, int _scale);

SpriteObj* SpriteObj_Instantiate(const bitResource* resource, int index, int x, int y);

/// <summary>
/// SpriteRender Layer에 사용할 수 있는 SpriteObj들로 구성된 글자를 생성합니다
/// </summary>
/// <param name="lineCount">[out] 이 글이 몇 줄인지 반환함</param>
/// /// <param name="_maxcharLength">[out] 이 글의 최대 가로 길이를 픽셀 단위로 반환함</param>
SpriteObj* SpriteObj_CreateText(char* _content, int x, int y, int* _lineCount, int* _maxlineLength, int color);

/// <summary>
/// SpriteObj로 만들어진 텍스트를 효율적으로 수정합니다
/// ! SpTextObj의 경우 전용 수정 함수를 사용하시오
/// </summary>
/// <param name="lineCount">[out] 이 글이 몇 줄인지 반환함</param>
/// /// <param name="_maxcharLength">[out] 이 글의 최대 가로 길이를 픽셀 단위로 반환함</param>
void SpriteObj_EditText(SpriteObj* spText, char* _content, int* _lineCount, int* _maxlineLength, int color);

/// <summary>
/// 고유한 렌더링 레이어에서 출력되는 SpriteObj들로 구성된 글자를 생성합니다
/// </summary>
/// <param name="_Scale">텍스트 크기</param>
SpTextObj* SpTextObj_Create(char* _content, int _Scale, int x, int y, SpTextAlign _align, int color);

void SpTextObj_Edit(SpTextObj* spText, char* _new_content);

void SpTextObj_Print(SpTextObj* spText);

void SpTextObj_Free(SpTextObj* spText);

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