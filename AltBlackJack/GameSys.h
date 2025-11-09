#pragma once
#include "Grouplist.h"

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