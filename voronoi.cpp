#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <set>
#include <map>
#include <cmath>
#include <sstream>
#include <memory>
#include <limits>

#include "voronoi_structs.h"
#include "perlin_noise_2d.h"
#include "vulkan_engine.h"




void printCell(Cell* cell) {
	std::cout << "-----------" << cell->value << "----------\n";
	std::cout << cell->toString() << std::endl;
	auto curr = cell->head;
	if (curr != nullptr) {
		do {
			std::cout << curr << ' ' << curr->toString() << std::endl;
			curr = curr->next;
		} while (curr != cell->head);
	}
	std::cout << "----------------------------" << std::endl;
	
}

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

int main(int argc, char** argv) {
	// freopen("output.txt", "w", stdout);
	int32_t seed = 1686078735; //1685906448
	std::cout << "Seed: " << seed << std::endl;
	PerlinNoise2D perlin(seed);
	perlin.saveImage(MAP_WIDTH, MAP_HEIGHT, 64, 3);

	std::vector<Cell*> cells;
	std::vector<MapTile::Type> tiles;
	for (int32_t i = 0; i < MAP_HEIGHT; ++i) {
		for (int32_t j = 0; j < MAP_WIDTH; ++j) {
			cells.push_back(new Cell(REGION_SIZE / 2 + j * REGION_SIZE, REGION_SIZE / 2 + i * REGION_SIZE, i * MAP_WIDTH + j + 1, i * MAP_WIDTH + j + 1));
			tiles.push_back(MapTile::getTile(perlin.noise(j / 64.0f, i / 64.0f, 3)));
		}
	}

	time_t seed2 = seed;
	std::cout << "Seed2: " << seed2 << std::endl; 
	std::default_random_engine engine(seed2);
    std::uniform_real_distribution<double> regionRand(-0.4 * REGION_SIZE, 0.4 * REGION_SIZE);
	std::vector<int32_t> dx = {-1, -1, 1, 1}, dy = {-1, 1, 1, -1};
	for (int32_t i = MAP_WIDTH; i < static_cast<int32_t>(tiles.size()) - MAP_WIDTH; ++i) {
		if (i % MAP_WIDTH == 0 || i % MAP_WIDTH == MAP_WIDTH -1) continue;
		int32_t moveX = 0, moveY = 0;
		for (int32_t k = 0; k < 4; ++k) {
			if (tiles[i + dx[k] + dy[k] * MAP_WIDTH] == tiles[i] && (tiles[i + dx[k]] != tiles[i] || tiles[i + dy[k] * MAP_WIDTH] != tiles[i])) {
				moveX += dx[k];
				moveY += dy[k];
			} else if (tiles[i + dx[k] + dy[k] * MAP_WIDTH] != tiles[i] && tiles[i + dx[k]] != tiles[i] && tiles[i + dy[k] * MAP_WIDTH] != tiles[i]) {
				moveX -= dx[k];
				moveY -= dy[k];
			}
		}
		cells[i]->x += round(moveX == 0 ? regionRand(engine) : (moveX < 0 ? -abs(regionRand(engine)) : abs(regionRand(engine))));
		cells[i]->y += round(moveY == 0 ? regionRand(engine) : (moveY < 0 ? -abs(regionRand(engine)) : abs(regionRand(engine))));
	}
	for (int32_t i = 0; i < MAP_HEIGHT; ++i) {
		cells.push_back(new Cell(-REGION_SIZE / 2, REGION_SIZE / 2 + i * REGION_SIZE));
		cells.push_back(new Cell(REGION_SIZE / 2 + MAP_WIDTH * REGION_SIZE, REGION_SIZE / 2 + i * REGION_SIZE));
	}
	for (int32_t j = 0; j < MAP_WIDTH; ++j) {
		cells.push_back(new Cell(REGION_SIZE / 2 + j * REGION_SIZE, -REGION_SIZE / 2));
		cells.push_back(new Cell(REGION_SIZE / 2 + j * REGION_SIZE, REGION_SIZE / 2 + MAP_HEIGHT * REGION_SIZE));
	}
	sort(cells.begin(), cells.end(), [](Cell* a, Cell* b) { return fuzzyCompare(a->x, b->x) == -1 || (fuzzyCompare(a->x, b->x) == 0 && fuzzyCompare(a->y, b->y) == -1); });
	voronoi(cells, 0, cells.size());
	std::cout << "voronoi ends" << std::endl;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<glm::vec3> normals; 
	std::vector<std::pair<uint32_t, uint32_t>> eXv; 

	int32_t vertIndex = MAP_WIDTH * MAP_HEIGHT + 1;
	for (size_t i = 0; i < cells.size(); ++i) {
		if (cells[i]->value == 0) continue;
		float aNoiseVal = perlin.noise((cells[i]->x / REGION_SIZE) / 64.0, (cells[i]->y / REGION_SIZE) / 64.0, 3);
		glm::vec3 aColor = MapTile::getColor(tiles[cells[i]->value - 1]);
		// float tileHeight = tiles[cells[i]->value - 1] >= MapTile::MOUNTAIN ? 0 : 0.05;
		// printCell(cells[i]);
		auto curr = cells[i]->head;
		Vertex a = { { 2 * cells[i]->x / (MAP_WIDTH * REGION_SIZE) - 1, 2 * cells[i]->y / (MAP_HEIGHT * REGION_SIZE) - 1, 1 - aNoiseVal, 0 }, aColor, {0, 0, 0}, { 0.0, 0.0, 0.0 }};
		vertices.emplace_back(a);
		uint32_t aIndex = vertices.size() - 1;
		do {
			auto start = curr->getStart();
			auto end = curr->getEnd();
			if (start != nullptr && end != nullptr) {
				float bNoiseVal = perlin.noise((std::max(0.0, start->x) / REGION_SIZE) / 64.0, (std::max(0.0, start->y) / REGION_SIZE) / 64.0, 3);
				float cNoiseVal = perlin.noise((std::max(0.0, end->x) / REGION_SIZE) / 64.0, (std::max(0.0, end->y) / REGION_SIZE) / 64.0, 3);
				Vertex b = { { 2 * start->x / (MAP_WIDTH * REGION_SIZE)  - 1, 2 * start->y / (MAP_HEIGHT * REGION_SIZE) - 1, 1 - bNoiseVal, 0 }, aColor, {0, 0, 0}, { 0.0, 0.0, 0.0 } };
				Vertex c = { { 2 * end->x / (MAP_WIDTH * REGION_SIZE)  - 1, 2 * end->y / (MAP_HEIGHT * REGION_SIZE) - 1, 1 - cNoiseVal, 0 }, aColor, {0, 0, 0}, { 0.0, 0.0, 0.0 } };
				
				glm::vec3 vec1 = { b.pos.x - a.pos.x, b.pos.y - a.pos.y, b.pos.z - a.pos.z };
				glm::vec3 vec2 = { c.pos.x - a.pos.x, c.pos.y - a.pos.y, c.pos.z - a.pos.z };
				glm::vec3 norm = glm::cross(vec1, vec2);
				
				if (start->index == 0) {
					start->index = vertIndex++;
					normals.push_back(norm);
				} else {
					normals[start->index - MAP_WIDTH * MAP_HEIGHT - 1] += norm;
				}
				if (end->index == 0) {
					end->index = vertIndex++;
					normals.push_back(norm);
				} else {
					normals[end->index - MAP_WIDTH * MAP_HEIGHT - 1] += norm;
				}
				vertices[aIndex].normal += norm;
				vertices.emplace_back(b);
				eXv.push_back(std::make_pair<uint32_t, uint32_t>(start->index - MAP_WIDTH * MAP_HEIGHT - 1, vertices.size() - 1));
				vertices.emplace_back(c);
				eXv.push_back(std::make_pair<uint32_t, uint32_t>(end->index - MAP_WIDTH * MAP_HEIGHT - 1, vertices.size() - 1));
				indices.emplace_back(aIndex);
				indices.emplace_back(vertices.size() - 2);
				indices.emplace_back(vertices.size() - 1);

				// if (tiles[cells[i]->value - 1] >= MapTile::MOUNTAIN && tiles[curr->twin->cell->value - 1] < MapTile::MOUNTAIN) {
				// 	b.pos.z = c.pos.z = 0.05;
				// 	c.color = b.color = {0.6, 0.6, 0.6};
				// 	vertices.emplace_back(c);
				// 	vertices.emplace_back(b);
					
				// 	index += 2;
				// 	indices.emplace_back(index - 3);
				// 	indices.emplace_back(index - 4);
				// 	indices.emplace_back(index - 1);
				// 	indices.emplace_back(index - 4);
				// 	indices.emplace_back(index - 2);
				// 	indices.emplace_back(index - 1);
					
				// }
			}
			curr = curr->next;
		} while (curr != cells[i]->head);
	}
	for(size_t i = 0; i < eXv.size(); ++i) {
		vertices[eXv[i].second].normal = normals[eXv[i].first];
	}

	auto tH = 1 - perlin.noise(MAP_WIDTH / 2 / 64, MAP_HEIGHT / 2 / 64, 3);
	Vertex tA = { { 0, 0, tH, 2.0 }, {0, 0, 1}, {0, 0, 0}, {0, 0, 0} };
	Vertex tB = { { -0.01, 0, tH - 0.1, 2.0 }, {0, 0, 1}, {0, 0, 0}, {0, 0, 0} };
	Vertex tC = { { 0.01, 0, tH - 0.1,  2.0 }, {0, 0, 1}, {0, 0, 0}, {0, 0, 0} };
	vertices.push_back(tA);
	vertices.push_back(tB);
	vertices.push_back(tC);
	indices.emplace_back(vertices.size() - 3);
	indices.emplace_back(vertices.size() - 2);
	indices.emplace_back(vertices.size() - 1);

	std::ifstream infile("LP_Airplane.obj");
	std::vector<Vertex> plane;
	std::vector<uint32_t> planeIndices;
	glm::vec3 planeColor = {1, 0, 0};
	std::string line;
	size_t offsetVertices = vertices.size();
	while (std::getline(infile, line)) {
	    std::istringstream iss(line);
		std::string s, t;
		iss >> s;
		if (s == "v") {
			double x, y, z;
			iss >> x >> y >> z;
			x /= 100; y /= 100; z /= 100;
			vertices.push_back({ { x, y, z, 1.0 }, {1, 1, 1}, {0, 0, 0}, {0, 0, 0} });
			if (vertices.size() < offsetVertices + 500) {
				vertices[vertices.size() - 1].color = {1, 0, 0};
			}
		} else if (s == "s") {
			int x;
			iss >> x;
			if (x == 1) {
				planeColor = {1, 0, 0};
			} else if (x == 2) {
				planeColor = {0, 1, 0};
			} else if (x == 3) {
				planeColor = {0, 0, 1};
			} else if (x == 4) {
				planeColor = {1, 1, 1};
			} else if (x == 5) {
				planeColor = {1, 1, 0};
			}

 		} else if (s == "f") {
			std::string t;
			std::vector<uint32_t> polyline;
			uint32_t idx;
			while (iss >> idx >> t) {
				polyline.push_back(idx - 1);
			}
			for (size_t i = 2 ; i < polyline.size(); ++i) {
				indices.push_back(offsetVertices + polyline[0]);
				indices.push_back(offsetVertices + polyline[i - 1]);
				indices.push_back(offsetVertices + polyline[i]);
			}
			
		}
	}



	VulkanEngine vulkanEngine(perlin, cells);
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

/*
Seed: 1685904510
Seed2: 1685904510
*/

/*
Seed 1685897482
*/