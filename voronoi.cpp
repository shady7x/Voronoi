#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include "vulkan_engine.h"

const double EPS = 1e-10;

int fuzzy_compare(double a, double b)
{
	if (abs(a - b) < EPS)
		return 0;
	return a < b ? -1 : 1;
}

class Point
{
public:
	const double x, y;
	Point(double _x, double _y) : x(_x), y(_y) {}
	static int orientation(Point* a, Point* b, Point* c)
	{
		double s = (b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x);
		return s < -EPS ? -1 : (s > EPS ? 1 : 0); // -1 правее a b, 0 на линии, 1 левее a b
	}
};

class PolyNode
{
public:
	Point* p;
	PolyNode* next = nullptr;
	PolyNode* prev = nullptr;
	PolyNode(Point* _p) : p(_p) {}
};

std::pair< PolyNode*, PolyNode* > find_right_chain(Point* p, PolyNode* right)
{
	//if (right == nullptr)

	if (right->next == nullptr || right->prev == nullptr)
		return std::make_pair(right, right);

	if (right->next == right->prev)
	{
		return Point::orientation(p, right->p, right->next->p) < 0
			? std::make_pair(right->next, right)
			: std::make_pair(right, right->next);
	}

	PolyNode* current = right;
	std::pair< PolyNode*, PolyNode* > chain;

	while (chain.first == nullptr || chain.second == nullptr)
	{
		int prev_orient = Point::orientation(p, current->p, current->prev->p);
		int next_orient = Point::orientation(p, current->p, current->next->p);

		if (chain.first == nullptr && prev_orient > 0 && next_orient >= 0)
			chain.first = current;

		if (chain.second == nullptr && prev_orient <= 0 && next_orient < 0)
			chain.second = current;
	}

	return chain;
}

PolyNode* merge(PolyNode* left, PolyNode* right)
{
	auto chain = find_right_chain(left->p, right);
	PolyNode* l = left->next;
	PolyNode* r = chain.first;
	std::vector< PolyNode* > m;
	m.emplace_back(left);
	while (l != nullptr || r != nullptr)
	{
		PolyNode* curr;
		bool rside = (l == nullptr || Point::orientation(left->p, l->p, r->p) < 0);
		if (rside)
		{
			curr = r;
			r = r == chain.second ? nullptr : r->next;
		}
		else
		{
			curr = l;
			l = l->next == left ? nullptr : l->next;
		}

		while (m.size() > 2 && Point::orientation(m[m.size() - 2]->p, m[m.size() - 1]->p, curr->p) <= 0)
		{
			m.pop_back();
		}
	}

	if (m.size() > 2 && Point::orientation(m[m.size() - 2]->p, m[m.size() - 1]->p, m[0]->p) <= 0) // может быть только == 0
	{
		m.pop_back();
	}

	for (size_t i = 0; i < m.size(); ++i)
	{
		m[i]->prev = m[i == 0 ? m.size() - 1 : i - 1];
		m[i]->next = m[(i + 1) % m.size()];
	}

	return m[0]; // left
}

PolyNode* kirkpatrick(const std::vector< Point* >& points, size_t begin, size_t end)
{
	if (end - begin == 1)
	{
		return new PolyNode(points[begin]);
	}

	size_t mid = (begin + end) / 2;
	auto left = kirkpatrick(points, begin, mid);
	auto right = kirkpatrick(points, mid, end);

	return merge(left, right);
}

int main(int argc, char** argv)
{

	VulkanEngine vulkanEngine;

    try {
        vulkanEngine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
	/*int n;
	std::vector < std::pair < double, double > > v;
	std::vector < Point* > points;
	double x, y;

	std::cin >> n;
	for (int i = 0; i < n; ++i)
	{
		std::cin >> x >> y;
		v.emplace_back(x, y);
	}

	std::sort(v.begin(), v.end());

	for (size_t i = 0; i < v.size(); ++i)
	{
		if (i == 0 || v[i] != v[i - 1])
		{
			points.emplace_back(new Point(v[i].first, v[i].second));
		}
	}

	auto head = kirkpatrick(points, 0, points.size());
	*/


	
	/*std::vector< std::string > extensions;
	getAvailableVulkanExtensions(window, extensions);
    
	SDL_Surface* screen_surface = SDL_GetWindowSurface(window);
	SDL_FillRect(screen_surface, NULL, SDL_MapRGB(screen_surface->format, 0, 255, 0));
	SDL_UpdateWindowSurface(window);*/

	return 0;
}