#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <set>
#include <cmath>
#include "vulkan_engine.h"

const double EPS = 1e-10;

int fuzzyCompare(double a, double b) {
	if (abs(a - b) < EPS)
		return 0;
	return a < b ? -1 : 1;
}

class Point {
    public:
        const double x, y;
        Point(double x, double y) : x(x), y(y) {}
        double distSqr(const Point& p) {
            return (x - p.x) * (x - p.x) + (y - p.y) * (y - p.y);
        }
        static int orientation(Point* a, Point* b, Point* c) {
            double s = (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);
            return s < -EPS ? -1 : (s > EPS ? 1 : 0); // -1 правее a b, 0 на линии, 1 левее a b
        }
};

class PolyNode {
    public:
        Point* p;
        PolyNode* next = nullptr;
        PolyNode* prev = nullptr;

        static PolyNode* makeNode(Point* p) {
            auto node = new PolyNode(p);
            node->next = node->prev = node;
            return node;
        }

    private:
        PolyNode(Point* p) : p(p) {}
};

std::pair< PolyNode*, PolyNode* > findRightChain(Point* p, PolyNode* right) {
	if (right == right->next)
		return std::make_pair(right, right);

	if (right->next == right->prev) {
		return Point::orientation(p, right->p, right->next->p) < 0
			? std::make_pair(right->next, right)
			: std::make_pair(right, right->next);
	}

	PolyNode* current = right;
	std::pair< PolyNode*, PolyNode* > chain{ nullptr, nullptr };
	while (chain.first == nullptr || chain.second == nullptr) {
		int prev_orient = Point::orientation(p, current->p, current->prev->p);
		int next_orient = Point::orientation(p, current->p, current->next->p);

		if (chain.first == nullptr && prev_orient > 0 && next_orient >= 0)
			chain.first = current;

		if (chain.second == nullptr && prev_orient <= 0 && next_orient < 0)
			chain.second = current;

        current = current->next;
	}

	return chain;
}

std::pair< PolyNode*, std::pair< Point*, Point* > > merge(PolyNode* left, PolyNode* right) {
	auto chain = findRightChain(left->p, right);
	PolyNode* l = left->next == left ? nullptr : left->next;
	PolyNode* r = chain.first;
	std::vector< PolyNode* > m;
	size_t low = 0, up = 0;
	m.emplace_back(left);
	while (l != nullptr || r != nullptr) {
		PolyNode* curr;
		bool rside = l == nullptr || (r != nullptr && Point::orientation(left->p, l->p, r->p) < 0);
		if (rside) {
			curr = r;
			r = r == chain.second ? nullptr : r->next;
		} else {
			curr = l;
			l = l->next == left ? nullptr : l->next;
		}

		while (m.size() >= 2 && Point::orientation(m[m.size() - 2]->p, m[m.size() - 1]->p, curr->p) <= 0) {
			m.pop_back();
		}

		if (m[m.size() - 1]->next != curr) {
			if (rside) {
				low = m.size() - 1;
			} else {
				up = m.size() - 1;
			}
		}
        m.emplace_back(curr);
	}

	if (m.size() > 2 && Point::orientation(m[m.size() - 2]->p, m[m.size() - 1]->p, m[0]->p) <= 0) { // может быть только == 0
		m.pop_back();
	}
	if (m[m.size() - 1]->next != m[0]) {
		up = m.size() - 1;
	}

	size_t up2 = (up + 1) % m.size();
	std::pair< Point*, Point* > bridge = std::make_pair(m[up]->p, m[up2]->p);
	if (Point::orientation(m[up]->p, m[up]->next->p, m[up2]->p) == 0) {
		bridge.first = m[up]->next->p;
	}
	if (Point::orientation(m[up2]->p, m[up2]->prev->p, m[up]->next->p) == 0) {
		bridge.second = m[up2]->prev->p;
	}

	m[low]->next = m[low + 1];
	m[low + 1]->prev = m[low];
	m[up]->next = m[up2];
	m[up2]->prev = m[up];

	return std::make_pair(m[0], bridge); // left
}

PolyNode* kirkpatrick(const std::vector< Point* >& points, size_t begin, size_t end) {
	if (end - begin == 1) {
		return PolyNode::makeNode(points[begin]);
	}

	size_t mid = (begin + end) / 2;
	auto left = kirkpatrick(points, begin, mid);
	auto right = kirkpatrick(points, mid, end);

	return merge(left, right).first;
}

void mergeVoronoi(const std::pair< Point*, Point* >& bridge) {

}

PolyNode* voronoi(const std::vector< Point* >& points, size_t begin, size_t end) {
	if (end - begin == 1) {
		return PolyNode::makeNode(points[begin]);
	}

	size_t mid = (begin + end) / 2;
	auto left = voronoi(points, begin, mid);
	auto right = voronoi(points, mid, end);

	auto merged = merge(left, right);
	mergeVoronoi(merged.second);

	return merged.first;
}

int main(int argc, char** argv)
{
	int n;
	std::set < std::pair < int, int > > v;
	std::vector < Point* > points;
	int x, y;

	std::cin >> n;
	for (int i = 0; i < n; ++i) {
		std::cin >> x >> y;
		v.emplace(x, y);
	}

	for (const auto& it : v) {
		points.emplace_back(new Point(it.first, it.second));
	}

	auto head = kirkpatrick(points, 0, points.size());
	
	VulkanEngine vulkanEngine;

	std::vector< Vertex > vrtx;

	Vertex a = { { (head->p->x - 3.0) / 5.0, (head->p->y - 3.0) / 5.0 }, { static_cast< float >(rand()) / RAND_MAX, static_cast< float >(rand()) / RAND_MAX, static_cast< float >(rand()) / RAND_MAX } };
	auto curr = head->next;
	while (curr->next != head) {
		Vertex b = { { (curr->p->x - 3.0) / 5.0, (curr->p->y - 3.0) / 5.0 }, { static_cast< float >(rand()) / RAND_MAX, static_cast< float >(rand()) / RAND_MAX, static_cast< float >(rand()) / RAND_MAX } };
		Vertex c = { { (curr->next->p->x - 3.0) / 5.0, (curr->next->p->y - 3.0) / 5.0 }, { static_cast< float >(rand()) / RAND_MAX, static_cast< float >(rand()) / RAND_MAX, static_cast< float >(rand()) / RAND_MAX } };
		vrtx.emplace_back(a);
		vrtx.emplace_back(b);
		vrtx.emplace_back(c);
		curr = curr->next;
	}

	vulkanEngine.v = vrtx;

	for (size_t i = 0; i < vrtx.size(); i += 3) {
		std::cout << "a: " << vrtx[i].pos.x << ' ' << vrtx[i].pos.y << std::endl;
		std::cout << "b: " << vrtx[i + 1].pos.x << ' ' << vrtx[i + 1].pos.y << std::endl;
		std::cout << "c: " << vrtx[i + 2].pos.x << ' ' << vrtx[i + 2].pos.y << std::endl;
	}

    try {
        vulkanEngine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

	return 0;
}