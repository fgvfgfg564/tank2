#include <algorithm>
#include <cassert>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <deque>
#include <queue>
#include <stack>
#include <bitset>
#include <fstream>

using namespace std;

#define INF 0x3f3f3f3f
#define INFLL 100000000000000000ll
#define gc getchar()
#define abs(x) ((x)>0?(x):(-(x)))
#define X first
#define Y second
#define mp(x,y) make_pair((x),(y))
#define eps (1e-8)
#define pb push_back
#define pf(x) ((x)*(x))
#define il inline

const double pi=3.1415926535897932384626;

typedef long long ll;
typedef unsigned int uint;
typedef unsigned long long ull;
typedef long double ld;
typedef pair<int,int> pii;
typedef pair<double,double> pdd;
typedef pair<ll,ll> pll;

il ll read()
{
	char c=gc;ll u=0,f=1;
	while(c!='-' && (c<'0' || '9'<c))c=gc;
	if(c=='-'){
		f=-1;
		c=gc;
	}
	while('0'<=c && c<='9'){
		u=u*10+c-'0';
		c=gc;
	}
	return u*f;
}
il char readc()
{
	for(;;){
		char c=gc;
		if(('A'<=c && c<='Z') || ('a'<=c && c<='z'))return c;
	}
}
il int read_digit()
{
	for(;;){
		char c=gc;
		if('0'<=c && c<='9')return c-'0';
	}
}
inline int max(int a,int b){return a>b?a:b;}
inline int min(int a,int b){return a<b?a:b;}
template <typename T>inline void updmax(T &a,T b){if(b>a)a=b;}
template <typename T>inline void updmin(T &a,T b){if(b<a)a=b;}
inline void swap(int &a,int &b){int t=a;a=b;b=t;}

inline void fileio(string s){
	if(s=="txt"){
		freopen("input.txt","r",stdin);
		freopen("output.txt","w",stdout);
	}
	else{
		freopen((s+".in").c_str(),"r",stdin);
		freopen((s+".out").c_str(),"w",stdout);
	}
}

using namespace std;

const int WATER=1;
const int STEEL=2;
const int BRICK=3;
const int EMPTY=0;
int m[81][81];

int water[3],steel[3],brick[3];

int main()
{
	ofstream fout("map.txt");
	srand(time(NULL));
	for(int i=0;i<9;i++){
		for(int j=0;j<9;j++){
			m[i][j]=rand()%8;
			if(m[i][j]>3)m[i][j]=0;
		}
	}
	for(int i=0;i<9;i++){
		m[4][i]=m[i][4]=BRICK;
	}
	m[4][2]=m[4][4]=m[4][6]=STEEL;
	for(int i=0;i<2;i++){
		m[i][3]=m[i][5]=m[8-i][3]=m[8-i][5]=BRICK;
	}
	for(int i=2;i<=6;i+=2){
		m[0][i]=m[8][i]=0;
	}
	for(int i=0;i<9;i++){
		for(int j=0;j<9;j++)cout<<m[i][j]<<' ';
		cout<<endl;
	}
	for(int i=0;i<3;i++){
		int msk=1;
		for(int x=i*3;x<(i+1)*3;x++){
			for(int y=0;y<9;y++){
				if(m[x][y]==WATER)water[i] |= msk;
				else if(m[x][y]==STEEL)steel[i] |= msk;
				else if(m[x][y]==BRICK)brick[i] |= msk;
				msk <<= 1;
			}
		}
	}
	fout<<"{\"requests\":[";
	fout<<"{\"waterfield\":[";
	for(int i=0;i<3;i++){
		fout<<water[i];
		if(i!=2)fout<<",";
	}
	fout<<"]";
	fout<<",\"steelfield\":[";
	for(int i=0;i<3;i++){
		fout<<steel[i];
		if(i!=2)fout<<",";
	}
	fout<<"]";
	fout<<",\"brickfield\":[";
	for(int i=0;i<3;i++){
		fout<<brick[i];
		if(i!=2)fout<<",";
	}
	fout<<"],\"mySide\":0}]";
	fout<<",\"responses\":[],\"data\":\"\",\"globaldata\":\"\",\"time_limit\":\"\",\"memory_limit\":\"\"}"<<endl;
	return 0;
}