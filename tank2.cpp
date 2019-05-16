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
#include <ctime>
#include "jsoncpp/json.h"

using namespace std;

const int INF = 200;
const double time_limit = 0.95;
template <typename T>inline void updmax(T &a,T b){if(b>a)a=b;}
template <typename T>inline void updmin(T &a,T b){if(b<a)a=b;}
template <typename T>inline int sgn(T a){
    if(a>0) return 1;
    if(a<0) return -1;
    return 0;
}
typedef pair<int,int> pii;

clock_t ticker = clock();

int myRand(int l,int r){
    return rand()%(r-l+1)+l;
}

namespace TankGame
{
    using std::stack;
    using std::set;
    using std::istream;

#ifdef _MSC_VER
#pragma region 常量定义和说明
#endif

    enum GameResult
    {
        NotFinished = -2,
        Draw = -1,
        Blue = 0,
        Red = 1
    };

    enum FieldItem
    {
        None = 0,
        Brick = 1,
        Steel = 2,
        Base = 4,
        Blue0 = 8,
        Blue1 = 16,
        Red0 = 32,
        Red1 = 64,
        Water = 128
    };

    template<typename T> inline T operator~ (T a) { return (T)~(int)a; }
    template<typename T> inline T operator| (T a, T b) { return (T)((int)a | (int)b); }
    template<typename T> inline T operator& (T a, T b) { return (T)((int)a & (int)b); }
    template<typename T> inline T operator^ (T a, T b) { return (T)((int)a ^ (int)b); }
    template<typename T> inline T& operator|= (T& a, T b) { return (T&)((int&)a |= (int)b); }
    template<typename T> inline T& operator&= (T& a, T b) { return (T&)((int&)a &= (int)b); }
    template<typename T> inline T& operator^= (T& a, T b) { return (T&)((int&)a ^= (int)b); }

    enum Action
    {
        Invalid = -2,
        Stay = -1,
        Up, Right, Down, Left,
        UpShoot, RightShoot, DownShoot, LeftShoot
    };

    // 坐标左上角为原点（0, 0），x 轴向右延伸，y 轴向下延伸
    // Side（对战双方） - 0 为蓝，1 为红
    // Tank（每方的坦克） - 0 为 0 号坦克，1 为 1 号坦克
    // Turn（回合编号） - 从 1 开始

    const int fieldHeight = 9, fieldWidth = 9, sideCount = 2, tankPerSide = 2;

    // 基地的横坐标
    const int baseX[sideCount] = { fieldWidth / 2, fieldWidth / 2 };

    // 基地的纵坐标
    const int baseY[sideCount] = { 0, fieldHeight - 1 };

    const int dx[4] = { 0, 1, 0, -1 }, dy[4] = { -1, 0, 1, 0 };
    const FieldItem tankItemTypes[sideCount][tankPerSide] = {
        { Blue0, Blue1 },{ Red0, Red1 }
    };

    int maxTurn = 100;

#ifdef _MSC_VER
#pragma endregion

#pragma region 工具函数和类
#endif

    inline bool ActionIsMove(Action x)
    {
        return x >= Up && x <= Left;
    }

    inline bool ActionIsShoot(Action x)
    {
        return x >= UpShoot && x <= LeftShoot;
    }

    inline bool ActionDirectionIsOpposite(Action a, Action b)
    {
        return a >= Up && b >= Up && (a + 2) % 4 == b % 4;
    }

    inline bool CoordValid(int x, int y)
    {
        return x >= 0 && x < fieldWidth && y >= 0 && y < fieldHeight;
    }

    // 判断 item 是不是叠在一起的多个坦克
    inline bool HasMultipleTank(FieldItem item)
    {
        // 如果格子上只有一个物件，那么 item 的值是 2 的幂或 0
        // 对于数字 x，x & (x - 1) == 0 当且仅当 x 是 2 的幂或 0
        return !!(item & (item - 1));
    }

    inline int GetTankSide(FieldItem item)
    {
        return item == Blue0 || item == Blue1 ? Blue : Red;
    }

    inline int GetTankID(FieldItem item)
    {
        return item == Blue0 || item == Red0 ? 0 : 1;
    }

    inline bool IsTank(FieldItem k){
        return (k&Blue0) || (k&Blue1) || (k&Red0) || (k&Red1);
    }

    // 获得动作的方向
    inline int ExtractDirectionFromAction(Action x)
    {
        if (x >= Up)
            return x % 4;
        return -1;
    }

    // 物件消失的记录，用于回退
    struct DisappearLog
    {
        FieldItem item;

        // 导致其消失的回合的编号
        int turn;

        int x, y;
        bool operator< (const DisappearLog& b) const
        {
            if (x == b.x)
            {
                if (y == b.y)
                    return item < b.item;
                return y < b.y;
            }
            return x < b.x;
        }
    };

#ifdef _MSC_VER
#pragma endregion

#pragma region TankField 主要逻辑类
#endif

    class TankField
    {
    public:
        //!//!//!// 以下变量设计为只读，不推荐进行修改 //!//!//!//

        // 游戏场地上的物件（一个格子上可能有多个坦克）
        FieldItem gameField[fieldHeight][fieldWidth] = {};

        // 坦克是否存活
        bool tankAlive[sideCount][tankPerSide] = { { true, true },{ true, true } };

        // 基地是否存活
        bool baseAlive[sideCount] = { true, true };
        // 坦克横坐标，-1表示坦克已炸
        int tankX[sideCount][tankPerSide] = {
            { fieldWidth / 2 - 2, fieldWidth / 2 + 2 },{ fieldWidth / 2 + 2, fieldWidth / 2 - 2 }
        };

        // 坦克纵坐标，-1表示坦克已炸
        int tankY[sideCount][tankPerSide] = { { 0, 0 },{ fieldHeight - 1, fieldHeight - 1 } };

        // 当前回合编号
        int currentTurn = 1;

        // 我是哪一方
        int mySide;

        // 用于回退的log
        stack<DisappearLog> logs;

        // 过往动作（previousActions[x] 表示所有人在第 x 回合的动作，第 0 回合的动作没有意义）
        Action previousActions[101][sideCount][tankPerSide] = { { { Stay, Stay },{ Stay, Stay } } };

        //!//!//!// 以上变量设计为只读，不推荐进行修改 //!//!//!//

        // 本回合双方即将执行的动作，需要手动填入
        Action nextAction[sideCount][tankPerSide] = { { Invalid, Invalid },{ Invalid, Invalid } };

        // 判断行为是否合法（出界或移动到非空格子算作非法）
        // 未考虑坦克是否存活
        bool ActionIsValid(int side, int tank, Action act)
        {
            if (act == Invalid)
                return false;
            if (act > Left && previousActions[currentTurn - 1][side][tank] > Left) // 连续两回合射击
                return false;
            if (act == Stay || act > Left)
                return true;
            int x = tankX[side][tank] + dx[act],
                y = tankY[side][tank] + dy[act];
            return CoordValid(x, y) && gameField[y][x] == None;// water cannot be stepped on
        }

        bool newActionIsValid(int side, int tank, Action act)
        {
            if (act == Invalid)
                return false;
            if (act == Stay)
                return true;
            if (act > Left){ // 连续两回合射击
                if(previousActions[currentTurn - 1][side][tank] > Left)return false;
                int dir=ExtractDirectionFromAction(act);
                int x = tankX[side][tank], y = tankY[side][tank];
                for(int i=1;;i++){
                    int newx = x + i*dx[dir], newy = y + i*dy[dir];
                    if(!CoordValid(newx,newy))break;
                    if(gameField[newy][newx] == FieldItem::Steel)return false;
                    if(gameField[newy][newx] == FieldItem::Brick || IsTank(gameField[newy][newx]) || gameField[newy][newx] == Base)return true;
                    if(IsTank(gameField[newy+dx[dir]][newx+dy[dir]]) || IsTank(gameField[newy-dx[dir]][newx-dy[dir]]))return true;
                }
                return false;
            }
            else{
                int x = tankX[side][tank] + dx[act],
                    y = tankY[side][tank] + dy[act];
                return CoordValid(x, y) && gameField[y][x] == None;// water cannot be stepped on
            }
        }
        
        bool tankShoot(int side, int tank){
            if(!tankAlive[side][tank])return false;
            return previousActions[currentTurn - 1][side][tank] > Left;
        }
        int rmp[2][2][9][9],rr[9][9],rb[9][9];/*
        struct ryll{
            int x,y,t;
            bool operator<(const ryll &other)const{
                return t > other.t;
            }
        };
        priority_queue<ryll>Q;
        bool vst[9][9];
        void onwork(int c[][9],int x,int y){
            while(!Q.empty())Q.pop();
            ryll t;
            t.x=x,t.y=y,t.t=0;
            Q.push(t);
            memset(vst,0,sizeof(vst));
            c[x][y]=0;
            while(!Q.empty()){
                t=Q.top();Q.pop();
                if(vst[t.x][t.y])continue;
                vst[t.x][t.y]=1;
                c[t.x][t.y]=t.t;
                for(int i=0;i<4;++i){
                    int X=dx[i]+t.x,Y=dy[i]+t.y;
                    if(X<0||Y<0||X>8||Y>8||vst[X][Y])continue;
                    if(gameField[Y][X]==FieldItem::Steel || gameField[Y][X]==FieldItem::Water)continue;
                    ryll sf;
                    sf.x=X,sf.y=Y,sf.t=t.t+1;
                    if(gameField[Y][X]==FieldItem::Brick)++sf.t;
                    if(IsTank(gameField[Y][X]))sf.t+=2;
                    Q.push(sf);
                }
            }
        }*/
        pii Q[10010];
        int l,r,X,Y,nx,ny,nt;
        bool inque[9][9];
        void onwork(int c[][9],int x,int y){
            l=0;r=0;
            memset(inque,0,sizeof(inque));
            c[x][y]=0;
            Q[r++]=pii(x,y);
            inque[x][y]=true;
            while(l^r){
                pii u=Q[l++];
                nx=u.first;ny=u.second;nt=c[nx][ny];
                inque[nx][ny]=false;
                for(int i=0;i<4;++i){
                    X=dx[i]+nx;Y=dy[i]+ny;
                    if(X<0||Y<0||X>8||Y>8)continue;
                    if(gameField[Y][X]==FieldItem::Steel || gameField[Y][X]==FieldItem::Water)continue;
                    int t=1;
                    if(gameField[Y][X]==FieldItem::Brick)++t;
                    //if(IsTank(gameField[Y][X]))t += 2;
                    if(c[X][Y]>nt+t){
                        c[X][Y]=nt+t;
                        if(!inque[X][Y]){
                            Q[r++]=pii(X,Y);
                            inque[X][Y]=true;
                        }
                    }
                }
            }
        }

        void calc_tank(int side, int tank){
            for(int i=0;i<9;i++){
                for(int j=0;j<9;j++){
                    rmp[side][tank][i][j]=100;
                }
            }
            if(!tankAlive[side][tank])return;
            onwork(rmp[side][tank],tankX[side][tank],tankY[side][tank]);
        }
        void dist_init(){
            for(int i=0;i<=1;i++){
                for(int j=0;j<=1;j++){
                    calc_tank(i,j);
                }
            }
        }
        int getreal(int ts){
            for(int i=0;i<9;i++)
                for(int j=0;j<9;j++)rb[i][j]=min(rmp[0][0][i][j],rmp[0][1][i][j]);
            for(int i=0;i<9;i++)
                for(int j=0;j<9;j++)rr[i][j]=min(rmp[1][0][i][j],rmp[1][1][i][j]);
            int jsq=0;
            //????????????????
            for(int i=0;i<9;i++){
                for(int j=0;j<9;j++){
            //        cout<<rb[j][i]<<','<<rr[j][i]<<' ';
                    jsq+=sgn(rb[i][j]-rr[i][j]);
                }
             //   cout<<endl;
            }
            if(ts)return jsq;
            return -jsq;
        }
        int mob(int side,int tank){
            if(!tankAlive[side][tank])return 0;
            int x=tankX[side][tank],y=tankY[side][tank],jsq=0;
            if(x&&!gameField[y][x-1])jsq++;
            if(x<8&&!gameField[y][x+1])jsq++;
            if(y&&!gameField[y-1][x])jsq++;
            if(y<8&&!gameField[y+1][x])jsq++;
            return jsq;
        }
        bool cantattack(int side,int tank){
            if(!tankAlive[side][tank])return false;
            int x=tankX[side][tank],y=tankY[side][tank];
            int tx=x,ty;
            while(tx>=0&&(!(gameField[y][tx]>0&&gameField[y][tx]<=4))){
                if(!side) {
                    if(gameField[y][tx]==64||gameField[y][tx]==32)return 0;
                }
                else 
                    if(gameField[y][tx]==8||gameField[y][tx]==16)return 0;
                tx--;
            }
            tx=x+1;
            while(tx<fieldWidth&&(!(gameField[y][tx]>0&&gameField[y][tx]<=4))){
                if(!side){if(gameField[y][tx]==64||gameField[y][tx]==32)return 0;}
                else if(gameField[y][tx]==8||gameField[y][tx]==16)return 0;
                tx++; 
            }
            ty=y;
            while(ty>=0&&(!(gameField[ty][x]>0&&gameField[y][tx]<=4))){
                if(!side) {if(gameField[ty][x]==64||gameField[ty][x]==32)return 0;}
                else if(gameField[ty][x]==8||gameField[ty][x]==16)return 0;
                ty--;
            }
            ty=y+1;
            while(ty<fieldHeight&&(!(gameField[ty][x]>0&&gameField[y][tx]<=4))){
                if(!side) {if(gameField[ty][x]==64||gameField[ty][x]==32)return 0;}
                else if(gameField[ty][x]==8||gameField[ty][x]==16)return 0;
                ty++;
            }
            return 1;
        }
        int shoot_times(int x, int y, int x0, int y0){
            if(x==x0 && y==y0)return 0;
            if(x!=x0 && y!=y0)return 100;
            int cnt=1;
            if(x==x0){
                if(y>y0)swap(y,y0);
                for(int i=y+1;i!=y0;i++)
                    if(gameField[i][x]==FieldItem::Steel)
                        return 100;
                    else if(gameField[i][x]==FieldItem::Brick)
                        ++cnt;
                return cnt;
            }
            else if(y==y0){
                if(x>x0)swap(x,x0);
                for(int i=x+1;i!=x0;i++)
                    if(gameField[y][i]==FieldItem::Steel)
                        return 100;
                    else if(gameField[y][i]==FieldItem::Brick)
                        ++cnt;
                return cnt;
            }
            return 100;
        }
        bool canWin(int side, int tank){
            if(!tankAlive[side][tank] || tankShoot(side, tank))return false;
            int x=tankX[side][tank], y=tankY[side][tank];
            int x0=baseX[1-side], y0=baseY[1-side];
            return (shoot_times(x,y,x0,y0)==1);
        }
        bool shoot(int side){
            return canWin(side, 0) || canWin(side, 1);
        }
        int get_astar(int me, int tank, int enemy){
            if(!tankAlive[me][tank])return 100;
            int res=100;
            for(int y=0;y<9;y++){
                for(int x=0;x<9;x++){
                    updmin(res, rmp[me][tank][x][y] + 2 * shoot_times(x, y, baseX[enemy], baseY[enemy]));
                }
            }
            return res;
        }
        //""
        // 判断 nextAction 中的所有行为是否都合法
        // 忽略掉未存活的坦克
        bool ActionIsValid()
        {
            for (int side = 0; side < sideCount; side++)
                for (int tank = 0; tank < tankPerSide; tank++)
                    if (tankAlive[side][tank] && !ActionIsValid(side, tank, nextAction[side][tank]))
                        return false;
            return true;
        }

    private:
        void _destroyTank(int side, int tank)
        {
            tankAlive[side][tank] = false;
            tankX[side][tank] = tankY[side][tank] = -1;
        }

        void _revertTank(int side, int tank, DisappearLog& log)
        {
            int &currX = tankX[side][tank], &currY = tankY[side][tank];
            if (tankAlive[side][tank])
                gameField[currY][currX] &= ~tankItemTypes[side][tank];
            else
                tankAlive[side][tank] = true;
            currX = log.x;
            currY = log.y;
            gameField[currY][currX] |= tankItemTypes[side][tank];
        }
    public:

        // 执行 nextAction 中指定的行为并进入下一回合，返回行为是否合法
        bool DoAction()
        {
            if (!ActionIsValid())
                return false;

            //cout<<"ActionDid"<<nextAction[0][0]<<' '<<nextAction[0][1]<<' '<<nextAction[1][0]<<' '<<nextAction[1][1]<<endl;

            // 1 移动
            for (int side = 0; side < sideCount; side++)
                for (int tank = 0; tank < tankPerSide; tank++)
                {
                    Action act = nextAction[side][tank];

                    // 保存动作
                    previousActions[currentTurn][side][tank] = act;
                    if (tankAlive[side][tank] && ActionIsMove(act))
                    {
                        int &x = tankX[side][tank], &y = tankY[side][tank];
                        FieldItem &items = gameField[y][x];

                        // 记录 Log
                        DisappearLog log;
                        log.x = x;
                        log.y = y;
                        log.item = tankItemTypes[side][tank];
                        log.turn = currentTurn;
                        logs.push(log);

                        // 变更坐标
                        x += dx[act];
                        y += dy[act];

                        // 更换标记（注意格子可能有多个坦克）
                        gameField[y][x] |= log.item;
                        items &= ~log.item;
                    }
                }

            // 2 射♂击!
            set<DisappearLog> itemsToBeDestroyed;
            for (int side = 0; side < sideCount; side++)
                for (int tank = 0; tank < tankPerSide; tank++)
                {
                    Action act = nextAction[side][tank];
                    if (tankAlive[side][tank] && ActionIsShoot(act))
                    {
                        int dir = ExtractDirectionFromAction(act);
                        int x = tankX[side][tank], y = tankY[side][tank];
                        bool hasMultipleTankWithMe = HasMultipleTank(gameField[y][x]);
                        while (true)
                        {
                            x += dx[dir];
                            y += dy[dir];
                            if (!CoordValid(x, y))
                                break;
                            FieldItem items = gameField[y][x];
                            //tank will not be on water, and water will not be shot, so it can be handled as None
                            if (items != None && items != Water)
                            {
                                // 对射判断
                                if (items >= Blue0 &&
                                    !hasMultipleTankWithMe && !HasMultipleTank(items))
                                {
                                    // 自己这里和射到的目标格子都只有一个坦克
                                    Action theirAction = nextAction[GetTankSide(items)][GetTankID(items)];
                                    if (ActionIsShoot(theirAction) &&
                                        ActionDirectionIsOpposite(act, theirAction))
                                    {
                                        // 而且我方和对方的射击方向是反的
                                        // 那么就忽视这次射击
                                        break;
                                    }
                                }

                                // 标记这些物件要被摧毁了（防止重复摧毁）
                                for (int mask = 1; mask <= Red1; mask <<= 1)
                                    if (items & mask)
                                    {
                                        DisappearLog log;
                                        log.x = x;
                                        log.y = y;
                                        log.item = (FieldItem)mask;
                                        log.turn = currentTurn;
                                        itemsToBeDestroyed.insert(log);
                                    }
                                break;
                            }
                        }
                    }
                }

            for (auto& log : itemsToBeDestroyed)
            {
                switch (log.item)
                {
                case Base:
                {
                    int side = log.x == baseX[Blue] && log.y == baseY[Blue] ? Blue : Red;
                    baseAlive[side] = false;
                    break;
                }
                case Blue0:
                    _destroyTank(Blue, 0);
                    break;
                case Blue1:
                    _destroyTank(Blue, 1);
                    break;
                case Red0:
                    _destroyTank(Red, 0);
                    break;
                case Red1:
                    _destroyTank(Red, 1);
                    break;
                case Steel:
                    continue;
                default:
                    ;
                }
                gameField[log.y][log.x] &= ~log.item;
                logs.push(log);
            }

            for (int side = 0; side < sideCount; side++)
                for (int tank = 0; tank < tankPerSide; tank++)
                    nextAction[side][tank] = Invalid;

            currentTurn++;
            return true;
        }

        // 回到上一回合
        bool Revert()
        {
            if (currentTurn == 1)
                return false;
            //cout<<"Undo"<<endl;
            currentTurn--;
            while (!logs.empty())
            {
                DisappearLog& log = logs.top();
                if (log.turn == currentTurn)
                {
                    logs.pop();
                    switch (log.item)
                    {
                    case Base:
                    {
                        int side = log.x == baseX[Blue] && log.y == baseY[Blue] ? Blue : Red;
                        baseAlive[side] = true;
                        gameField[log.y][log.x] = Base;
                        break;
                    }
                    case Brick:
                        gameField[log.y][log.x] = Brick;
                        break;
                    case Blue0:
                        _revertTank(Blue, 0, log);
                        break;
                    case Blue1:
                        _revertTank(Blue, 1, log);
                        break;
                    case Red0:
                        _revertTank(Red, 0, log);
                        break;
                    case Red1:
                        _revertTank(Red, 1, log);
                        break;
                    default:
                        ;
                    }
                }
                else
                    break;
            }
            return true;
        }

        // 游戏是否结束？谁赢了？
        GameResult GetGameResult()
        {
            bool fail[sideCount] = {};
            for (int side = 0; side < sideCount; side++)
                if ((!tankAlive[side][0] && !tankAlive[side][1]) || !baseAlive[side])
                    fail[side] = true;
            if (fail[0] == fail[1])
                return fail[0] || currentTurn > maxTurn ? Draw : NotFinished;
            if (fail[Blue])
                return Red;
            return Blue;
        }

        /* 三个 int 表示场地 01 矩阵（每个 int 用 27 位表示 3 行）
           initialize gameField[][]
           brick>water>steel
        */
        TankField(int hasBrick[3],int hasWater[3],int hasSteel[3], int mySide) : mySide(mySide)
        {
            for (int i = 0; i < 3; i++)
            {
                int mask = 1;
                for (int y = i * 3; y < (i + 1) * 3; y++)
                {
                    for (int x = 0; x < fieldWidth; x++)
                    {
                        if (hasBrick[i] & mask)
                            gameField[y][x] = Brick;
                        else if(hasWater[i] & mask)
                            gameField[y][x] = Water;
                        else if(hasSteel[i] & mask)
                            gameField[y][x] = Steel;
                        mask <<= 1;
                    }
                }
            }
            for (int side = 0; side < sideCount; side++)
            {
                for (int tank = 0; tank < tankPerSide; tank++)
                    gameField[tankY[side][tank]][tankX[side][tank]] = tankItemTypes[side][tank];
                gameField[baseY[side]][baseX[side]] = Base;
            }
        }
        // 打印场地
        void DebugPrint()
        {
#ifndef _BOTZONE_ONLINE
            const string side2String[] = { "蓝", "红" };
            const string boolean2String[] = { "已炸", "存活" };
            const char* boldHR = "==============================";
            const char* slimHR = "------------------------------";
            cout << boldHR << endl
                << "图例：" << endl
                << ". - 空\t# - 砖\t% - 钢\t* - 基地\t@ - 多个坦克" << endl
                << "b - 蓝0\tB - 蓝1\tr - 红0\tR - 红1\tW - 水" << endl //Tank2 feature
                << slimHR << endl;
            for (int y = 0; y < fieldHeight; y++)
            {
                for (int x = 0; x < fieldWidth; x++)
                {
                    switch (gameField[y][x])
                    {
                    case None:
                        cout << '.';
                        break;
                    case Brick:
                        cout << '#';
                        break;
                    case Steel:
                        cout << '%';
                        break;
                    case Base:
                        cout << '*';
                        break;
                    case Blue0:
                        cout << 'b';
                        break;
                    case Blue1:
                        cout << 'B';
                        break;
                    case Red0:
                        cout << 'r';
                        break;
                    case Red1:
                        cout << 'R';
                        break;
                    case Water:
                        cout << 'W';
                        break;
                    default:
                        cout << '@';
                        break;
                    }
                }
                cout << endl;
            }
            cout << slimHR << endl;
            for (int side = 0; side < sideCount; side++)
            {
                cout << side2String[side] << "：基地" << boolean2String[baseAlive[side]];
                for (int tank = 0; tank < tankPerSide; tank++)
                    cout << ", 坦克" << tank << boolean2String[tankAlive[side][tank]];
                cout << endl;
            }
            cout << "当前回合：" << currentTurn << "，";
            GameResult result = GetGameResult();
            if (result == -2)
                cout << "游戏尚未结束" << endl;
            else if (result == -1)
                cout << "游戏平局" << endl;
            else
                cout << side2String[result] << "方胜利" << endl;
            cout << boldHR << endl;
#endif
        }

        bool operator!= (const TankField& b) const
        {

            for (int y = 0; y < fieldHeight; y++)
                for (int x = 0; x < fieldWidth; x++)
                    if (gameField[y][x] != b.gameField[y][x])
                        return true;

            for (int side = 0; side < sideCount; side++)
                for (int tank = 0; tank < tankPerSide; tank++)
                {
                    if (tankX[side][tank] != b.tankX[side][tank])
                        return true;
                    if (tankY[side][tank] != b.tankY[side][tank])
                        return true;
                    if (tankAlive[side][tank] != b.tankAlive[side][tank])
                        return true;
                }

            if (baseAlive[0] != b.baseAlive[0] ||
                baseAlive[1] != b.baseAlive[1])
                return true;

            if (currentTurn != b.currentTurn)
                return true;

            return false;
        }
    };

#ifdef _MSC_VER
#pragma endregion
#endif

    TankField *field;

#ifdef _MSC_VER
#pragma region 与平台交互部分
#endif

    // 内部函数
    namespace Internals
    {
        Json::Reader reader;
#ifdef _BOTZONE_ONLINE
        Json::FastWriter writer;
#else
        Json::StyledWriter writer;
#endif

        void _processRequestOrResponse(Json::Value& value, bool isOpponent)
        {
            if (value.isArray())
            {
                if (!isOpponent)
                {
                    for (int tank = 0; tank < tankPerSide; tank++)
                        field->nextAction[field->mySide][tank] = (Action)value[tank].asInt();
                }
                else
                {
                    for (int tank = 0; tank < tankPerSide; tank++)
                        field->nextAction[1 - field->mySide][tank] = (Action)value[tank].asInt();
                    field->DoAction();
                }
            }
            else
            {
                // 是第一回合，裁判在介绍场地
                int hasBrick[3],hasWater[3],hasSteel[3];
                for (int i = 0; i < 3; i++){//Tank2 feature(???????????????)
                    hasWater[i] = value["waterfield"][i].asInt();
                    hasBrick[i] = value["brickfield"][i].asInt();
                    hasSteel[i] = value["steelfield"][i].asInt();
                }
                field = new TankField(hasBrick,hasWater,hasSteel,value["mySide"].asInt());
            }
        }

        // 请使用 SubmitAndExit 或者 SubmitAndDontExit
        void _submitAction(Action tank0, Action tank1, string debug = "", string data = "", string globalData = "")
        {
            Json::Value output(Json::objectValue), response(Json::arrayValue);
            response[0U] = tank0;
            response[1U] = tank1;
            output["response"] = response;
            if (!debug.empty())
                output["debug"] = debug;
            if (!data.empty())
                output["data"] = data;
            if (!globalData.empty())
                output["globalData"] = globalData;
            cout << writer.write(output) << endl;
        }
    }

    // 从输入流（例如 cin 或者 fstream）读取回合信息，存入 TankField，并提取上回合存储的 data 和 globaldata
    // 本地调试的时候支持多行，但是最后一行需要以没有缩进的一个"}"或"]"结尾
    void ReadInput(istream& in, string& outData, string& outGlobalData)
    {
        Json::Value input;
        string inputString;
        do
        {
            getline(in, inputString);
        } while (inputString.empty());
#ifndef _BOTZONE_ONLINE
        // 猜测是单行还是多行
        char lastChar = inputString[inputString.size() - 1];
        if (lastChar != '}' && lastChar != ']')
        {
            // 第一行不以}或]结尾，猜测是多行
            string newString;
            do
            {
                getline(in, newString);
                inputString += newString;
            } while (newString != "}" && newString != "]");
        }
#endif
        Internals::reader.parse(inputString, input);

        if (input.isObject())
        {
            Json::Value requests = input["requests"], responses = input["responses"];
            if (!requests.isNull() && requests.isArray())
            {
                unsigned int i, n = requests.size();
                for (i = 0; i < n; i++)
                {
                    Internals::_processRequestOrResponse(requests[i], true);
                    if (i < n - 1)
                        Internals::_processRequestOrResponse(responses[i], false);
                }
                outData = input["data"].asString();
                outGlobalData = input["globaldata"].asString();
                return;
            }
        }
        Internals::_processRequestOrResponse(input, true);
    }

    // 提交决策并退出，下回合时会重新运行程序
    void SubmitAndExit(Action tank0, Action tank1, string debug = "", string data = "", string globalData = "")
    {
        Internals::_submitAction(tank0, tank1, debug, data, globalData);
        exit(0);
    }

    // 提交决策，下回合时程序继续运行（需要在 Botzone 上提交 Bot 时选择“允许长时运行”）
    // 如果游戏结束，程序会被系统杀死
    void SubmitAndDontExit(Action tank0, Action tank1)
    {
        Internals::_submitAction(tank0, tank1);
        field->nextAction[field->mySide][0] = tank0;
        field->nextAction[field->mySide][1] = tank1;
        cout << ">>>BOTZONE_REQUEST_KEEP_RUNNING<<<" << endl;
    }
#ifdef _MSC_VER
#pragma endregion
#endif
}
int RandBetween(int from, int to)
{
    return rand() % (to - from) + from;
}

TankGame::Action RandAction(int tank)
{
    while (true)
    {
        auto act = (TankGame::Action)RandBetween(TankGame::Stay, TankGame::LeftShoot + 1);
        if (TankGame::field->ActionIsValid(TankGame::field->mySide, tank, act))
            return act;
    }
}

double d(double x){
    return 5*log(x+1);
}

double func(double x, double y){
    x=d(x);y=d(y);
    return sqrt((x*x+y*y)/2);
}

double value1(TankGame::TankField &field){  //只有这里需要重写
    if((clock()-ticker)/double(CLOCKS_PER_SEC) > time_limit)return -INF;
    using namespace TankGame;
    field.dist_init();
    double res = 0;
    int me=field.mySide, enemy=1-me;
    GameResult result=field.GetGameResult();
    if(result==enemy)res += -INF;
    if(result==me)res += INF;
    //if(field.shoot(me))res += INF/2;
    //if(field.shoot(enemy))res += -INF/2;
    const double t1=10,t2=0,t3=1,t4=5;

    const double tank_alive = 100, tank_shoot = 0.2;
    if(field.tankShoot(me,0))res-=tank_shoot;
    if(field.tankShoot(me,1))res-=tank_shoot;
    if(field.tankShoot(enemy,0))res+=tank_shoot;
    if(field.tankShoot(enemy,1))res+=tank_shoot;

    //tank占领区域
    res+=t3*field.getreal(me);

    //tank打到对方老巢所需时间
    double dist[2][2];
    dist[me][0]=field.get_astar(me,0,enemy);
    dist[me][1]=field.get_astar(me,1,enemy);
    dist[enemy][0]=field.get_astar(enemy,0,me);
    dist[enemy][1]=field.get_astar(enemy,1,me);

    //cout<<dist[0][0]<<' '<<dist[0][1]<<' '<<dist[1][0]<<' '<<dist[1][1]<<endl;
    double u1=min(dist[me][0],dist[me][1]), u2=min(dist[enemy][0],dist[enemy][1]);
    res-=t1*func(dist[me][0],dist[me][1])*2;
    res+=t1*func(dist[enemy][0],dist[enemy][1]);

    //res+=evaluate(mp(field.tankX[me][0],field.tanY[me][0]))

    //tank的灵活度
    res+=t2*(field.mob(me,0)+field.mob(me,1)-field.mob(enemy,0)-field.mob(enemy,1));

    //tank是否存活 
    if(field.tankAlive[me][0])res += tank_alive;
    if(field.tankAlive[me][1])res += tank_alive;
    if(field.tankAlive[enemy][0])res -= tank_alive;
    if(field.tankAlive[enemy][1])res -= tank_alive;
    ////tank卖弱行为 
    /*
    if(field.tankShoot(me,0)&&field.cantattack(me,0))res+=5;
    if(field.tankShoot(me,0)&&field.cantattack(me,1))res+=5;
    if(field.tankShoot(enemy,0)&&field.cantattack(enemy,0))res-=5;
    if(field.tankShoot(enemy,0)&&field.cantattack(enemy,1))res-=5;*/
    //cout<<res<<endl;
    //cout<<res<<endl;
    return res;
}
struct TwoActions{
    TankGame::Action action1, action2;
    TwoActions():action1(TankGame::Action::Stay),action2(action1){}
    TwoActions(const TwoActions &other):action1(other.action1),action2(other.action2){}
    TwoActions(TankGame::Action a, TankGame::Action b):action1(a),action2(b){}
    friend ostream& operator<<(ostream &out, TwoActions x){
        out<<'('<<x.action1<<','<<x.action2<<')';
        return out;
    }
};
class MyArtificialIdiot1{
public:
    const static int search_depth_limit = 2;
    static int gen(int k){return rand()%k;}
    double searched_value(TankGame::TankField &field, double (*value)(TankGame::TankField &f), int depth){
        if(depth==0 || (clock()-ticker)/double(CLOCKS_PER_SEC)>time_limit)return value(field);
        vector<TwoActions> myValidActions, enemyValidActions;
        vector<TankGame::Action> my[2],enemy[2];
        int me=field.mySide;
        for(TankGame::Action i=TankGame::Action::Stay;i<=TankGame::Action::LeftShoot;i = TankGame::Action(i+1)){
            if(field.newActionIsValid(me,0,i))my[0].push_back(i);
            if(field.newActionIsValid(me,1,i))my[1].push_back(i);
            if(field.newActionIsValid(1-me,0,i))enemy[0].push_back(i);
            if(field.newActionIsValid(1-me,1,i))enemy[1].push_back(i);
        }
        for(int i=0;i<2;i++){
            if(!field.tankAlive[me][i]){
                my[i].clear();
                my[i].push_back(TankGame::Action::Stay);
            }
            if(!field.tankAlive[1-me][i]){
                enemy[i].clear();
                enemy[i].push_back(TankGame::Action::Stay);
            }
        }
        for(auto u:my[0]){
            for(auto v:my[1]){
                myValidActions.push_back(TwoActions(u,v));
            }
        }
        for(auto u:enemy[0]){
            for(auto v:enemy[1]){
                enemyValidActions.push_back(TwoActions(u,v));
            }
        }
        double val=-0x3f3f3f3f;
        for(auto myAction:myValidActions){
            double cur=0, div=0;
            for(auto enemyAction:enemyValidActions){
                field.nextAction[me][0] = myAction.action1;
                field.nextAction[me][1] = myAction.action2;
                field.nextAction[1-me][0] = enemyAction.action1;
                field.nextAction[1-me][1] = enemyAction.action2;
                field.DoAction();
                double u;
                if(depth==1)u=value(field);
                else u=searched_value(field, value, depth-1);
                double v=weight(u);
                cur += v*u; div += v;
                field.Revert();
            }
            updmax(val, cur/div);
        }
        return val;
    }
    static double weight(double u){
        return exp(-u/50);
    }
    double search(TankGame::TankField &field, double (*value)(TankGame::TankField &f), TwoActions myAction, bool ispre){
        if((clock()-ticker)/double(CLOCKS_PER_SEC)>time_limit)return -200;
        vector<TwoActions> enemyValidActions;
        vector<TankGame::Action> enemy[2];
        int me=field.mySide;
        for(TankGame::Action i=TankGame::Action::Stay;i<=TankGame::Action::LeftShoot;i = TankGame::Action(i+1)){
            if(field.newActionIsValid(1-me,0,i))enemy[0].push_back(i);
            if(field.newActionIsValid(1-me,1,i))enemy[1].push_back(i);
        }
        for(int i=0;i<2;i++){
            if(!field.tankAlive[1-me][i]){
                enemy[i].clear();
                enemy[i].push_back(TankGame::Action::Stay);
            }
        }
        for(auto u:enemy[0]){
            for(auto v:enemy[1]){
                enemyValidActions.push_back(TwoActions(u,v));
            }
        }
        double current_val=0, div=0;
        for(auto enemyAction:enemyValidActions){
            field.nextAction[me][0] = myAction.action1;
            field.nextAction[me][1] = myAction.action2;
            field.nextAction[1-me][0] = enemyAction.action1;
            field.nextAction[1-me][1] = enemyAction.action2;
            field.DoAction();
            double u;
            if(ispre)u=value(field);
            else u=searched_value(field, value, search_depth_limit-1);
            //cout<<myAction<<' '<<enemyAction<<' '<<u<<endl;
            double v=weight(u);
            current_val += v*u; div += v;
            field.Revert();
        }
        return current_val / div;
    }
    typedef pair<TwoActions, double> node;
    static inline bool cmp(const node &a, const node &b){
        return a.second > b.second;
    }
    TwoActions run(TankGame::TankField &field, double (*value)(TankGame::TankField &f)){
        using namespace TankGame;
        vector<Action> my[2];
        vector<TwoActions> myValidActions;
        vector<node> tmp;
        int me=field.mySide;
        for(TankGame::Action i=TankGame::Action::Stay;i<=TankGame::Action::LeftShoot;i = TankGame::Action(i+1)){
            if(field.newActionIsValid(me,0,i))my[0].push_back(i);
            if(field.newActionIsValid(me,1,i))my[1].push_back(i);
        }
        for(int i=0;i<2;i++){
            if(!field.tankAlive[me][i]){
                my[i].clear();
                my[i].push_back(TankGame::Action::Stay);
            }
        }
        for(auto u:my[0]){
            for(auto v:my[1]){
                TwoActions cur(u,v);
                tmp.push_back(node(cur,search(field, value, cur, true)));
            }
        }
        random_shuffle(tmp.begin(),tmp.end());
        sort(tmp.begin(),tmp.end(),cmp);
        for(auto u:tmp){
            myValidActions.push_back(u.first);
        }
        double maxi=-1e18;
        TwoActions response;
        for(auto myAction:myValidActions){
            double u=search(field, value, myAction, false);
            //cout<<myAction<<' '<<u<<endl;
            if(maxi<u){
                maxi=u;
                response=myAction;
            }
            if((clock()-ticker)/double(CLOCKS_PER_SEC)>time_limit)break;
        }
        return response;
    }
};

#ifndef _BOTZONE_ONLINE
ifstream filein("map.txt");
#else
#define filein cin
#endif

int main()
{
    srand(time(NULL));
    MyArtificialIdiot1 AI;
        string data, globaldata;
        TankGame::ReadInput(filein, data, globaldata);
        TankGame::field->DebugPrint();
        TwoActions result = AI.run(*(TankGame::field), value1);
        TankGame::SubmitAndExit(result.action1, result.action2);
}