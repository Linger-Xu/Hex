#include<iostream>
#include<vector>
#include<string>
#include<queue>
#include<ctime>
#include<cstdlib>
#include<random>
#include<string.h>
#include<algorithm>
using namespace std;

const int SIZE = 15;
int board[SIZE][SIZE] = { 0 };//本方1，对方-1，空白0
int Offset_x[6] = { -1,-1,0,1,1,0 };
int Offset_y[6] = { 0,1,1,0,-1,-1 };
mt19937 rnd(1);                //用种子生成随机数                
int stateCount = 1;                //用于记录涉及到的状态数量,棋盘为空的时候是第一个状态
const int MAXSTATENUM = 1e7;
int SIMULATENUM = 7;        //模拟次数
vector<pair<int, int> >allbox;
int Stimulate_Num = 0;
int judgeourturn = 0;
double C;

class Hex_board;
int getid(int x, int y) {//表格棋盘一维化
	return 11 * x + y;
}

void decodeid(int id, int& x, int& y) {//一维化解码
	x = id / 11;
	y = id % 11;
}

bool inboard(int x, int y) {                //判断节点坐标是不是在棋盘中
	if (x >= 0 && x < 11 && y >= 0 && y < 11) return true;
	return false;
}


vector<vector<int> >path;
int st[130] = { 0 };
int top = 0;

int chkposlist1[11] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
int chkposlist2[11] = { 10, 21, 32, 43, 54, 65, 76, 87, 98, 109, 120 };
bool chkpos(int pos, int mode) {
	if (mode == 1) {
		for (int i = 0; i < 11; i++) {
			if (pos == chkposlist1[i])return true;
		}
	}
	else if (mode == 2) {
		for (int i = 0; i < 11; i++) {
			if (pos == chkposlist2[i])return true;
		}
	}
	return false;
}

class Hex_ChessBoard {        //棋盘类   
public:
	int board[11][11];  //本方1，对方-1，空白0
	int fa[130];        //并查集父节点数组
	double UCB;
	int currentplayer;                //现在的玩家

	int find(int u) {                        //并查集找父节点操作
		if (fa[u] == u) return u;
		else return fa[u] = find(fa[u]);
	}

	void combine(int x, int y, int newx, int newy) {//并查集的合并，此处我认为有查找的路径压缩不用按秩合并
		fa[find(getid(x, y))] = find(getid(newx, newy));
	}

	Hex_ChessBoard() {                                                                                //初始化构造函数
		memset(board, 0, sizeof(board));
		for (int i = 0; i < 130; i++) {
			fa[i] = i;
		}
		currentplayer = 1;
		UCB = 0;
	}

	void putapiece(int x, int y) {                        //放一个棋子
		board[x][y] = currentplayer;                        //放下一个子
		for (int i = 0; i < 6; i++) {                        //查找周边的6个位置的子，有相同的就合并
			int newx = x + Offset_x[i];
			int newy = y + Offset_y[i];
			if (inboard(newx, newy)) {          //偏移位置在棋盘范围内
				if (board[newx][newy] == board[x][y])   combine(x, y, newx, newy);//相同合并
			}
			else {
				if (currentplayer == 1)
				{
					if (newx == -1) fa[find(getid(x, y))] = find(121);//如果currentplayer是1，认为是红棋， 红棋的两边认为是121和122
					if (newx == 11) fa[find(getid(x, y))] = find(122);//如果周边节点超出范围，说明xy是边缘节点，把节点和边连起来
				}
				else
				{
					if (newy == -1) fa[find(getid(x, y))] = find(123);//同理，对蓝棋操作
					if (newy == 11) fa[find(getid(x, y))] = find(124);
				}
			}
		}
		currentplayer *= -1;                                                                                //交换棋手
	}
	bool redwin() { return find(121) == find(122); }
	bool bluewin() { return find(123) == find(124); }
	bool gameover() { return redwin() || bluewin(); }

};

Hex_ChessBoard nowstate;
Hex_ChessBoard input;


void dfs(int pos, int end, Hex_ChessBoard& c, int vis[], int mode)//从pos点开始访问
{

	if (chkpos(pos, mode)) {//到达终点
		vector<int>tmp;
		for (int i = 0; i < top; i++)
			tmp.push_back(st[i]);
		path.push_back(tmp);
		return;
	}
	vis[pos] = 1;//标记被访问过 
	st[top++] = pos;//经过的路径加入队列
	for (int i = 0; i < 6; i++) {
		int xx = 0;
		int yy = 0;
		if (mode == 1 && (i == 3 || i == 4 || i == 2 || i == 5))continue;
		if (mode == 2 && (i == 5 || i == 4 || i == 0 || i == 3))continue;
		decodeid(pos, xx, yy);
		if (inboard(xx + Offset_x[i], yy + Offset_y[i]) && c.board[xx + Offset_x[i]][yy + Offset_y[i]] == 0 && vis[getid(xx + Offset_x[i], yy + Offset_y[i])] == 0) {//如果这个点没有被访问过，而且b与这个点相连，就继续搜索
			//cout << xx + Offset_x[i] << " " << yy + Offset_y[i] << endl;
			dfs(getid(xx + Offset_x[i], yy + Offset_y[i]), end, c, vis, mode);
		}
	}
	vis[pos] = 0;//删除标记 
	top--;//队列里删除b
}

int maxflow(int mode) {
	Hex_ChessBoard a = input;
	int countpath[11][11] = { 0 };
	int vis[130] = { 0 };
	if (mode == 1) {
		for (int i = 110; i < 121; i++) {
			int xx = 0;
			int yy = 0;
			decodeid(i, xx, yy);
			if (a.board[xx][yy] != 0)continue;
			memset(vis, 0, sizeof(vis));
			memset(st, 0, sizeof(st));
			for (int j = 110; j < 121; j++) {
				if (j != i)
					vis[j] = 1;
			}
			top = 0;

			dfs(i, 122, a, vis, 1);
		}
	}
	else if (mode == 2) {
		for (int i = 0; i < 120; i += 11) {
			int xx = 0;
			int yy = 0;
			decodeid(i, xx, yy);
			if (a.board[xx][yy] != 0)continue;
			top = 0;
			memset(vis, 0, sizeof(vis));
			memset(st, 0, sizeof(st));
			for (int j = 0; j < 120; j += 11) {
				if (j != i)
					vis[j] = 1;
			}
			dfs(i, 122, a, vis, 2);
		}
	}

	for (int i = 0; i < path.size(); i++) {
		for (int j = 0; j < path[i].size(); j++) {
			int tmp = path[i][j];
			int xx = 0;
			int yy = 0;
			decodeid(tmp, xx, yy);
			countpath[xx][yy]++;
		}
	}
	int xx = 0;
	int yy = 0;
	int max = 0;


	for (int i = 0; i < 11; i++) {
		for (int j = 0; j < 11; j++) {
			if (countpath[i][j] > max && a.board[i][j] == 0) {
				max = countpath[i][j];
				xx = i;
				yy = j;
			}
		}
	}
	return getid(xx, yy);
}



int Bridge_Offset_x[6] = { -2,-1,1,2,1,-1 };
int Bridge_Offset_y[6] = { 1,2,1,-1,-2,-1 };

bool feasible(Hex_ChessBoard& state, int x, int y, int i)                //判断是否可以构成桥对:1.成桥对的棋子是不是同色的 2.成桥对的棋子中间是否都没有棋子 3.成桥对的棋子有没有越界
{
	if (state.board[x][y] != state.board[x + Bridge_Offset_x[i]][y + Bridge_Offset_y[i]] || state.board[x + Offset_x[i]][y + Offset_y[i]] != 0 || state.board[x + Offset_x[(i + 1) % 6]][y + Offset_y[(i + 1) % 6]] != 0)
		return false;
	int newx = x + Bridge_Offset_x[i];
	int newy = y + Bridge_Offset_y[i];
	if (Bridge_Offset_x[i] < 0) { if (newx < 0)        return false; }
	else { if (newx >= 11)        return false; }
	if (Bridge_Offset_y[i] < 0) { if (newy < 0) return false; }
	else { if (newy >= 11)        return false; }
	return true;
}

void bridgeoperation(Hex_ChessBoard& state, int x, int y, int R[], int R_size) {                //查看是否构成桥然后填桥
	//首先考虑6种基本情况
	for (int j = 0; j < R_size; j++)
	{
		int i = R[j];
		if (feasible(state, x, y, i))
		{
			if (rnd() & 1)        state.putapiece(x + Offset_x[i], y + Offset_y[i]), state.putapiece(x + Offset_x[(i + 1) % 6], y + Offset_y[(i + 1) % 6]);
			else state.putapiece(x + Offset_x[(i + 1) % 6], y + Offset_y[(i + 1) % 6]), state.putapiece(x + Offset_x[i], y + Offset_y[i]);
		}
	}
	//接下来考虑四个边界情况
	if (state.board[x][y] == 1 && x == 1 && y < 10 && state.board[x - 1][y] == 0 && state.board[x - 1][y + 1] == 0)
	{
		if (rnd() & 1) state.putapiece(x - 1, y), state.putapiece(x - 1, y + 1);
		else state.putapiece(x - 1, y + 1), state.putapiece(x - 1, y);
	}
	if (state.board[x][y] == 1 && x == 9 && y > 0 && state.board[x + 1][y - 1] == 0 && state.board[x + 1][y] == 0)
	{
		if (rnd() & 1) state.putapiece(x + 1, y), state.putapiece(x + 1, y - 1);
		else state.putapiece(x + 1, y - 1), state.putapiece(x + 1, y);
	}
	if (state.board[x][y] == -1 && x < 10 && y == 1 && state.board[x][y - 1] == 0 && state.board[x + 1][y - 1] == 0)
	{
		if (rnd() & 1) state.putapiece(x, y - 1), state.putapiece(x + 1, y - 1);
		else state.putapiece(x + 1, y - 1), state.putapiece(x, y - 1);
	}
	if (state.board[x][y] == -1 && x > 0 && y == 9 && state.board[x][y + 1] == 0 && state.board[x - 1][y + 1] == 0)
	{
		if (rnd() & 1) state.putapiece(x, y + 1), state.putapiece(x - 1, y + 1);
		else state.putapiece(x - 1, y + 1), state.putapiece(x, y + 1);
	}
}



void rollout() {                        //随机落子直到游戏结束
	shuffle(allbox.begin(), allbox.end(), rnd);
	int R[6] = { 0,1,2,3,4,5 };
	for (vector<pair<int, int> >::iterator iter = allbox.begin(); iter != allbox.end(); iter++)
	{
		int x = iter->first;
		int y = iter->second;
		if (nowstate.board[x][y] == 0) {
			nowstate.putapiece(x, y);
			bridgeoperation(nowstate, x, y, R, 6);
		}
		if (nowstate.gameover())        return;
	}
}

int dis[11][11] = { 0 };                //用于判定下一步可能性的依据：dist数组
int id[122] = { 0 };                        //可行解数组
int nodeidlist[MAXSTATENUM] = { 0 };        //存储树节点里的状态
int head[MAXSTATENUM] = { 0 };                        //存储树节点孩子链表的头节点
int nxt[MAXSTATENUM] = { 0 };                        //存储某个树节点的兄弟节点
int vistime[MAXSTATENUM] = { 0 };                //存储树节点的访问次数
int wintime[MAXSTATENUM] = { 0 };                //存储树节点的胜利次数
int currentstateid;                                                //当前棋盘状态的下标
int MAXDist;                                                        //BFS的深度限制

void Expand() {                                                                                                //扩展操作
	memset(dis, -1, sizeof(dis));
	//通过类似BFS的位置将每个有棋子的地方周围一部分的空位列入下一步的备选位置
	queue<pair<int, int> >occupied;
	for (int i = 0; i < 11; i++) {                                                        //找到所有下过棋的节点
		for (int j = 0; j < 11; j++) {
			if (nowstate.board[i][j] != 0) {
				dis[i][j] = 0;                                                                //填dist数组
				occupied.push(make_pair(i, j));                                //下过棋的位置入队
			}
		}
	}
	int cnt = 0;
	while (!occupied.empty()) {                                                                //往外找现在的节点之外没有占用的节点MaxDist层
		int x = occupied.front().first;
		int y = occupied.front().second;
		occupied.pop();
		if (dis[x][y] == MAXDist)break;                                                //拓展已经抵达深度
		for (int i = 0; i < 6; i++) {
			int newx = x + Offset_x[i];
			int newy = y + Offset_y[i];
			if (inboard(newx, newy) && dis[newx][newy] == -1) {        //位置合法且没下过棋
				dis[newx][newy] = dis[x][y] + 1;                                //填dist数组
				occupied.push(make_pair(newx, newy));
				id[++cnt] = getid(newx, newy);                                //id存放可以下的位置的编号（一维后的编号）
			}
		}
	}
	shuffle(id + 1, id + cnt + 1, rnd);                                                        //把id随机排序，为了随机在新节点中找一个
	for (int i = 1; i <= cnt; i++)
	{
		int state_id = ++stateCount;
		nodeidlist[state_id] = id[i];
		nxt[state_id] = head[currentstateid];
		head[currentstateid] = state_id;                                //拓展子节点链表的过程
	}
}

void select(int& nodeid, int& stateid) {//模拟过程中找下一步要下的位置

	if (head[currentstateid] == 0) {//没有拓展过就拓展
		Expand();
		nodeid = nodeidlist[head[currentstateid]];
		stateid = -1 * head[currentstateid];                //选定拓展后子节点链表的第一个节点预备进行rollout
		for (int i = head[currentstateid]; i != 0; i = nxt[i])
		{
			int x, y;
			decodeid(nodeid, x, y);
			if (judgeourturn % 2 == 0) {

				if (inboard(x+2, y-1) && inboard(x + 1, y ) && inboard(x + 1, y -1)&&nowstate.board[x + 1][y - 1] == 0 && nowstate.board[x + 1][y] == 0 && nowstate.board[x + 2][y -1] == nowstate.currentplayer)
				{
					nodeid = getid(x+2, y-1);
					stateid = -i;
					return;
				}
				if (inboard(x + 1, y + 1) && inboard(x + 1, y) && inboard(x, y+1) && nowstate.board[x + 1][y] == 0 && nowstate.board[x][y+1] == 0 && nowstate.board[x + 1][y + 1] == nowstate.currentplayer)
				{
					nodeid = getid(x + 1, y + 1);
					stateid = -i;
					return;
				}
				if (inboard(x + 1, y - 2) && inboard(x , y-1) && inboard(x + 1, y - 1) && nowstate.board[x + 1][y - 1] == 0 && nowstate.board[x][y-1] == 0 && nowstate.board[x + 1][y - 2] == nowstate.currentplayer)
				{
					nodeid = getid(x + 1, y - 2);
					stateid = -i;
					return;
				}

			}
			else{
				if (inboard(x + 1, y + 1) && inboard(x + 1, y) && inboard(x, y + 1) && nowstate.board[x + 1][y] == 0 && nowstate.board[x][y+1] == 0 && nowstate.board[x + 1][y + 1] == nowstate.currentplayer)
				{
					nodeid = getid(x + 1, y + 1);
					stateid = -i;
					return;
				}
				if (inboard(x - 1, y + 2) && inboard(x, y+1) && inboard(x-1, y + 1) && nowstate.board[x - 1][y+1] == 0 && nowstate.board[x][y + 1] == 0 && nowstate.board[x - 1][y + 2] == nowstate.currentplayer)
				{
					nodeid = getid(x - 1, y + 2);
					stateid = -i;
					return;
				}
				if (inboard(x + 2, y - 1) && inboard(x+1, y - 1) && inboard(x + 1, y) && nowstate.board[x + 1][y - 1] == 0 && nowstate.board[x+1][y] == 0 && nowstate.board[x + 2][y - 1] == nowstate.currentplayer)
				{
					nodeid = getid(x + 2, y - 1);
					stateid = -i;
					return;
				}
			}
		}
		return;
	}
	//如果拓展过， 就找一个UCB最大的
	int maxnodeid = 0;
	double maxUCB = -1e20;
	int maxstateid = 0;
	for (int i = head[currentstateid]; i != 0; i = nxt[i]) {
		if (!vistime[i]) {
			nodeid = nodeidlist[i];
			stateid = i;
			return;
		}                        //拓展了但是还没rollout过
		double tmp = (double)wintime[i] / vistime[i] + C * sqrt(log(vistime[currentstateid]) / vistime[i]);
		if (tmp > maxUCB) {
			maxUCB = tmp;
			maxnodeid = nodeidlist[i];
			maxstateid = i;
		}
	}
	nodeid = maxnodeid;
	stateid = maxstateid;                        //拓展了，也都rollout过了，就选UCB最大的再次rollout
}

void Stimulate(vector<int>& vis)//模拟过程
{
	nowstate = input;
	currentstateid = 1;
	while (!nowstate.gameover())                //不断向下找叶节点
	{
		int newnodeid = 0;
		int newstateid = 0;
		select(newnodeid, newstateid);
		nowstate.putapiece(newnodeid / 11, newnodeid % 11);
		if (newstateid < 0) break;                        //直到遇到叶子结点结束        
		vis.push_back(newstateid);                        //记录从根节点向下的路径，便于回滚
		currentstateid = newstateid;
	}
}

void BackPropagation(const vector<int>& vis) {                                //回滚
	Hex_ChessBoard cur = nowstate;                //记录现场
	int R[3] = { 2,3,4 };
	for (int i = 0; i < 11; i++) {
		for (int j = 0; j < 11; j++) {
			if (cur.board[i][j] != 0) {
				bridgeoperation(cur, i, j, R, 3);
			}
		}
	}
	for (int i = 0; i < SIMULATENUM; i++)
	{
		nowstate = cur;                                        //保存现场
		rollout();
		int winner = nowstate.redwin() ? 1 : -1;
		int flag = input.currentplayer;
		for (int j : vis)
		{
			vistime[j]++;
			if (flag != winner) wintime[j]++;
			flag = -flag;
		}
	}
}

int getBestmove()
{
	int maxwin = -1;
	int maxid = 0;
	for (int i = head[1]; i != 0; i = nxt[i])                        //??
	{
		if (wintime[i] > maxwin) {
			maxwin = wintime[i];
			maxid = nodeidlist[i];
		}
	}
	return maxid;
}

int starttime;
int run()
{
	starttime=clock();
    while ((double)(clock() - starttime) / CLOCKS_PER_SEC < 0.97 && stateCount <= MAXSTATENUM - 130) {
		vector<int> vis(1, 1);
		Stimulate(vis);
		BackPropagation(vis);
#ifndef _BOTZONE_ONLINE
		Stimulate_Num++;
#endif
	}
	return getBestmove();
}

const int specialstep[] = {
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
-1,-1,-1,80,40,45,80,48,50,45,-1,
-1,35,73,34,40,40,73,73,45,-1,-1,
-1,73,80,80,50,80,61,73,80,45,45,
-1,80,80,45,45,50,42,80,80,50,58,
40,70,45,59,45,80,80,80,80,72,80,
84,85,51,45,45,70,80,80,80,51,80,
59,80,80,51,45,80,70,70,80,72,60,
50,59,45,69,80,80,80,70,70,80,72,
42,71,100,80,80,70,96,70,70,80,40,
-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
};


int Make_Decision(int count, bool WeFirst)
{

	if (count == 0 && WeFirst)        return 1 * 11 + 2;                //开局且我们先手，第一步下在(1,2)
	MAXDist = count <= 6 ? 3 : 2;
	if (count == 1)        return 7 * 11 + 3;
	if (count == 2)
	{
		int tmp = 0;
		for (int i = 0; i < 11; i++)
			for (int j = 0; j < 11; j++)
			{
				if (i == 1 && j == 2)        continue;
				if (input.board[i][j] != 0)        tmp = 11 * i + j;
			}
		if (specialstep[tmp] == -1)
		{
			MAXDist = 20;
			return run();
		}
		else
			return specialstep[tmp];
	}
	if (count == 3)
	{
		return maxflow(2);	
	}

	return run();
}

void initial()
{
    memset(dis, 0, sizeof(dis));
    memset(id, 0, sizeof(id));
    memset(nodeidlist, 0, sizeof(nodeidlist));
    memset(head, 0, sizeof(head));    
    memset(nxt, 0, sizeof(nxt));
    memset(vistime, 0, sizeof(vistime));
    memset(wintime, 0, sizeof(wintime));  
    stateCount=1;
    Stimulate_Num=0; 
    memset(st, 0, sizeof(st));
    top = 0;
}

int main()
{
    ios::sync_with_stdio(0);
    cin.tie(0);
    cout.tie(0);
    int x, y, n, count = 0;
    //恢复目前的棋盘信息
    cin >> n;
    bool WeFirst = 0;
    for (int i = 0; i < n - 1; i++) {
        cin >> x >> y;
        if (x != -1) input.putapiece(x, y), count++;        //对方
        else WeFirst = true;                                        //对方不是先手
        cin >> x >> y;
        if (x != -1) input.putapiece(x, y), count++;        //我方
    }
    cin >> x >> y;
    if (x != -1) input.putapiece(x, y), count++;        //对方
    else        WeFirst = true;

    judgeourturn = count;

    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < 11; j++) {
            allbox.push_back(make_pair(i, j));
        }
    }
    int new_x;
    int new_y;
    Hex_ChessBoard tmp;
    tmp=input;
    for(int time=0;time<=100;time++)  
    {
        input=tmp;
        initial();
        C=(double)(time)*0.01;
        int tmp = Make_Decision(count, WeFirst);
        decodeid(tmp, new_x, new_y);
        //cout << new_x << ' ' << new_y << '\n';
    #ifndef _BOTZONE_ONLINE
        cout << Stimulate_Num<<endl;
    #endif
    }
    return 0;
}