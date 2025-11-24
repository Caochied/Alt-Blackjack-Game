#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <time.h>

#include "GameSys.h"
#include "Grouplist.h"
#include "gameScheduler.h"
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
		star = SpriteObj_Instantiate(&rStar_crumb, 0, 8, 8); // ! 렌더링 큐에 포함되면, 프로그램 종료시, 자동으로 메모리 해제됨
		//애니메이터 생성해서 추가해주기

		Group_Add(RenderList_Sprite[1], star, SpriteObj_class);
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
	system("mode con:cols=120 lines=32"); //960,512 offset(+8,+32) [240, 128]

	CONSOLE_CURSOR_INFO cursor = { 0, }; //커서 가리기
	cursor.dwSize = 100;
	cursor.bVisible = FALSE;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor);

	srand(time(NULL)); //시드 초기화

	// TODO 테스트 코드 실행 제거
	Group_Add(RenderList_Text, SpTextObj_Create("Alt BlackJack week 4", 2, 120, 9, SpText_LowerMiddle, 0), SpTextObj_class);
	Group_Add(RenderList_Text, SpTextObj_Create("Z - Confirm / Stackup\nX - Cancel\nUp - DrawCards / Down - TurnEnd\nLeft/Right - Select Hands"
		, 2, 6, 6, SpText_UpperLeft, 0), SpTextObj_class);
	
	Start_InGame();
	Game_MainLoop();

	//리스트, 카드 등 객체들 알아서 해제해야 한다
	Free_AnimStream();
	Free_InGameUI();
	Group_FreeAll(Deck);
	//렌더링과 리소스 객체 해제
	Unload_bitResource();
	return 0;
}