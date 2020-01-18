/*
 * PROJECT.c
 *
 *  Created on: 14 Oct 2019
 *      Author: Daniel Eggleton läp
 */
/*
===============================================================================
 Name        : Exercise01.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/


/*Found issues, Solutions not ideal, but work
 * Player can enter multiple arguments to line etc, hit:2,a5,a6,a (will run 3 loops, without new input). No quick solution to clear terminal, solutions would lead to other limitations, time limitation will not implement
 * Formatting errors sometimes runs extra loop to find error. No solution as doesn't effect game notably
 * Csys alloc fails when trying to fread over, about 1720 chars. Unknown reason malloc not null and malloc size correct. Solution shorter game log entry and split players to own file. Tested to handle at least 90 shot round, fails 95, String could be shortened to handle full (5 placement, 100 shot, 1 win) 106 row game, but for purpose of clarity will be left at current lenght
 * fseeks&rewind didn't allow reading file that was fseeked to end. FEOF flag set off by fseek (should activate only after reading past end?)? Solution (reopen fresh file pointer)
 * printf failed after 8 characters. First printf works,after that it would fail about 100% when running, debugging slowly would allow it to work often. solution (format string with sprintf and print with fputs combination)
 *
 */

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif
#include <string.h>
#include <cr_section_macros.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
typedef enum grid{A,B,C,D,E,F,G,H,I,J}grid;
typedef enum direc{down,right}direc;
#define gridSize 10
#define submarLen 3
#define patrolLen 2
#define battlesLen 4
#define carrierLen 5
#define destrLen 3
#define asciiMove 65
#define fileLen 10
#define maxLine 100

typedef struct battleshipsSquare{
	bool hit;
	bool ship;
}BATTLE;


void printHits(BATTLE bs[gridSize][gridSize]);
bool setHit(BATTLE *bs);
bool placeShip(BATTLE bs[gridSize][gridSize], int lenght,int player,const char filename[fileLen]);
void startGame(BATTLE bs[gridSize][gridSize],int player,const char filename[fileLen]);
bool gameEnd(BATTLE bs[gridSize][gridSize]);
void fireHit(BATTLE bs[gridSize][gridSize],const char filename[fileLen],int player);
void writeLogFile(const char filename[fileLen], char* text);
void printLog(const char filename[fileLen]);
void clearLog(const char filename[fileLen]);

int main(void) {
#if defined (__USE_LPCOPEN)
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
    // Set up and initialize all required blocks and
    // functions related to the board hardware
    Board_Init();
    // Set the LED to the state of "On"
    Board_LED_Set(0, true);
#endif
#endif

    //rules split if time permits to implement printing rules mid game
    const char *ruleStart={"Starting from player 1, enter the 5 ships on to the board.\nTo place ship enter anchor point, then enter either \"right\" or \"up\"\nValid entry: 1,A,right invalid entry: 10,A,right (cannot fill right past 10)  10,C,left (left not valid direction)\nShips must not be placed on top of each other, value range 1-10 and a-j\n" };
    const char *rulePlay={"Players take turns entering locations to hit\nYou can hit only squares that haven't been hit\nExample 1,b hits 1b square, values 1-10, A-J\nEnter 0,? for list of current hits on enemy\n"};
    char ui[10] = {};
    const char logFile1[fileLen]={"log1.dat"};
    const char logFile2[fileLen]={"log2.dat"};
    printf("MECHANICS OF TWO PLAYER BATTLESHIPS\n%s\n%s\nEnter 1 to start game, 2 to print last log:",ruleStart,rulePlay);
    scanf("%10s",ui);


#if 1
    if(ui[0]=='1'){
    	clearLog(logFile1);
    	clearLog(logFile2);
    	BATTLE bsP1[gridSize][gridSize]={false,false};
    	BATTLE bsP2[gridSize][gridSize]={false,false};
		startGame(bsP1,1,logFile1);
		startGame(bsP2,2,logFile2);
    	do{
    		printf("Player 1 set attack coordinate ");
    		fireHit(bsP2,logFile1,1);
    		printf("Player 2 set attack coordinate ");
    		fireHit(bsP1,logFile2,2);

    	}while(!gameEnd(bsP1)&&!gameEnd(bsP2));
    	//Game finished at this point
    	if(!gameEnd(bsP1)&&gameEnd(bsP2)){
    		fputs("Player 1 wins!\n",stdout);
    		writeLogFile(logFile1,"Player 1 wins!\n");
    		writeLogFile(logFile2,"Player 1 wins!\n");
    	}else if(gameEnd(bsP1)&&!gameEnd(bsP2)){
    		fputs("Player 2 wins!\n",stdout);
    		writeLogFile(logFile1,"Player 2 wins!\n");
    		writeLogFile(logFile2,"Player 2 wins!\n");
    	}else{
    		fputs("It's a draw\n",stdout);
    		writeLogFile(logFile1,"It's a draw\n");
    		writeLogFile(logFile2,"It's a draw\n");
    	}
    	//Prints state of gameboard at end of game, not equal to gamelog(not in correct order of play)
    	fputs("All shots on player 1\n",stdout);
    	printHits(bsP1);
    	fputs("All shots on player 2\n",stdout);
    	printHits(bsP2);
    }else if(ui[0]=='2'){
    	//ui command to print
    	fputs("Player 1 log \n",stdout);
    	printLog(logFile1);
    	fputs("Player 2 log \n",stdout);
    	printLog(logFile2);
    }else{
    	//first ui command not correct
    	printf("Enter a correct command\n");
    	while(getchar()!='\n');
    }
#endif

}



void printHits(BATTLE bs[gridSize][gridSize]){
	//Check for all squares, if hit and if it's hit was there a ship. Prints this info out
	for(int i=0;i<gridSize;i++){
		for(int b=0;b<gridSize;b++){
			char c=b+asciiMove;
			if(bs[i][b].hit==true && bs[i][b].ship==false){
				printf("[%d][%c] Shot missed\n",i+1, c);
			}else if(bs[i][b].hit==true && bs[i][b].ship==true){
				printf("[%d][%c] Shot hit\n",i+1, c);
			}
		}
	}
}

bool setHit(BATTLE *bs){
	//Sets a a square to hit, only can hit if not already hit
	if(bs->hit==false){
		bs->hit=true;
		if(bs->ship==true){
			printf("Ship hit\n");
		}else{
			printf("No hit\n");
		}
		return true;
	}
	printf("location hit already\n");
	return false;
}

bool placeShip(BATTLE bs[gridSize][gridSize], int lenght, int player,const char filename[fileLen]){
	int anchorRow=0, anchorCol=0,direction=-1;
	char tempChar='\0';
	char dir[20]={};
	//Takes location defined by player a converts to better format for code
	do{
		printf("enter: ");
		int i=scanf("%d,%c,%s",&anchorRow,&tempChar,dir);
		if(i!=3){printf("Format fail ");while(getchar()!='\n'); continue;}
	}while(strcmp(dir,"down")!=0&&strcmp(dir,"right")!=0);
	if(strcmp(dir,"right")==0){
		direction=right;
	}else if(strcmp(dir,"down")==0){
		direction=down;
	}
	anchorCol=toupper(tempChar)-asciiMove+1;

	//anchor point bound check
	if(anchorCol<=0||anchorCol>=11||anchorRow<=0||anchorRow>=11){
		printf("Error in anchor points, 1-10, A-J, right or down avoid white spaces\n");
		return false;
	}

	bool clearSlot=true;
	//checking so that ship will fit it bounds. Also routes function to either a vertical or horizontal ship placement code
	//Vertical placement
	if(direction==down && anchorRow+lenght<=10 && anchorRow>=1){
		//Snippet checks that placement area is clear before placing ship
		for(int i=anchorRow-1;i<lenght+anchorRow-1;i++){
			if(bs[i][anchorCol-1].ship==true){
				clearSlot = false;
			}
		}
		//Snippet places ship to defined location
		if(clearSlot==true){
			for(int i=anchorRow-1;i<lenght+anchorRow-1;i++){
				if(bs[i][anchorCol-1].ship==false){
					bs[i][anchorCol-1].ship=true;
				}
			}
			printf("Placement succeeded\n");
			char arr[100]={};
			sprintf(arr,"Player %d Places ship %2d,%c - %2d,%c\n",player,anchorRow,toupper(tempChar),anchorRow+lenght,toupper(tempChar));
			writeLogFile(filename,arr);
			return true;
		}
	}//horizontal placement
	else if(direction==right&& anchorCol+lenght<=10 && anchorCol>=1){
		for(int i=anchorCol-1;i<lenght+anchorCol-1;i++){
			if(bs[anchorRow-1][i].ship==true){
				clearSlot = false;
			}
		}
		if(clearSlot==true){
			for(int i=anchorCol-1;i<lenght+anchorCol-1;i++){
				if(bs[anchorRow-1][i].ship==false){
					bs[anchorRow-1][i].ship=true;
				}
			}
			printf("Placement succeeded\n");
			char arr[100]={};
			sprintf(arr,"%d Places ship %2d,%c - %2d,%c\n",player,anchorRow,toupper(tempChar),anchorRow,toupper(tempChar+lenght));
			writeLogFile(filename,arr);
			return true;

		}
	}
	printf("Placement failed, out of bound or placed on other ship\n");
	return false;

}
void startGame(BATTLE bs[gridSize][gridSize], int player,const char filename[fileLen]){
	//Sequence to place 5 available ships to defined array
	char array[maxLine]={};
	sprintf(array,"Player %d Enter Patrol Boat anchor point, length %d ",player,patrolLen);
	fputs(array,stdout);
	while(!placeShip(bs, patrolLen,player,filename));
	sprintf(array,"Player %d Enter Submarine anchor point, length %d ",player,submarLen);
	fputs(array,stdout);
	while(!placeShip(bs, submarLen,player,filename));
	sprintf(array,"Player %d Enter Destroyer anchor point, length %d ",player,destrLen);
	fputs(array,stdout);
	while(!placeShip(bs, destrLen,player,filename));
	sprintf(array,"Player %d Enter Battleship anchor point, length %d ",player,battlesLen);
	fputs(array,stdout);
	while(!placeShip(bs, battlesLen,player,filename));
	sprintf(array,"Player %d Enter Carrier anchor point, length %d ",player,carrierLen);
	fputs(array,stdout);
	while(!placeShip(bs, carrierLen,player,filename));
	fputs("\n\n\n\n\n\n\n",stdout);
	/*
	CODE Not working for unknown reason. Printf fails to print normal defined text after 8 characters, Hardware limitation?
	printf("Player %d enter Patrol boat anchor point, length %d\n",player, patrolLen);
	while(!placeShip(bs, patrolLen));
	printf("Player %d enter submarine anchor point, length %d\n",player,submarLen);
	while(!placeShip(bs, submarLen));
	printf("Player %d enter destroyer anchor point, length %d\n",player,destrLen);
	while(!placeShip(bs, destrLen));
	printf("Player %d enter battleship anchor point, length %d\n",player,battlesLen);
	while(!placeShip(bs, battlesLen));
	printf("Player %d enter carrier anchor point, length %d\n",player,carrierLen);
	while(!placeShip(bs, carrierLen));
	*/
}
bool gameEnd(BATTLE bs[gridSize][gridSize]){
	//check if there are any slots with ship, without hit
	bool check=true;
	for(int i=0;i<gridSize;i++){
		for(int o=0;o<gridSize;o++){
			if(bs[i][o].ship==true && bs[i][o].hit==false){
				check = false;
			}
		}
	}
	return check;
}
void fireHit(BATTLE bs[gridSize][gridSize],const char filename[fileLen],int player){
	//For setting up for setHit function. Makes bound checks and has alt function to print hits on enemy.
	int row=0, col=0;
	char colTemp='\0';
	bool comp = false;
	while(!comp){
		fflush(stdin);
		printf("enter: ");
		if(scanf("%d,%c",&row,&colTemp)==2){
			col = toupper(colTemp)-asciiMove;
			row--;
			if(colTemp=='?'){
				printHits(bs);
			}else if(row<=9&&row>=0&&col<=9&&col>=0){
				if(!setHit(&bs[row][col])){
					continue;
					//If setHit returns false (already hit) goes to next loop
				}
				comp=true;
				char log[100]={},hitLog[20];
				bool hit = bs[row][col].ship;
				if(hit){
					strcpy(hitLog,"hit");
				}else{
					strcpy(hitLog,"miss");
				}
				sprintf(log,"%d shot %2d,%c %s\n",player,row+1,toupper(colTemp),hitLog);
				writeLogFile(filename,log);
			}else{
				printf("Format/bound fail ");
			}
		}else{
			printf("Format fail \n");
		}
	}
}
void writeLogFile(const char filename[fileLen], char* text){
	long size=0;
	char * tempText;
	FILE * file= fopen(filename,"rb");
	if(file!=NULL){
		fseek(file,0,SEEK_END);
		size=ftell(file);
		fclose(file);
		FILE * file= fopen(filename,"rb");
		tempText=(char*) malloc(size+strlen(text));
		if(tempText==NULL){
			printf("malloc fail");
		}else{
			fread(tempText,1,size,file);
			tempText[size]='\0';
			strcat(tempText,text);
		}
		fclose(file);
	}else{
		tempText=(char*) malloc(strlen(text)*sizeof(char));
		strcpy(tempText,text);
	}
	FILE *fileout= fopen(filename,"wb");
	fwrite(tempText,sizeof(char),size+strlen(text),fileout);

	fclose(fileout);
	free(tempText);

}
void printLog(const char filename[fileLen]){
	long size=0;
	char * tempText;
	FILE * file= fopen(filename,"rb");
	if(file!=NULL){
		fseek(file,0,SEEK_END);
		size=ftell(file);
		fclose(file);
		FILE * file= fopen(filename,"rb");
		tempText=(char*) malloc(size);
		if(tempText==NULL){
			printf("malloc fail");
		}else{
			fread(tempText,1,size,file);
			fputs(tempText,stdout);
		}
		free(tempText);
	}
	fclose(file);


}
void clearLog(const char filename[fileLen]){
	FILE * file= fopen(filename,"wb");
	fclose(file);
}





