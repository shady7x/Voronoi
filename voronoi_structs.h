#include <memory>
#include <cmath>
#include <cstdint>
#include <string>

const double EPS = 1e-9;

inline int fuzzyCompare(double val1, double val2) {
	double eps = (abs(val2) + 1.0) * EPS;
	double diff = val1 - val2;
	return diff < -eps ? -1 : (diff > eps ? 1 : 0);
}

class Point {
    public:
	    double x, y;
		int32_t value;
		uint32_t index;

        Point(double x, double y, int32_t value = 0, uint32_t index = 0) : x(x), y(y), value(value), index(index) {}
        
		double distSqr(const Point& p) {
            return (x - p.x) * (x - p.x) + (y - p.y) * (y - p.y);
        }
        
		bool fuzzyEquals(const Point* other) {
        	return other != nullptr && fuzzyCompare(x, other->x) == 0 && fuzzyCompare(y, other->y) == 0;
		}

		std::string toString() {
			return "(" + std::to_string(x) + ", " + std::to_string(y) + ")";
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

		Cell(double x, double y, int32_t value = 0, uint32_t index = 0) : Point(x, y, value, index) {}
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
			return std::to_string(a) + "x + " + std::to_string(b) + "y + " + std::to_string(c) + " = 0";
		}
};

class HalfEdge {
	public:
		Cell* cell;
		HalfEdge* next = nullptr;
		HalfEdge* prev = nullptr;
		HalfEdge* twin;

		HalfEdge(std::shared_ptr< Point > source, Cell* cell) : source(source), cell(cell) {}

		std::string toString() {

			return "HalfEdge cell: " + cell->toString() + " twin->cell: " + twin->cell->toString() + "\n"
				+ " start: " + (getStart() != nullptr ? (std::to_string(getStart()->x) + " " + std::to_string(getStart()->y)) : getLine().toString()) + "\n"
				+ " end: " + (getEnd() != nullptr ? (std::to_string(getEnd()->x) + " " + std::to_string(getEnd()->y)) : getLine().toString());
		}

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
