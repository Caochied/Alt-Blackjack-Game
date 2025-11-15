#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <time.h>

#include "GameSys.h"
#include "Grouplist.h"
#include "resource.h"

#pragma comment(lib, "Msimg32.lib") //TransparentBlt 사용

/*
 ### 객체 생성 기준 ###
 - 값을 전체 복사해서 관리하지 않는다
 - 함수에서 생성했는데 외부에서도 지속적으로 사용하고 싶다
 - 전역 변수같이 하드 코딩으로 만들어진 저장공간에 계속 존재하지 않는다. 즉, 생성과 파괴가 잦다
		-> 리스트에 보관되면 객체임
*/

// ! 키 입력 체계 테스트 함수
void MoveStar(int event, int direc) {
	static SpriteObj* star = NULL;
	if (star == NULL) {
		star = SpriteObj_Instantiate(&Star_crumb, 0, 8, 8); // ! 렌더링 큐에 포함되면, 프로그램 종료시, 자동으로 메모리 해제됨
		//애니메이터 생성해서 추가해주기

		Group_Add(RenderList[1], star, SpriteObj_class);
	}

	if (event != 1) return; //KeyDown 에만 반응

	star->sprite_index = (star->sprite_index + 1) % 2;

	if (direc == 1) //Right
	{
		star->x += 10;
	}
	else if (direc == -1) //Left
	{
		star->x += -10;
	}
}

int main() {
	Load_bitResource();

	system("title BlackJack Game"); //콘솔창 정보 수정
	system("mode con:cols=120 lines=32"); //960,512 offset(+8,+32)

	CONSOLE_CURSOR_INFO cursor = { 0, }; //커서 가리기
	cursor.dwSize = 100;
	cursor.bVisible = FALSE;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor);

	srand(time(NULL)); //시드 초기화

	for (int i = 0; i < 4; i++) { //구조체 초기화 작업
		CardGameObj_Initialize(&Hands[i], 0, 42 + i * (Card_BG.width + 4), 91);
	}

	Deck = Group_Create(Card_class);

	//덱 세팅
	for (int i = 1; i <= 13; i++) { //스페이드 13개 카드 덱에 세팅
		Card* card = Card_Instantiate(Spade, i);
		Group_Add(Deck, card, Card_class);
	}
	for (int i = 1; i <= 13; i++) { //하트 13개 카드 덱에 세팅
		Card* card = Card_Instantiate(Heart, i);
		Group_Add(Deck, card, Card_class);
	}

	//카드 뽑기
	for (int i = 0; i < 2; i++) {
		//카드 한장 뽑아 손패에 저장하고, 리스트 제거

		if (i == 0) {
			Hands[i].card = (Card*)Group_ExcludeByIndex(Deck, 12);
		}
		else Hands[i].card = (Card*)Group_ExcludeByIndex(Deck, rand() % Deck->count);
	}

	// TODO 테스트 코드 실행 제거
	Card_StackUp(Hands[0].card, (Card*)Group_ExcludeByIndex(Deck, rand() % Deck->count));
	SelectHands(-1, 0); //별 스프라이트 선언 및 생성을 위한 첫 실행
	
	StateGo_Idle(); //플레이어 조작 가능

	clock_t last_time, temp;
	double deltaTime = 0; //실제 코드의 elasped clock
	double frameTime = 0; double frameRate = (1.0 / 120.0) * CLOCKS_PER_SEC;
	int animTime = 0; int animRate = 5; //스프라이트 교체에 대한 연산은 - 24프레임

	while (1) {
		last_time = clock();

		if (frameTime > frameRate) { // ! 프레임별 연산
			if (animTime >= animRate) {
				//star->sprite_index = (star->sprite_index + 1) % 2;
				// TODO 여기에 스프라이트를 바꾸는 애니메이터 연산
				animTime = 0;
			}
			else animTime++;

			//그래픽 루프 전, 게임오브젝트 업데이트
			for (int i = 0; i < 4; i++) {
				CardGameObj_Update(&Hands[i]);
			}

			RECT rc = { 0, 0, 960, 512};
			FillRect(renderDC, &rc, GameBG);

			SpriteObj* listLoop;
			for (int i = 0; i < RENDERING_LAYERSIZE; i++) {
				listLoop = (SpriteObj*)RenderList[i]->front;
				while (listLoop != NULL) {
					SpriteObj_Print(listLoop);
					listLoop = (SpriteObj*)listLoop->groupProp.next;
				}
			}

			//버퍼에 담긴 결과 출력
			BitBlt(hdc, offsetX, offsetY, 960, 512, renderDC, 0, 0, SRCCOPY);

			// ! 키입력 받기
			Input_KeyPress();

			frameTime = 0; // ! 프레임 초기화
			//printf("\b\b\b\b\b\b\b\b\b\b\b\bfps : %.2lf", CLOCKS_PER_SEC / deltaTime);
		}

		temp = clock();
		deltaTime = (double)(temp - last_time);
		frameTime += deltaTime;
	}


	Group_FreeAll(Deck);

	//리스트, 카드 등 객체들 알아서 해제해야 한다
	Unload_bitResource();
	return 0;
}