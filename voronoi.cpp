#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <set>
#include <cmath>
#include <memory>
#include "vulkan_engine.h"
#include "perlin_noise_2d.h"

const double EPS = 1e-10;

int fuzzyCompare(double a, double b) {
	if (abs(a - b) < EPS)
		return 0;
	return a < b ? -1 : 1;
}

class Point {
    public:
	    double x, y;
		int value;

        Point(double x, double y, int value = 0) : x(x), y(y), value(value) {}
        
		double distSqr(const Point& p) {
            return (x - p.x) * (x - p.x) + (y - p.y) * (y - p.y);
        }
        static int orientation(Point* a, Point* b, Point* c) {
            double s = (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);
            return s < -EPS ? -1 : (s > EPS ? 1 : 0); // -1 правее a b, 0 на линии, 1 левее a b
        }

		bool fuzzyEquals(Point* other) {
        	return other != nullptr && fuzzyCompare(x, other->x) == 0 && fuzzyCompare(y, other->y) == 0;
		}
};

class HalfEdge;

class Cell : public Point {
	public:
		HalfEdge* head = nullptr;

		Cell(double x, double y, int value = 0) : Point(x, y, value) {}
};

class Line {
	public:
		double a, b, c;
		
		Line(double a, double b, double c) : a(a), b(b), c(c) {}
		
		Line(double x1, double y1, double x2, double y2) : a(y2 - y1), b(x1 - x2), c(-a * x1 - b * y1) {}
		
		bool isParallel(const Line& line) {
			return fuzzyCompare(a * line.b, line.a * b) == 0;
		}
		
		bool isEqual(const Line& line) {
			return fuzzyCompare(a * line.b, line.a * b) == 0 
				&& fuzzyCompare(a * line.c, line.a * c) == 0
				&& fuzzyCompare(b * line.c, line.b * c) == 0;
		}
		
		Point* intersection(const Line& line) {
			if (isParallel(line) || isEqual(line)) {
				return nullptr;
			}
			double px = (line.b * c - b * line.c) / (line.a * b - a * line.b);
        	double py = fuzzyCompare(b, 0) != 0 ? (-c - a * px) / b : (-line.c - line.a * px) / line.b;
			return new Point(px, py);
		}
		
		static Line perpendicular(const Point& p1, const Point& p2, const Point& p) {
			double a = p2.y - p1.y, b = p1.x - p2.x;
			return Line(b, -a, -b * p.x + a * p.y);
		}
};

class HalfEdge {
	public:
		Cell* cell;
		HalfEdge* next = nullptr;
		HalfEdge* prev;
		HalfEdge* twin;

		HalfEdge(Point* source, Cell* cell) : source(source), cell(cell) {}

		Point* getStart() {
        	return source->value == 0 ? source : nullptr;
    	}

    	Point* getEnd() {
        	return twin->source->value == 0 ? twin->source : nullptr;
    	}

		void setStart(Point* p) {
			if (twin->source->value == -2) {
				twin->source->x = -source->x;
				twin->source->y = -source->y;
				twin->source->value = -1;
			}
			source = p;
		}

		void setEnd(Point* p) {
			if (source->value == -2) {
				source->x = -twin->source->x;
				source->y = -twin->source->y;
				source->value = -1;
			}
			twin->source = p;
		}

		Line getLine() {
			return Line(source->x, source->y, twin->source->x, twin->source->y);
		}

		bool onEdge(const Point& p) {
			if (source->value >= 0 && twin->source->value >= 0) {
				return fuzzyCompare(p.x, std::min(source->x, twin->source->x)) >= 0 
					&& fuzzyCompare(p.x, std::max(source->x, twin->source->x)) <= 0
					&& fuzzyCompare(p.y, std::min(source->y, twin->source->y)) >= 0
					&& fuzzyCompare(p.y, std::max(source->y, twin->source->y)) <= 0;
			}
			if (source->value < 0 && twin->source->value < 0) {
				return true;
			}
			if (source->value < 0) {
				int da = fuzzyCompare(twin->source->x - p.x, 0), db = fuzzyCompare(twin->source->y - p.y, 0);
				return (da == 0 && db == 0) || (da == fuzzyCompare(source->x, 0) && db == fuzzyCompare(source->y, 0));
			} else {
				int da = fuzzyCompare(source->x - p.x, 0), db = fuzzyCompare(source->y - p.y, 0);
				return (da == 0 && db == 0) || (da == fuzzyCompare(twin->source->x, 0) && db == fuzzyCompare(twin->source->y, 0));
			}
		}

	private:
		
		Point* source;
		
};

HalfEdge* createEdge(Point* p1, Point* p2, const Line& l, Cell* left, Cell* right) {
	double b = (l.a > 0 && l.b > 0) || (l.a < 0 && l.b < 0) || fuzzyCompare(l.a, 0) == 0 ? std::abs(l.b): -std::abs(l.b); //? 4 : 3;
	if (p1 == nullptr) {
		p1 = new Point(-b, abs(l.a), -1);
		if (p2 == nullptr) {
			p2 = new Point((left->x + right->x) / 2, (left->y + right->y) / 2, -2);
		}
	} else if (p2 == nullptr) {
		p2 = new Point(b, -std::abs(l.a), -1);
	}
	auto leftEdge = new HalfEdge(p1, left);
	auto rightEdge = new HalfEdge(p2, right);
	leftEdge->twin = rightEdge;
	rightEdge->twin = leftEdge;
	leftEdge->prev = leftEdge->next = leftEdge;
	rightEdge->prev = rightEdge->next = rightEdge;
	return leftEdge;
}

class HalfEdgePtr {
	
	public:
		Cell* cell;
		Point* cp = nullptr;
		HalfEdge* top = nullptr;
		HalfEdge* edge;
		bool headSkipped = false;

		HalfEdgePtr(Cell* cell, bool clockwise) : cell(cell), clockwise(clockwise), edge(cell->head) {}
		
		void set(HalfEdge* newEdge) {
			cell = newEdge->cell;
			top = edge = newEdge;
			cp = nullptr;
			headSkipped = false;
		}

		void intersection(const Line& seam, Point* last) {
			if (edge != nullptr) {
				auto start = edge;
				do {
					auto p = edge->getLine().intersection(seam);
					if (p != nullptr) {
						int cmpY = last == nullptr ? -1 : fuzzyCompare(p->y, last->y);
						if ((cmpY < 0 || (cmpY == 0 && fuzzyCompare(p->x, last->x) > 0)) && edge->onEdge(*p)) {
							int eq = p->fuzzyEquals(edge->getStart()) ? -1 : (p->fuzzyEquals(edge->getEnd()) ? 1 : 0);
                            if (eq == 0) {
                                cp = p;
                            } else if (eq == -1) {
                                cp = edge->getStart();
								delete p;
                                if (clockwise) move();
                            } else {
                                cp = edge->getEnd();
								delete p;
                                if (!clockwise) move();
                            }
                            return;
						} else {
							delete p;
						}
					}
					move();
				} while (edge != start);
			}
		}
	private:
		const bool clockwise;

		void move() {
            if (clockwise) {
                headSkipped = headSkipped || edge == cell->head;
                edge = edge->prev;
            } else {
                edge = edge->next;
                headSkipped = headSkipped || edge == cell->head;
            }
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


void connectChain(HalfEdge* first, HalfEdge* chainStart, HalfEdge* second, bool headSkipped) {
	Cell* cell = chainStart->cell;
	auto chainEnd = chainStart->prev;
	auto headPrev = cell->head->prev;


	if (first != nullptr && second != nullptr) {
		if (cell->head != headPrev && cell->head->next == headPrev
				&& headPrev->next == cell->head && cell->head->getLine().isParallel(headPrev->getLine())) {
			if (cell->head->getStart() != nullptr) {
				cell->head = headPrev;
			}
			headSkipped = false;
		}
		first->next = chainStart;
		chainStart->prev = first;
		second->prev = chainEnd;
		chainEnd->next = second;
		if (headSkipped) {
			cell->head = chainStart;
		}
	} else if (first == nullptr && second == nullptr) {
		if (cell->head != nullptr) {
			headPrev = cell->head->next = chainStart;
			chainStart->prev = chainStart->next = cell->head;
		}
		cell->head = chainStart;
	} else if (first == nullptr) {
		headPrev->next = chainStart;
		chainStart->prev = headPrev;
		second->prev = chainEnd;
		chainEnd->next = second;
		cell->head = chainStart;
	} else {
		first->next = chainStart;
		chainStart->prev = first;
		headPrev = chainEnd;
		chainEnd->next = cell->head;
	}
}

HalfEdge* addChainLink(HalfEdge* edge, HalfEdge* head, bool inHead) {
    if (head == nullptr) {
		edge->prev = edge->next = edge;
		return edge;
	}
	edge->next = head;
	edge->prev = head->prev;
	head->prev = head->prev->next = edge;
	return inHead ? edge : head;
}

void mergeVoronoi(const std::pair< Point*, Point* >& bridge) {
	HalfEdgePtr left = HalfEdgePtr(static_cast< Cell* >(bridge.second), true);
	HalfEdgePtr right = HalfEdgePtr(static_cast< Cell* >(bridge.first), false);
	Point mid{ (left.cell->x + right.cell->x) / 2, (left.cell->y + right.cell->y) / 2};
	Point* lastP = nullptr;
	HalfEdge* leftChain = nullptr;
	HalfEdge* rightChain = nullptr;
	while (true) {
		Line seam = Line::perpendicular(*left.cell, *right.cell, mid);
		left.intersection(seam, lastP);
		right.intersection(seam, lastP);
		if (left.cp == nullptr && right.cp == nullptr) {
			auto edge = createEdge(nullptr, lastP, seam, left.cell, right.cell);
        	leftChain = addChainLink(edge, leftChain, true);
            rightChain = addChainLink(edge->twin, rightChain, false);
            connectChain(nullptr, leftChain, left.top, left.headSkipped);
            connectChain(right.top, rightChain, nullptr, right.headSkipped);
			break;
		}
		int cmp = left.cp == nullptr ? 1 : (right.cp == nullptr ? -1 : fuzzyCompare(right.cp->y, left.cp->y));
		Point* point = cmp <= 0 ? left.cp : right.cp;

		auto edge = createEdge(point, lastP, seam, left.cell, right.cell);
		leftChain = addChainLink(edge, leftChain, true);
		rightChain = addChainLink(edge->twin, rightChain, false);
		mid = Point(point->x, point->y);
		lastP = point;
		if (cmp <= 0) {
			auto intersectTwin = point->fuzzyEquals(left.edge->getEnd()) ? left.edge->next->twin->next : left.edge->twin;
			left.edge->setEnd(point);
			intersectTwin->setStart(point);
			connectChain(left.edge, leftChain, left.top, left.headSkipped);
			left.set(intersectTwin);
			leftChain = nullptr;
		}
		if (cmp >= 0) {
			auto intersectTwin = right.edge->twin;
			if (point->fuzzyEquals(right.edge->getStart())) {
				while (right.edge->prev->twin->prev != intersectTwin) {
					intersectTwin->setEnd(point);
					intersectTwin = intersectTwin->next->twin;
				}
			}
			right.edge->setStart(point);
			intersectTwin->setEnd(point);
			connectChain(right.top, rightChain, right.edge, right.headSkipped);
			right.set(intersectTwin);
			rightChain = nullptr;
		}
	}
}

PolyNode* voronoi(const std::vector< Cell* >& cells, size_t begin, size_t end) {
	if (end - begin == 1) {
		return PolyNode::makeNode(cells[begin]);
	}
	size_t mid = (begin + end) / 2;
	auto left = voronoi(cells, begin, mid);
	auto right = voronoi(cells, mid, end);
	auto merged = merge(left, right);
	mergeVoronoi(merged.second);
	return merged.first;
}

int main(int argc, char** argv)
{
	// 1683966317
	// PerlinNoise2D::generateImage(512, 512, 80, 1683966317);

	// int n;
	// std::set < std::pair < int, int > > v;
	// std::vector < Cell* > cells;
	// int x, y;

	// std::cin >> n;
	// for (int i = 0; i < n; ++i) {
	// 	std::cin >> x >> y;
	// 	v.emplace(x, y);
	// }

	// for (const auto& it : v) {
	// 	cells.emplace_back(new Cell(it.first, it.second));
	// }

	// auto head = voronoi(cells, 0, cells.size());
	
	// VulkanEngine vulkanEngine;

	// std::vector< Vertex > vrtx;

	// Vertex a = { { (head->p->x - 3.0) / 5.0, (head->p->y - 3.0) / 5.0 }, { static_cast< float >(rand()) / RAND_MAX, static_cast< float >(rand()) / RAND_MAX, static_cast< float >(rand()) / RAND_MAX } };
	// auto curr = head->next;
	// while (curr->next != head) {
	// 	Vertex b = { { (curr->p->x - 3.0) / 5.0, (curr->p->y - 3.0) / 5.0 }, { static_cast< float >(rand()) / RAND_MAX, static_cast< float >(rand()) / RAND_MAX, static_cast< float >(rand()) / RAND_MAX } };
	// 	Vertex c = { { (curr->next->p->x - 3.0) / 5.0, (curr->next->p->y - 3.0) / 5.0 }, { static_cast< float >(rand()) / RAND_MAX, static_cast< float >(rand()) / RAND_MAX, static_cast< float >(rand()) / RAND_MAX } };
	// 	vrtx.emplace_back(a);
	// 	vrtx.emplace_back(b);
	// 	vrtx.emplace_back(c);
	// 	curr = curr->next;
	// }

	// vulkanEngine.v = vrtx;

	// for (size_t i = 0; i < vrtx.size(); i += 3) {
	// 	std::cout << "a: " << vrtx[i].pos.x << ' ' << vrtx[i].pos.y << std::endl;
	// 	std::cout << "b: " << vrtx[i + 1].pos.x << ' ' << vrtx[i + 1].pos.y << std::endl;
	// 	std::cout << "c: " << vrtx[i + 2].pos.x << ' ' << vrtx[i + 2].pos.y << std::endl;
	// }

    // try {
    //     vulkanEngine.run();
    // } catch (const std::exception& e) {
    //     std::cerr << e.what() << std::endl;
    //     return EXIT_FAILURE;
    // }

	return 0;
}