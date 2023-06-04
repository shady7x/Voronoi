﻿#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <set>
#include <map>
#include <cmath>
#include <memory>
#include <limits>
#include "vulkan_engine.h"
#include "perlin_noise_2d.h"

const double EPS = 1e-10;

int fuzzyCompare(double val1, double val2) {
	double eps = (abs(val2) + 1) * EPS;
	double diff = val1 - val2;
	return diff < -eps ? -1 : (diff > eps ? 1 : 0);
}

class Point {
    public:
	    double x, y;
		int32_t value;

        Point(double x, double y, int32_t value = 0) : x(x), y(y), value(value) {}
        
		double distSqr(const Point& p) {
            return (x - p.x) * (x - p.x) + (y - p.y) * (y - p.y);
        }
        
		bool fuzzyEquals(const Point* other) {
        	return other != nullptr && fuzzyCompare(x, other->x) == 0 && fuzzyCompare(y, other->y) == 0;
		}

		static int orientation(Point* a, Point* b, Point* c) {
            double s = (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);
            return s < -EPS ? -1 : (s > EPS ? 1 : 0); // -1 правее(cw) a b, 0 на линии, 1 левее(ccw) a b
        }
};

class HalfEdge;

class Cell : public Point {
	public:
		HalfEdge* head = nullptr;

		Cell(double x, double y, int32_t value = 0) : Point(x, y, value) {}
};

class Line {
	public:
		const double a, b, c;
		
		Line(double a, double b, double c) : a(a), b(b), c(c) {}

		Line(const Point& p1, const Point& p2) : a(p2.y - p1.y), b(p1.x - p2.x), c(-a * p1.x - b * p1.y) {}
		
		Line(double x1, double y1, double x2, double y2) : a(y2 - y1), b(x1 - x2), c(-a * x1 - b * y1) {}
		
		bool isParallel(const Line& line) {
			return fuzzyCompare(a * line.b, line.a * b) == 0;
		}
		
		bool isEqual(const Line& line) {
			return fuzzyCompare(a * line.b, line.a * b) == 0 
				&& fuzzyCompare(a * line.c, line.a * c) == 0
				&& fuzzyCompare(b * line.c, line.b * c) == 0;
		}
		
		std::shared_ptr< Point > intersection(const Line& line) {
			if (isParallel(line) || isEqual(line)) {
				return nullptr;
			}
			double px = (line.b * c - b * line.c) / (line.a * b - a * line.b);
        	double py = fuzzyCompare(b, 0) != 0 ? (-c - a * px) / b : (-line.c - line.a * px) / line.b;
			return std::make_shared< Point >(px, py);
		}
		
		static Line perpendicular(const Point& p1, const Point& p2, const Point& p) {
			double a = p2.y - p1.y, b = p1.x - p2.x;
			return Line(b, -a, -b * p.x + a * p.y);
		}

		std::string toString() {
			return std::to_string(a) + " " + std::to_string(b) + " " + std::to_string(c);
		}
};

class HalfEdge {
	public:
		Cell* cell;
		HalfEdge* next = nullptr;
		HalfEdge* prev = nullptr;
		HalfEdge* twin;

		HalfEdge(std::shared_ptr< Point > source, Cell* cell) : source(source), cell(cell) {}

		std::shared_ptr< Point > getStart() {
        	return source->value == 0 ? source : nullptr;
    	}

    	std::shared_ptr< Point > getEnd() {
        	return twin->source->value == 0 ? twin->source : nullptr;
    	}

		void setStart(std::shared_ptr< Point > p) {
			if (twin->source->value < 0) {
				source->value = abs(twin->source->value);
				twin->source = source;
			}
			source = p;
		}

		void setEnd(std::shared_ptr< Point > p) {
			if (source->value < 0) {
				twin->source->value = abs(source->value);
				source = twin->source;
			}
			twin->source = p;
		}

		int getQuadrant() {
            if (source->value == 0 && twin->source->value == 0) {
                int dx = fuzzyCompare(twin->source->x, source->x);
                int dy = fuzzyCompare(twin->source->y, source->y);
                if (dx >= 0 && dy > 0) return 1;
                if (dx < 0 && dy >= 0) return 2;
                if (dx <= 0 && dy < 0) return 3;
                return 4;
            }
            if (source->value != 0) {
                return abs(source->value);
            }
            return twin->source->value + (twin->source-> value < 3 ? 2 : -2);
        }

		Line getLine() {
			if (source->value == 0 && twin->source->value == 0) {
                return Line(*source, *twin->source);
            }
            if (source->value != 0 && twin->source->value != 0) {
                return source->value > 0
                        ? Line(source->x, source->y, twin->source->x)
                        : Line(twin->source->x, twin->source->y, source->x);
            }
            return source->value > 0
                    ? Line(source->x, source->y, -source->x * twin->source->x - source->y * twin->source->y)
                    : Line(twin->source->x, twin->source->y, -source->x * twin->source->x - source->y * twin->source->y);
		}

		bool onEdge(const Point& p) {
			auto s = getStart(), e = getEnd();
            if (s != nullptr && e != nullptr) {
                return fuzzyCompare(p.x, std::min(s->x, e->x)) >= 0 
					&& fuzzyCompare(p.x, std::max(s->x, e->x)) <= 0
                    && fuzzyCompare(p.y, std::min(s->y, e->y)) >= 0
                    && fuzzyCompare(p.y, std::max(s->y, e->y)) <= 0;
            }
            if (s == nullptr && e == nullptr) {
                return true;
            }
            switch (getQuadrant()) {
                case 1:
                    return s == nullptr
                        ? fuzzyCompare(p.x, e->x) <= 0 && fuzzyCompare(p.y, e->y) <= 0
                        : fuzzyCompare(p.x, s->x) >= 0 && fuzzyCompare(p.y, s->y) >= 0;
                case 2:
                    return s == nullptr
                        ? fuzzyCompare(p.x, e->x) >= 0 && fuzzyCompare(p.y, e->y) <= 0
                        : fuzzyCompare(p.x, s->x) <= 0 && fuzzyCompare(p.y, s->y) >= 0;
                case 3:
                    return s == nullptr
                        ? fuzzyCompare(p.x, e->x) >= 0 && fuzzyCompare(p.y, e->y) >= 0
                        : fuzzyCompare(p.x, s->x) <= 0 && fuzzyCompare(p.y, s->y) <= 0;
                default:
                    return s == nullptr
                        ? fuzzyCompare(p.x, e->x) <= 0 && fuzzyCompare(p.y, e->y) >= 0
                        : fuzzyCompare(p.x, s->x) >= 0 && fuzzyCompare(p.y, s->y) <= 0;
            }
		}

		static HalfEdge* createEdge(std::shared_ptr< Point > p1, std::shared_ptr< Point > p2, const Line& l, Cell* left, Cell* right) {
			int q = (l.a > 0 && l.b > 0) || (l.a < 0 && l.b < 0) || fuzzyCompare(l.a, 0) == 0 ? 4 : 3;
			if (p1 == nullptr) {
				p1 =  std::make_shared< Point >(l.a, l.b, q - 2);
				if (p2 == nullptr) {
					p2 =  std::make_shared< Point >(l.c, 0, -q);
				}
			} else if (p2 == nullptr) {
				p2 =  std::make_shared< Point >(l.a, l.b, q);
			}
			HalfEdge* leftEdge = new HalfEdge(p1, left);
			HalfEdge* rightEdge = new HalfEdge(p2, right);
			leftEdge->twin = rightEdge->next = rightEdge->prev = rightEdge;
			rightEdge->twin = leftEdge->next = leftEdge->prev = leftEdge;
			return leftEdge;
		}

	private:
		std::shared_ptr< Point > source;
};

class VoronoiChainTree {
	public:
		std::unique_ptr< VoronoiChainTree > l = nullptr, r = nullptr;
		std::vector< Cell* > left, right;
		std::vector< std::shared_ptr< Point > > chain;


};

class HalfEdgePtr {
	
	public:
		Cell* cell;
		std::shared_ptr< Point > cp = nullptr;
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

		void intersection(const Line& seam, std::shared_ptr< Point > last) {
			if (edge != nullptr) {
				auto start = edge;
				do {
					auto p = edge->getLine().intersection(seam);
					if (p != nullptr) {
						int cmpY = last == nullptr ? -1 : fuzzyCompare(p->y, last->y);
						if ((cmpY < 0 || (cmpY == 0 && fuzzyCompare(p->x, last->x) > 0)) && edge->onEdge(*p)) {
							int eq = p->fuzzyEquals(edge->getStart().get()) ? -1 : (p->fuzzyEquals(edge->getEnd().get()) ? 1 : 0);
                            if (eq == 0) {
                                cp = p;
                            } else if (eq == -1) {
                                cp = edge->getStart();
                                if (clockwise) move();
                            } else {
                                cp = edge->getEnd();
                                if (!clockwise) move();
                            }
                            return;
						}
					}
					move();
				} while (edge != start);
			}
			cp = nullptr;
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
            return node->next = node->prev = node;
        }

    private:
        PolyNode(Point* p) : p(p) {}
};

std::pair< PolyNode*, PolyNode* > findRightChain(Point* p, PolyNode* right) {
	if (right == right->next) return std::make_pair(right, right);
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
	if (Point::orientation(m[up]->p, m[up2]->prev->p, m[up2]->p) == 0) {
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

void markEdgesForDeletion(HalfEdge* curr, HalfEdge* finish, std::vector< HalfEdge* >& deletion) {
	while (curr != finish) {
		deletion.emplace_back(curr);
		curr = curr->next;
	}
}

void connectChain(HalfEdge* first, HalfEdge* chainStart, HalfEdge* second, bool headSkipped, std::vector< HalfEdge* >& deletion) {
	Cell* cell = chainStart->cell;
	auto chainEnd = chainStart->prev;
	if (first != nullptr && second != nullptr) { // Два пересечения
		if (cell->head != cell->head->next && cell->head->next == cell->head->prev && cell->head->getLine().isParallel(cell->head->next->getLine())) { // Две параллельные прямые
			if (cell->head->getStart() != nullptr) { 
				cell->head = cell->head->next;
			}
			headSkipped = false;
		} else { // Отрезаем кусок ячейки
			markEdgesForDeletion(first->next, second, deletion);
		}
		first->next = chainStart; //edges for delete
		chainStart->prev = first;
		second->prev = chainEnd;
		chainEnd->next = second;
		if (headSkipped) {
			cell->head = chainStart;
		}
	} else if (first == nullptr && second == nullptr) { // Пересечения нет
 		if (cell->head != nullptr) { // одна прямая
			cell->head->prev = cell->head->next = chainStart;
			chainStart->prev = chainStart->next = cell->head;
		}
		cell->head = chainStart;
	} else if (first == nullptr) { 
		markEdgesForDeletion(cell->head->prev->next, second, deletion);
		cell->head->prev->next = chainStart;
		chainStart->prev = cell->head->prev;
		second->prev = chainEnd;
		chainEnd->next = second;
		cell->head = chainStart;
	} else {
		markEdgesForDeletion(first->next, cell->head, deletion);
		first->next = chainStart;
		chainStart->prev = first;
		cell->head->prev = chainEnd;
		chainEnd->next = cell->head;
	}
}

HalfEdge* addChainLink(HalfEdge* edge, HalfEdge* head, bool inHead) {
    if (head == nullptr) {
		return edge->prev = edge->next = edge;
	}
	edge->next = head;
	edge->prev = head->prev;
	head->prev = head->prev->next = edge;
	return inHead ? edge : head;
}

void mergeVoronoi(const std::pair< Point*, Point* >& bridge) {
	HalfEdgePtr left = HalfEdgePtr(static_cast< Cell* >(bridge.second), true);
	HalfEdgePtr right = HalfEdgePtr(static_cast< Cell* >(bridge.first), false);
	std::vector< HalfEdge* > deletion;
	std::shared_ptr< Point > lastP = nullptr;
	HalfEdge* leftChain = nullptr;
	HalfEdge* rightChain = nullptr;
	while (true) {
		Point mid = Point((left.cell->x + right.cell->x) / 2, (left.cell->y + right.cell->y) / 2);
		Line seam = Line::perpendicular(*left.cell, *right.cell, mid);
		left.intersection(seam, lastP);
		right.intersection(seam, lastP);
		if (left.cp == nullptr && right.cp == nullptr) {
			auto edge = HalfEdge::createEdge(nullptr, lastP, seam, left.cell, right.cell);
        	leftChain = addChainLink(edge, leftChain, true);
            rightChain = addChainLink(edge->twin, rightChain, false);
            connectChain(nullptr, leftChain, left.top, left.headSkipped, deletion);
            connectChain(right.top, rightChain, nullptr, right.headSkipped, deletion);
			break;
		}
		int cmp = left.cp == nullptr ? 1 : (right.cp == nullptr ? -1 : fuzzyCompare(right.cp->y, left.cp->y));
		std::shared_ptr< Point > point = cmp <= 0 ? left.cp : right.cp;
		auto edge = HalfEdge::createEdge(point, lastP, seam, left.cell, right.cell);
		leftChain = addChainLink(edge, leftChain, true);
		rightChain = addChainLink(edge->twin, rightChain, false);
		lastP = point;
		if (cmp <= 0) {
			auto intersectTwin = point->fuzzyEquals(left.edge->getEnd().get()) ? left.edge->next->twin->next : left.edge->twin;
			left.edge->setEnd(point);
			intersectTwin->setStart(point);
			connectChain(left.edge, leftChain, left.top, left.headSkipped, deletion);
			left.set(intersectTwin);
			leftChain = nullptr;
		}
		if (cmp >= 0) {
			auto intersectTwin = right.edge->twin;
			if (point->fuzzyEquals(right.edge->getStart().get())) {
				while (right.edge->prev->twin->prev != intersectTwin) {
					intersectTwin->setEnd(point);
					intersectTwin = intersectTwin->next->twin;
				}
			}
			right.edge->setStart(point);
			intersectTwin->setEnd(point);
			connectChain(right.top, rightChain, right.edge, right.headSkipped, deletion);
			right.set(intersectTwin);
			rightChain = nullptr;
		}
	}
	for (size_t i = 0; i < deletion.size(); ++i) {
		delete deletion[i];
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

void printCell(Cell* cell) {
	std::cout << cell->x << ' ' << cell->y << ' ' << cell->value << std::endl;
	auto curr = cell->head;
	int k = 0;
	do {
		std::cout << "HalfEdge: " << curr;
		if (curr->getStart() != nullptr) {
			std::cout << " start: " << curr->getStart()->x << " " << curr->getStart()->y;
		}
		if (curr->getEnd() != nullptr) {
			std::cout << " end: " << curr->getEnd()->x << " " << curr->getEnd()->y;
		}
		std::cout << "   |   " << curr->getLine().toString() << " " << curr->twin <<std::endl;
		k++;
		curr = curr->next;
	} while (curr != cell->head);
	std::cout << k << std::endl;
}
 

int main(int argc, char** argv) {
	// freopen("input.txt", "r", stdin);
	freopen("output.txt", "w", stdout);
	int32_t regionSize = 32, mapWidth = 256, mapHeight = 256, seed = 1684952532;// 1683966317, 1684952532
	std::cout << "Seed: " << seed << std::endl;
	PerlinNoise2D perlin(seed);
	perlin.saveImage(mapWidth, mapHeight, 64, 3);

	std::vector<Cell*> cells;
	std::vector<MapTile::Type> tiles;
	for (int32_t i = 0; i < mapHeight; ++i) {
		for (int32_t j = 0; j < mapWidth; ++j) {
			cells.push_back(new Cell(regionSize / 2 + j * regionSize, regionSize / 2 + i * regionSize, i * mapWidth + j + 1));
			tiles.push_back(MapTile::getTile(perlin.noise(j / 64.0f, i / 64.0f, 3)));
		}
	}

	time_t seed2 = time(0);
	std::cout << seed2 << std::endl;
	std::default_random_engine engine(seed2); // error 1685837735
    std::uniform_real_distribution<double> regionRand(-0.4 * regionSize, 0.4 * regionSize);
	std::vector<int32_t> dx = {-1, -1, 1, 1}, dy = {-1, 1, 1, -1};
	for (int32_t i = mapWidth; i < static_cast<int32_t>(tiles.size()) - mapWidth; ++i) {
		if (i % mapWidth == 0 || i % mapWidth == mapWidth -1) continue;
		int32_t moveX = 0, moveY = 0;
		for (int32_t k = 0; k < 4; ++k) {
			if (tiles[i + dx[k] + dy[k] * mapWidth] == tiles[i] && (tiles[i + dx[k]] != tiles[i] || tiles[i + dy[k] * mapWidth] != tiles[i])) {
				moveX += dx[k];
				moveY += dy[k];
			} else if (tiles[i + dx[k] + dy[k] * mapWidth] != tiles[i] && tiles[i + dx[k]] != tiles[i] && tiles[i + dy[k] * mapWidth] != tiles[i]) {
				moveX -= dx[k];
				moveY -= dy[k];
			}
		}
		cells[i]->x += round(moveX == 0 ? regionRand(engine) : (moveX < 0 ? -abs(regionRand(engine)) : abs(regionRand(engine))));
		cells[i]->y += round(moveY == 0 ? regionRand(engine) : (moveY < 0 ? -abs(regionRand(engine)) : abs(regionRand(engine))));
	}
	for (int32_t i = 0; i < mapHeight; ++i) {
		cells.push_back(new Cell(-regionSize / 2, regionSize / 2 + i * regionSize));
		cells.push_back(new Cell(regionSize / 2 + mapWidth * regionSize, regionSize / 2 + i * regionSize));
	}
	std::cout << std::endl;
	for (int32_t j = 0; j < mapWidth; ++j) {
		cells.push_back(new Cell(regionSize / 2 + j * regionSize, -regionSize / 2));
		cells.push_back(new Cell(regionSize / 2 + j * regionSize, regionSize / 2 + mapHeight * regionSize));
	}
	std::cout << std::endl;

	sort(cells.begin(), cells.end(), [](Cell* a, Cell* b) { return fuzzyCompare(a->x, b->x) == -1 || (fuzzyCompare(a->x, b->x) == 0 && fuzzyCompare(a->y, b->y) == -1); });

	voronoi(cells, 0, cells.size());
	std::cout << "voronoi ends" << std::endl;

	std::vector< Vertex > vertices;
	std::vector< uint32_t > indices;
	uint32_t index = 0;
	for (size_t i = 0; i < cells.size(); ++i) {
		if (cells[i]->value == 0) continue;
		float aNoiseVal = perlin.noise((cells[i]->x / regionSize) / 64.0, (cells[i]->y / regionSize) / 64.0, 3);
		glm::vec3 aColor = MapTile::getColor(tiles[cells[i]->value - 1]);
		// printCell(cells[i]);
		auto curr = cells[i]->head;
	
		Vertex a = { { 2 * cells[i]->x / (mapWidth * regionSize) - 1, 2 * cells[i]->y / (mapHeight * regionSize) - 1, 1 - aNoiseVal }, aColor, { 0.0, 0.0, 0.0 } };
		uint32_t aIndex = index++;
		vertices.emplace_back(a);
		do {
			auto start = curr->getStart();
			auto end = curr->getEnd();
			if (start != nullptr && end != nullptr) {
				float bNoiseVal = perlin.noise((std::max(0.0, start->x) / regionSize) / 64.0, (std::max(0.0, start->y) / regionSize) / 64.0, 3);
				float cNoiseVal = perlin.noise((std::max(0.0, end->x) / regionSize) / 64.0, (std::max(0.0, end->y) / regionSize) / 64.0, 3);
				Vertex b = { { 2 * start->x / (mapWidth * regionSize)  - 1, 2 * start->y / (mapHeight * regionSize) - 1, 1 - bNoiseVal  }, aColor, { 0.0, 0.0, 0.0 } };
				Vertex c = { { 2 * end->x / (mapWidth * regionSize)  - 1, 2 * end->y / (mapHeight * regionSize) - 1, 1 - cNoiseVal  }, aColor, { 0.0, 0.0, 0.0 } };
				vertices.emplace_back(c);
				vertices.emplace_back(b);
				indices.emplace_back(aIndex);
				indices.emplace_back(index++);
				indices.emplace_back(index++);
			}
			curr = curr->next;
		} while (curr != cells[i]->head);
	}

	VulkanEngine vulkanEngine;
	vulkanEngine.vertices = vertices;
	vulkanEngine.indices = indices;
    try {
        vulkanEngine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
	return 0;
}