import enum
import random

'''
class Suit(enum):
    Spade = 1
    Heart = 2
    Diamond = 3
    Club = 4
'''     

class Card:
    __value = 0
    __suit = 0
    
    __stackedCard = None #이 카드 바로 위에 스택된 카드를 가리킴

    def __init__(self, value, suit):
        self.__value = value
        self.__suit = suit

    def getValue(self):
        score = int(self.__value)
        if score > 10:
            score = 10
        return score
    def getValueAsMark(self):
        '''str형태로 1, 11, 12, 13를 각각 A, J, Q, K로 표현합니다'''
        signMark = ['A','2','3','4','5',
                    '6','7','8','9','10',
                    'J','Q','K']
        return signMark[self.__value-1]
    def getSuit(self):
        mark = ['Spade','Heart','Diamd','Clubb']
        return mark[self.__suit-1]
    def getStackedCard(self):
        return self.__stackedCard
    def getUpperStackedCard(self):
        '''이 카드부터 위에 있는 스택들에서 가장 위에 놓여진 카드를 반환함'''
        if self.__stackedCard == None:
            return self
        else:
            return self.__stackedCard.getUpperStackedCard()
    
    def stackUp(self, stackCard):
        '''이 카드 위에 다른 카드를 쌓는다
        - 가능여부 반환
        - 이 pile들 위에 쌓고자 한다면, upperstackedcard를 호출하고 사용'''
        if self.__stackedCard != None:
            return False
        #아직 색깔을 번갈아 가며 쌓는 규칙은 제외, 오직 내림차순으로 쌓는 규칙만
        if stackCard.getValue() >= self.__value:
            return False
        self.__stackedCard = stackCard
        return True

    def stackDown(self):
        '''이 카드 위에 쌓인 카드 스택을 전부 내린다'''
        temp = self.__stackedCard
        self.__stackedCard = None
        return temp
    def Cardinfo(self):
        return "{1}-{0}".format(self.getSuit(),self.getValueAsMark())

class Player:
    HP = 0
    holds = []
    maxHold = 4
    def __init__(self, hp, maxHold):
        self.holds = []
        self.HP = hp
        self.maxHold = maxHold
    
    def takeCard(self, card):
        '''카드를 손패에 넣음
        - 가능여부 반환'''
        if card == None:
            return False
        if len(self.holds) >= self.maxHold:
            return False
        self.holds.append(card)
        return True
    
    def stackupNewCard(self, HoldIndex, card):
        '''HoldIndex 번째의 카드에 새 카드를 스택한다
        - 가능여부 반환'''
        return self.holds[HoldIndex].stackUp(card)

    def stackupHoldCard(self, destIndex, cardIndex):
        '''손패의 destIndex 번째의 카드에 cardIndex 번째의 카드를 올린다
        - 가능여부 반환'''
        
        if self.holds[destIndex].getUpperStackedCard().stackUp(self.holds[cardIndex]):
            del(self.holds[cardIndex])
            return True
        else:
            return False
    #사용하지 않음
    def stackdown(self, pile, destIndex):
        '''특정 스택된 카드, 또는 카드 더미를 hold의 destIndex번째 카드 위에 내려봄
        - destIndex가 올바른 수가 아니라면, 빈 hold에 내려놓음
        - 가능여부 반환
        - 사용하지 않음'''
        if destIndex >= len(self.holds):
            if destIndex < self.maxHold:
                self.holds.append(pile)
                return True
            else:
                return False
        return self.holds[destIndex].getUpperStackedCard().stackUp(pile)
    def getScore(self, aceIsOne = True):
        '''현재 손패의 점수를 계산함
        - aceIsOne : 에이스를 1로 계산함'''
        score = 0
        for i in self.holds:
            temp = i.getUpperStackedCard().getValue()
            if temp == 1 and not aceIsOne:
                temp = 11
            score += temp
        return score
    def discardHolds(self):
        '''자신의 holds의 카드 스택을 모두 단일 카드들로 풀어, 리스트로 반환함'''
        cards = []
        for i in self.holds:
            card_temp : Card = i
            while card_temp != None:
                cards.append(card_temp)
                card_temp = card_temp.getStackedCard()
        self.holds = []
        return cards
    
    def printHoldStat(self):
        print("\n==========================")
        statLine = []
        for i in self.holds:
            card_temp : Card = i
            pile = []
            while card_temp != None:
                pile.append(card_temp.Cardinfo())
                card_temp = card_temp.getStackedCard()
            statLine.append(pile)
            pile.reverse()
            print(pile)
        
        for i in range(self.maxHold - len(self.holds)):
            print("['슬롯 {0}']".format(len(self.holds) + i+1))
        
        print("==========================")
        #print(statLine)
        
    def cal_currentDMG(self):
        '''플레이어 손패로 예상되는 체력 피해를 계산함'''
        i = self.getScore(aceIsOne=False)
        if i > 21:
            i = self.getScore()
        dmg = abs(21-i)
        if i > 21:
            dmg += 5
        return dmg
        
#한 게임의 상황을 담음
class OneGame:
    LeftTurn = 0
    decks = []
    discarded = []
    
    Player = None
    TotalScore = 0
    UsedFLEE = False
    
    def __init__(self):
        self.decks = []
        self.discarded = []
        self.Player = Player(10, 4)
        self.TotalScore = 0
        self.Set26()
        self.GameLoop()
        
    def GameLoop(self):
        print("게임 루프 시작")
        
        #첫 시작 드로우 - 버퍼 없음 주의
        self.Player.takeCard(self.drawRandCard())
        self.Player.takeCard(self.drawRandCard())
        
        while self.Player.HP > 0 and len(self.decks) > 0:
            TurnEnd = False
            while not TurnEnd:
                self.Player.printHoldStat()
                TurnEnd = self.getOption_Main()
            
            #주어진 덱에서 최대 점수를 얻기 위해 다시 덱을 채우지 않음
            self.Player.discardHolds()
            
                        #드로우, 한계치 넘기는거 주의 - 버퍼 없음
            self.Player.takeCard(self.drawRandCard())
            self.Player.takeCard(self.drawRandCard())
        
        print("\n\n=======================")
        print("Game Over\nTotal : ",self.TotalScore)
        if self.Player.HP > 0:
            print("살아남았다!")  
        return 0
    
    def Set52(self):
        self.decks = []
        for suit in range(1,5):
            for value in range(1,14):
                self.decks.append(Card(value, suit))
    def Set26(self):
        '''덱 카운팅을 위한 2수트 덱'''
        print("\n // 이번 게임은 2suit - 26장만을 섞습니다 //\n")
        self.decks = []
        for suit in range(1,3):
            for value in range(1,14):
                self.decks.append(Card(value, suit))
    
    def drawRandCard(self):
        '''덱에서 카드를 한 장 드로우 합니다
        - 반드시 뽑은 카드는 버퍼에 저장하고
        유저가 카드를 수용할 수 있는지 확인해야 합니다
        - 뽑은 카드를 반환하는 것을 잊지마세요; 카드가 덱 순환에서 완전히 소실됩니다'''
        if len(self.decks) <= 0:
            if self.Shuffle():
                return self.drawRandCard()
            else:
                print("카드가 충분하지 않음")
                return None
        rdindex = random.randint(0, len(self.decks)-1)
        card = self.decks[rdindex]
        del(self.decks[rdindex])
        return card
    
    def returnDrawCard(self, card):
        '''덱에서 카드는 뽑았지만, 가져갈 수 없으면 반납함'''
        self.decks.append(card)
        
    def discardCard(self, card):
        self.discarded.append(card)
    
    #버려진 카드를 다시 덱에 넣음
    def Shuffle(self):
        if len(self.discarded) <= 0:
            return False
        self.decks += self.discarded
        self.discarded = []
        return True
    
    def warningLaststand(self):
        '''게임 막바지에 남은 카드들을 경고합니다
        - 5장 이하로 남은 경우 카드 리스트를 보임'''
        if len(self.decks) > 5:
            return
        print("\t!!!덱이 얼마 안 남았습니다!!!")
        lists = []
        for i in self.decks:
            lists.append(i.Cardinfo())
        print("\t남은 카드들",lists)
    
    def getOption_Main(self):
        '''플레이어의 메인 옵션
        - Player가 Stand, 턴을 종료했는지 반환'''
        option = 0
        print("\tHP {2}\tcurrent Score {0}-{1}\tincomming DMG {3}".format(self.Player.getScore(),self.Player.getScore(aceIsOne=False), self.Player.HP, self.Player.cal_currentDMG()))
        print("\tLeft Deck count :",len(self.decks))
        self.warningLaststand()
        option = int(input("[1.STAND] [2.DRAW-] [3.STACK] [4.FLEE-]\n :: ")) #[4.SPLIT]
        if option < 1 or option > 4:
            print("\n\n잘못된 입력")
            return False
        
        if option == 1:
            i = self.Player.getScore(aceIsOne=False)
            if i > 21:
                i = self.Player.getScore()
            
            if i <= 21:
                self.TotalScore += i
                print("\n\n+{0} TOTAL : {1}".format(i, self.TotalScore))
            else:
                print("\n\nBURSTED... TOTAL :",self.TotalScore)
            
            dmg = self.Player.cal_currentDMG()
            if dmg > 0:
                print("hurts! -",dmg)
                self.Player.HP -= dmg
            else:
                print("perfect!")
            return True
        elif option == 2:
            drawed = self.drawRandCard()
            if self.Player.takeCard(drawed):
                return False
            else:
                self.returnDrawCard(drawed)
                print("\n\n카드를 받을 슬롯이 없습니다")
                return False
        elif option == 3:
            self.getOption_Stack()
            return False
        elif option == 4:
            if self.UsedFLEE:
                print("이미 한번 도망갔는걸... (한번만 사용 가능)")
                return False
            print("\n\n손패를 전부 덱에 다시 넣었다...")
            for i in self.Player.discardHolds():
                self.returnDrawCard(i)
            return True
        else:
            return False
    
    def getOption_Stack(self):
        if len(self.Player.holds) <= 1:
            print("\n\n스택할 카드가 없음")
            return
        print("\n슬롯에 있는 카드 전부 집어 다른 카드 위로 올립니다 (aprt slot, dest slot)")
        aprt = int(input("from slot, 옮길 슬롯 번호 : "))
        dest = int(input("to slot, 올릴 카드 위치 : "))
        
        if self.Player.stackupHoldCard(dest-1, aprt-1):
            return
        else:
            print("\n잘못된 스택")
        

def main():
    needsTuto = True
    if needsTuto:
        print(" // 게임 설명 //\n")
        print("\t블랙잭/러미 기반의 카드 게임")
        print("\t목표 :: 체력이 0이 되지 않고 덱을 비우기\n")
        print("- STAND할 때, 손패 점수와 21점의 편차만큼 체력을 잃음")
        print("  손패가 21점 초과되면 추가 5 데미지")
        print("")
        print("- 'FLEE-' 행동으로 단 한번만, 손패 전부를 다시 덱에 넣고 다시 뽑을 수 있음")
        print("- 쌓은 카드 더미는 윗 카드만 점수로 인정됨")
        print("  (카드 쌓기는 K -> Q -> J.. 같이 내림차순으로만 쌓습니다, A는 1, 11점 이지만, 2카드 위에만 올릴 수 있습니다)")
        print("- 덱에 5장 남으면, 편의를 위해 남은 카드의 종류를 전부 드러냄 (마지막에 부족한 카드로 인한 데미지 주의)")
        input("\n\nEnter해서 시작하세요")
    MainGame = OneGame()
    

if __name__ == '__main__':
    random.seed()
    main()
    
# TO-DO
# 언더컷을 정해준다 -> 냅다 콜만 하면 점수가 더 높음