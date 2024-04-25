#define PROGRAM_NAME "NoGo_Chess"
#define VERSION      "1.0.20.1017"

/* This File is part of $PROGRAM_NAME
/  $PROGRAM_NAME is free software: you can redistribute it and/or modify
/  it under the terms of the GNU Lesser General Public License as published by
/  the Free Software Foundation, either version 3 of the License, or
/  (at your option) any later version.
/
/  $PROGRAM_NAME is distributed in the hope that it will be useful,
/  but WITHOUT ANY WARRANTY; without even the implied warranty of
/  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/  GNU Lesser General Public License for more details.
/
/  You should have received a copy of the GNU Lesser General Public License
/  along with $PROGRAM_NAME at /LICENSE.
/  If not, see <http://www.gnu.org/licenses/>. */


#include <deque>
#include <cmath>
#include <cstring>
#include <time.h>
#include <iostream>
#include <mmintrin.h>
#include "jsoncpp/json.h"
using namespace std;

#define PER_PACK 16
#define TIME_TOTAL 950

struct ivec2 {
	union {
		__m64 xy;
		struct {
			int x, y;
		};
	};
	ivec2(int _x, int _y) {
		x = _x;
		y = _y;
	}
	ivec2(int _x) {
		x = _x;
		y = _x;
	}
	ivec2(__m64 _xy) {
		xy = _xy;
	}
	ivec2() {
		x = 0;
		y = 0;
	}
	ivec2 operator+(const ivec2& n) {
		_m_empty();
		__m64 buf = _m_paddd(xy, n.xy);
		return buf;
	}
	ivec2 operator-(const ivec2& n) {
		_m_empty();
		__m64 buf = _m_psubd(xy, n.xy);
		return buf;
	}
};
struct ivec3 {
	union {
		__m64 xy;
		struct {
			int x, y;
		};
	};
	double z;
	ivec3(int _x, int _y, double _z) {
		x = _x;
		y = _y;
		z = _z;
	}
	ivec3(ivec2 _xy, double _z) {
		xy = _xy.xy;
		z = _z;
	}
	ivec3() {
		x = 0;
		y = 0;
		z = 0;
	}
	ivec3 operator+(const ivec3& n) {
		_m_empty();
		__m64 buf = _m_paddd(xy, n.xy);
		return ivec3(buf, z + n.z);
	}
	ivec3 operator-(const ivec3& n) {
		_m_empty();
		__m64 buf = _m_psubd(xy, n.xy);
		return ivec3(buf, z - n.z);
	}
};

bool player = true;
const int offset_x[] = { -1, 0, 1, 0 };
const int offset_y[] = { 0,-1, 0, 1 };
#define inBorder(x, y) x >= 0 && y >= 0 && x < 9 && y < 9
bool checkEmpty(ivec2 location, int* board_map, int* visit_map, bool flags) {
	if (inBorder(location.x, location.y)) {
		int pos = 9 * location.x + location.y;
		if (visit_map[pos]) return false;
		if (board_map[pos] == -1) return true;
		if (board_map[pos] == flags) {
			visit_map[pos] = true;
			for (int i = 0; i < 4; ++i)
				if (checkEmpty(location + ivec2(offset_x[i], offset_y[i]), board_map, visit_map, flags)) return true;
			return false;
		}
		else return false;
	}
	else return false;
}
bool checkAvailable(ivec2 location, int* board_map, bool flags) {
	if (board_map[9 * location.x + location.y] != -1) return false;
	int visit_map[81] = {};
	int empty = 0, enemy = 0;
	for (int i = 0; i < 4; ++i) {
		ivec2 pos = location + ivec2(offset_x[i], offset_y[i]);
		if (inBorder(pos.x, pos.y)) {
			if (board_map[9 * pos.x + pos.y] == -1) ++empty;
			if (board_map[9 * pos.x + pos.y] == !flags) ++enemy;
		}
		else ++empty, ++enemy;
	}
	if (empty == 4) return true;
	if (enemy == 4) return false;
	bool isEmpty = true;
	board_map[9 * location.x + location.y] = flags;
	for (int i = 0; i < 4; ++i) {
		ivec2 pos = location + ivec2(offset_x[i], offset_y[i]);
		if (inBorder(pos.x, pos.y)) {
			int status = board_map[9 * pos.x + pos.y];
			if (status != -1) {
				memset(visit_map, 0, sizeof(visit_map));
				isEmpty = isEmpty && checkEmpty(pos, board_map, visit_map, status);
			}
		}
	}
	board_map[9 * location.x + location.y] = -1;
	return isEmpty;
}

//Subpixel Morphological Estimate
//a
const int test_x0[] = { 2, 1, 1 ,0, 1,-1,-2,-1,-1, 0, 1,-1 };
const int test_y0[] = { 0, 1,-1 ,2, 1, 1, 0,-1, 1,-2,-1,-1 };
//b
const int test_x1[] = { 1, 0,-1, 0 };
const int test_y1[] = { 0, 1, 0,-1 };
//c
const int test_x2[] = { 1,-1, 1,-1 };
const int test_y2[] = { 1,-1,-1, 1 };

float radicalInverse(unsigned int bits) {
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555) << 1u) | ((bits & 0xAAAAAAAA) >> 1u);
	bits = ((bits & 0x33333333) << 2u) | ((bits & 0xCCCCCCCC) >> 2u);
	bits = ((bits & 0x0F0F0F0F) << 4u) | ((bits & 0xF0F0F0F0) >> 4u);
	bits = ((bits & 0x00FF00FF) << 8u) | ((bits & 0xFF00FF00) >> 8u);
	return float(bits) * 2.328306437e-10f;
}
void estimateValue(deque<ivec3>& available, int* board_map, bool flags) {
	for (int i = 0; i < available.size(); ++i) {
		double a = 0, b = 0, c = 0;
		// e.x.    X            O
		//       0   X   or   O   O  etc.
		//         X            O
		for (int j = 0; j < 4; ++j) { //Pass.1 a:catalyst
			int hita = 0, hitb = 0;
			for (int k = 0; k < 3; ++k) {
				ivec2 pos = ivec2(available[i].x + test_x0[j * 3 + k], available[i].y + test_y0[j * 3 + k]);
				if (inBorder(pos.x, pos.y)) {
					if (board_map[9 * pos.x + pos.y] == flags) ++hita;
					if (board_map[9 * pos.x + pos.y] == !flags) ++hitb;
				}
				else {
					++hita; ++hitb;
				}
			}
			ivec2 pos = ivec2(available[i].x + test_x1[j], available[i].y + test_y1[j]);
			if (inBorder(pos.x, pos.y)) if (board_map[9 * pos.x + pos.y] == -1) {
				if (hita == 3) ++a;
				if (hitb == 3) a += 0.35f;
			}
		}
		// e.x.    X            X
		//         O X   or   X O X  etc.
		//
		int enemy = 0, hit = 0;
		for (int j = 0; j < 4; ++j) { //Pass.2 b:inhibitor
			ivec2 pos = ivec2(test_x1[j] + available[i].x, test_y1[j] + available[i].y);
			if (inBorder(pos.x, pos.y)) {
				if (board_map[9 * pos.x + pos.y] == flags) ++hit;
				if (board_map[9 * pos.x + pos.y] == !flags) ++enemy;
			}
		}
		if (hit > 2) {
			available[i].z = -2.0f;
			continue;
		}
		if (enemy > 1 && hit == 0) b = --enemy;
		// e.x.  X   X        O   O
		//         X     or     X    etc.
		//       X   X        O   O
		for (int j = 0; j < 4; ++j) { //Pass.3 c:initiator
			ivec2 pos = ivec2(test_x2[j] + available[i].x, test_y2[j] + available[i].y);
			if (inBorder(pos.x, pos.y)) if (board_map[9 * pos.x + pos.y] != -1) ++c;
		}
		//
		//magic algorithm mixture value
		available[i].z = tanh(sqrt(a * 0.50f)) * 2.20f + pow(b * 0.35f, 0.90f) * 2.15f + c * 0.30f - 0.35f;
		//scale factor
		available[i].z *= 0.65f;
	}
}
int statisticValue(deque<ivec3>& available, int* board_map, bool flags) {
	int stat = 0;
	for (int i = 0; i < available.size(); i++) {
		int strong = 0;
		for (int j = 0; j < 4; j++) {
			ivec2 pos = ivec2(available[i].x, available[i].y) + ivec2(offset_x[j], offset_y[j]);
			if (inBorder(pos.x, pos.y)) {
				if (board_map[9 * pos.x + pos.y] == flags) ++strong;
			}
			else ++strong;
		}
		if (strong == 4) ++stat;
		++stat; ++stat;
	}
	return stat;
}
class MCTS {
private:
	struct TreeNode {
		//state
		int n = 0;
		int* N = NULL;
		double Q = 0.0f;
		//board
		int action = 0;
		int* board = nullptr;
		bool flags = false;
		deque<ivec3> available;
		//tree
		TreeNode* parent;
		deque<TreeNode*> children;
		//child node
		TreeNode(TreeNode* _parent, int* _board, double _Q, int _action, bool _flags) {
			//init native variables
			parent = _parent;
			Q = _Q;
			N = parent->N;
			board = _board;
			flags = _flags;
			action = _action;
			//do action
			board[action] = flags;
			//get available set
			for (int s = 0; s < 9; ++s) for (int t = 0; t < 9; ++t)
				if (checkAvailable(ivec2(s, t), board, !flags)) available.push_back(ivec3(s, t, 0));
			//estimate native board
			estimateValue(available, board, !flags);
		}
		//root node
		TreeNode(int _N) {
			//init native variables
			N = new int(_N);
			flags = player;
			parent = nullptr;
		}
	};
	static TreeNode* createMCTSTree(deque<ivec3>& available, int* board_map) {
		TreeNode* root = new TreeNode(available.size());
		estimateValue(available, board_map, player);
		for (int i = 0; i < available.size(); i++) {
			int* child_board = new int[81]();
			memcpy(child_board, board_map, 81 * sizeof(int));
			TreeNode* root_child = new TreeNode(root, child_board,
				available[i].z, 9 * available[i].x + available[i].y, player);
			root->children.push_back(root_child);
		}
		return root;
	}
	static TreeNode* selectChild(TreeNode* node) {
		int maxNode = 0;
		double maxScore = -32768;
		for (int i = 0; i < node->children.size(); i++) {
			double Score = node->children[i]->Q / double(node->children[i]->n)
				+ 1.65f * sqrt(log((double)*(node->N)) / double(node->children[i]->n));
			if (Score > maxScore) {
				maxScore = Score;
				maxNode = i;
			}
		}
		return node->children[maxNode];
	}
	static void expandTreeTop(TreeNode* node) {
		for (int i = 0; i < node->available.size(); i++) {
			int* child_board = new int[81]();
			memcpy(child_board, node->board, 81 * sizeof(int));
			TreeNode* new_child = new TreeNode(node, child_board,
				node->available[i].z, 9 * node->available[i].x + node->available[i].y, !node->flags);
			node->children.push_back(new_child);
		}
	}
	static TreeNode* treePolicy(TreeNode* root) {
		//select the treetop
		TreeNode* treetop = root;
		while (!treetop->children.empty()) treetop = selectChild(treetop);
		//expand treetop, create new child treenodes
		expandTreeTop(treetop);
		return treetop;
	}
	static double defaultPolicy(TreeNode* node) {
		deque<ivec3> available_opposite;
		for (int s = 0; s < 9; ++s) for (int t = 0; t < 9; ++t)
			if (checkAvailable(ivec2(s, t), node->board, node->flags)) available_opposite.push_back(ivec3(s, t, 0));
		if (node->available.empty()) return 2.0;
		if (available_opposite.empty()) return -2.0;
		double strong = statisticValue(node->available, node->board, !node->flags);
		double strong_opposite = statisticValue(available_opposite, node->board, node->flags);
		return tanh((strong_opposite - strong) / (strong + strong_opposite));
	}
	static void backupValue(TreeNode* node, double delta) {
		for (int i = 0; node != nullptr; i++) {
			node->Q += i % 2 == 0 ? delta : -delta;
			++node->n;
			node = node->parent;
		}
	}
	static void searchCycle(TreeNode* root) {
		++(*(root->N));
		TreeNode* treetop = treePolicy(root);
		double reward = defaultPolicy(treetop);
		backupValue(treetop, reward);
	}
public:
	ivec2 getStep(int* board_map, int step) {
		deque<ivec3> available;
		for (int s = 0; s < 9; ++s) for (int t = 0; t < 9; ++t)
			if (checkAvailable(ivec2(s, t), board_map, player)) available.push_back(ivec3(s, t, 0));
		if (available.size() == 0) return ivec2(0);
		if (step == 0) {
			srand(time(NULL));
			int i = rand() % available.size();
			return ivec2(available[i].x, available[i].y);
		}
		TreeNode* root = createMCTSTree(available, board_map);
		int end = (int)(TIME_TOTAL)+clock();
		while (clock() < end) for (int i = 0; i < PER_PACK; ++i) searchCycle(root);
		int maxn = 0, maxStep = 0;
		for (int i = 0; i < root->children.size(); i++)
			if (root->children[i]->n > maxn) {
				maxn = root->children[i]->n;
				maxStep = root->children[i]->action;
			}
		//cout << root->Q;
		return ivec2(maxStep / 9, maxStep % 9);
	}
}Bot;
int main() {

	string str;
	getline(cin, str);

	Json::Value root;
	Json::Reader reader;
	reader.parse(str, root);

	int board_map[81] = {};
	memset(board_map, -1, sizeof(board_map));
	const char* frase[2] = { "requests" ,"responses" };

	int num = 0;
	for (int j = 0; j < 2; j++)
		for (int i = 0; i < root[frase[j]].size(); i++) {
			int x = root[frase[j]][i]["x"].asInt();
			int y = root[frase[j]][i]["y"].asInt();
			if (x != -1) {
				board_map[9 * x + y] = j;
				num++;
			}
		}

	ivec2 result = Bot.getStep(board_map, num);

	Json::Value ret;
	Json::Value action;
	Json::FastWriter writer;

	action["x"] = result.x; action["y"] = result.y;
	ret["response"] = action;

	cout << writer.write(ret) << endl;
	return 0;

}