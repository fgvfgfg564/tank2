#include<iostream>
#include<cstdio>
#include<algorithm>
#include<cmath>

using namespace std;

const int INF=0x3f3f3f3f;
template <typename T>inline void updmax(T &a,T b){if(b>a)a=b;}
template <typename T>inline void updmin(T &a,T b){if(b<a)a=b;}

#define BOARD_SIZE 10

int myRand(int l,int r){
	return rand()%(r-l+1)+l;
}

class Board{
	int a[BOARD_SIZE][BOARD_SIZE];
public:
	Board(){
		for(int i=0;i<BOARD_SIZE;i++){
			for(int j=0;j<BOARD_SIZE;j++){
				a[i][j]=myRand(-10,10);
			}
		}
	}
	int* operator[](int k){return a[k];}
	void change_side(){
		for(int i=0;i<BOARD_SIZE;i++){
			for(int j=0;j<=i;j++){
				swap(a[i][j],a[j][i]);
				a[i][j]=-a[i][j];
				if(i!=j)a[j][i]=-a[j][i];
			}
		}
	}
};

//side=0: 选行，越大越好；
//side=1: 选列，越小越好

struct AI{
	virtual int decide(Board &board, int side){return 0;}
};

struct RandomAI:AI{
	int decide(Board &board, int side){
		return myRand(0,BOARD_SIZE - 1);
	}
};

struct SaddlePointAI:AI{
	int decide(Board &board, int side){
		int minn[BOARD_SIZE],maxx[BOARD_SIZE];
		for(int i=0;i<BOARD_SIZE;i++){
			minn[i]=INF;
			maxx[i]=-INF;
		}
		for(int i=0;i<BOARD_SIZE;i++){
			for(int j=0;j<BOARD_SIZE;j++){
				updmin(minn[i],board[i][j]);
				updmax(maxx[j],board[i][j]);
			}
		}
		if(side==0){
			int ans=myRand(0,BOARD_SIZE - 1);
			int val=-INF;
			for(int i=0;i<BOARD_SIZE;i++){
				for(int j=0;j<BOARD_SIZE;j++){
					if(board[i][j]==minn[i] && board[i][j]==maxx[j] && board[i][j]>val){
						val=board[i][j];
						ans=i;
					}
				}
			}
			return ans;
		}
		else{
			int ans=myRand(0,BOARD_SIZE - 1);
			int val=INF;
			for(int i=0;i<BOARD_SIZE;i++){
				for(int j=0;j<BOARD_SIZE;j++){
					if(board[i][j]==minn[i] && board[i][j]==maxx[j] && board[i][j]<val){
						val=board[i][j];
						ans=j;
					}
				}
			}
			return ans;
		}
	}
};

struct MinLossAI:AI{
	int decide(Board &board, int side){
		if(side)board.change_side();
		int minn, res, val=-INF;
		for(int i=0;i<BOARD_SIZE;i++){
			minn = INF;
			for(int j=0;j<BOARD_SIZE;j++){
				updmin(minn,board[i][j]);
			}
			if(minn>val){
				val=minn;
				res=i;
			}
		}
		if(side)board.change_side();
		return res;
	}
};

struct ClevererMinLossAI:AI{
	int decide(Board &board, int side){
		if(side)board.change_side();
		int minn, res, val=-INF;
		for(int i=0;i<BOARD_SIZE;i++){
			minn = INF;
			for(int j=0;j<BOARD_SIZE;j++){
				updmin(minn,board[i][j]);
			}
			if(minn>val){
				val=minn;
				res=i;
			}
			else if(minn==val){

			}
		}
		if(side)board.change_side();
		return res;
	}
};

int compete(AI *a, AI *b){
	int res=0;
	for(int i=1;i<=100;i++){
		Board board;
		int a_action = a->decide(board, 0);
		int b_action = b->decide(board, 1);
		res+=board[a_action][b_action];
	}
	return res;
}

int main()
{
	srand(time(NULL));
	AI *a = new MinLossAI(), *b = new SaddlePointAI();
	for(int i=1;i<=10;i++){
		cout<<compete(a, b)<<endl;
	}
	return 0;
}