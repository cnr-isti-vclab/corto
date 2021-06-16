/*
Corto

Copyright(C) 2017 - Federico Ponchio
ISTI - Italian National Research Council - Visual Computing Lab

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  You should have received 
a copy of the GNU General Public License along with Corto.
If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CRT_POINT_H
#define CRT_POINT_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

typedef unsigned char uchar;

namespace crt {

template <typename S> class Point2 {
private:
	S v[2];

public:
	Point2() {}
	Point2(S x, S y) { v[0] = x; v[1] = y; }
	Point2(S *x) { v[0] = x[0]; v[1] = x[1]; }
	explicit Point2(S x) { v[0] = v[1] = x; }

	S &operator[](int k) { return v[k]; }
	const S &operator[](int k) const { return v[k]; }

	bool operator==(const Point2 &c) const { return v[0] == c[0] && v[1] == c[1]; }
	bool operator!=(const Point2 &c) const { return v[0] != c[0] || v[1] != c[1]; }
	bool operator<(const Point2 &c) const {
		if(v[0] == c[0]) return v[1] < c[1];
		return v[0] < c[0];
	}
	void setMin(const Point2 &c) {
		if(c[0] < v[0]) v[0] = c[0];
		if(c[1] < v[1]) v[1] = c[1];
	}
	void setMax(const Point2 &c) {
		if(c[0] > v[0]) v[0] = c[0];
		if(c[1] > v[1]) v[1] = c[1];
	}

	S operator*(const Point2 &c) { return v[0]*c[0] + v[1]*c[1]; }
	S norm() { return (S)sqrt((double)(v[0]*v[0] + v[1]*v[1])); }

	Point2 operator-() const     { return Point2(-v[0], -v[1]); }
	Point2 operator+(const Point2 &c) const { return Point2(v[0] + c[0], v[1] + c[1]); }
	Point2 operator-(const Point2 &c) const { return Point2(v[0] - c[0], v[1] - c[1]); }
	Point2 operator*(S c) const { return Point2(v[0]*c, v[1]*c); }
	Point2 operator/(S c) const { return Point2(v[0]/c, v[1]/c); }
	Point2 operator>>(int i) const { return Point2(v[0]>>i, v[1]>>i); }
	Point2 operator<<(int i) const { return Point2(v[0]<<i, v[1]<<i); }

	Point2 &operator+=(const Point2 &c) {
		v[0] += c[0];
		v[1] += c[1];
		return *this;
	}

	Point2 &operator-=(const Point2 &c) {
		v[0] -= c[0];
		v[1] -= c[1];
		return *this;
	}

	Point2 &operator*=(S c)  {
		v[0] *= c;
		v[1] *= c;
		return *this;
	}
	Point2 &operator/=(S c)  {
		v[0] /= c;
		v[1] /= c;
		return *this;
	}
};

typedef Point2<int16_t> Point2s;
typedef Point2<int32_t> Point2i;
typedef Point2<float>   Point2f;
typedef Point2<double>  Point2d;


template <typename S> class Point3 {
private:
	S v[3];
public:
	Point3() {}
	Point3(S x, S y, S z) { v[0] = x; v[1] = y; v[2] = z; }
	Point3(S *x) { v[0] = x[0]; v[1] = x[1]; v[2] = x[2]; }
	explicit Point3(S x) { v[0] = v[1] = v[2] = x; }

	S &operator[](int k) { return v[k]; }
	const S &operator[](int k) const { return v[k]; }

	S norm() { return (S)sqrt((double)(v[0]*v[0] + v[1]*v[1] + v[2]*v[2])); }
	S operator*(const Point3 &c) { return v[0]*c[0] + v[1]*c[1] + v[2]*c[2]; }
	Point3 operator^(const Point3 &p) const {
		return Point3(v[1]*p.v[2] - v[2]*p.v[1], v[2]*p.v[0] - v[0]*p.v[2], v[0]*p.v[1] - v[1]*p.v[0]);
	}

	bool operator==(const Point3 &c) const { return v[0] == c[0] && v[1] == c[1] && v[2] == c[2]; }
	bool operator!=(const Point3 &c) const { return !(c == *this); }
	bool operator<(const Point3 &c) const {
		if(v[0] == c[0]) {
			if(v[1] == c[1])
				return v[2] < c[2];
			return v[1] < c[1];
		}
		return v[0] < c[0];
	}
	void setMin(const Point3 &c) {
		if(c[0] < v[0]) v[0] = c[0];
		if(c[1] < v[1]) v[1] = c[1];
		if(c[2] < v[2]) v[2] = c[2];
	}
	void setMax(const Point3 &c) {
		if(c[0] > v[0]) v[0] = c[0];
		if(c[1] > v[1]) v[1] = c[1];
		if(c[2] > v[2]) v[2] = c[2];
	}

	Point3 operator-() const { return Point3(-v[0], -v[1], -v[2]); }

	Point3 operator+(const Point3 &c) const { return Point3(v[0] + c[0], v[1] + c[1], v[2] + c[2]); }
	Point3 operator-(const Point3 &c) const { return Point3(v[0] - c[0], v[1] - c[1], v[2] - c[2]); }
	Point3 operator*(S c) const { return Point3(v[0]*c, v[1]*c, v[2]*c); }
	Point3 operator/(S c) const { return Point3(v[0]/c, v[1]/c, v[2]/c); }

	Point3 &operator-=(const Point3 &c) {
		v[0] -= c[0];
		v[1] -= c[1];
		v[2] -= c[2];
		return *this;
	}
	Point3 &operator+=(const Point3 &c) {
		v[0] += c[0];
		v[1] += c[1];
		v[2] += c[2];
		return *this;
	}
	Point3 &operator*=(S c)  {
		v[0] *= c;
		v[1] *= c;
		v[2] *= c;
		return *this;
	}
	Point3 &operator/=(S c)  {
		v[0] /= c;
		v[1] /= c;
		v[2] /= c;
		return *this;
	}
};

typedef Point3<int16_t> Point3s;
typedef Point3<int32_t> Point3i;
typedef Point3<float>   Point3f;
typedef Point3<double>  Point3d;

template <typename S> class Point4 {
protected:
	S v[4];
public:
	Point4() {}
	Point4(S x, S y, S z, S w) { v[0] = x; v[1] = y; v[2] = z; v[3] = w; }
	Point4(S *x) { v[0] = x[0]; v[1] = x[1]; v[2] = x[2]; v[3] = x[3]; }
	explicit Point4(S x) { v[0] = v[1] = v[2] = v[3] = x; }

	S &operator[](int k) { return v[k]; }
	const S &operator[](int k) const { return v[k]; }

	Point4 operator+(const Point4 &c) const { return Point4(v[0] + c[0], v[1] + c[1], v[2] + c[2], v[3] + c[3]); }
	Point4 operator-(const Point4 &c) const { return Point4(v[0] - c[0], v[1] - c[1], v[2] - c[2], v[3] + c[3]); }
	Point4 operator*(S c) const { return Point4(v[0]*c, v[1]*c, v[2]*c, v[3]*c); }
	Point4 operator/(S c) const { return Point4(v[0]/c, v[1]/c, v[2]/c, v[3]/c); }

	Point4 &operator-=(const Point4 &c) {
		v[0] -= c[0];
		v[1] -= c[1];
		v[2] -= c[2];
		v[3] -= c[3];
		return *this;
	}
	Point4 &operator+=(const Point4 &c) {
		v[0] += c[0];
		v[1] += c[1];
		v[2] += c[2];
		v[3] += c[3];
		return *this;
	}
};

class Color4b: public Point4<unsigned char> {
public:
	Color4b() {}
	Color4b(uchar r, uchar g, uchar b, uchar a) { v[0] = r; v[1] = g; v[2] = b; v[3] = a; }
	Color4b toYCC() { return Color4b(v[1], (uchar)(v[2] - v[1]), (uchar)(v[0] - v[1]), v[3]); }
	Color4b toRGB() { return Color4b((uchar)(v[2] + v[0]), v[0], (uchar)(v[1] + v[0]), v[3]); }
};

/* convenience methods */

/*Point2i lroundf(Point2f &p) { return Point2i(::lroundf(p[0]), ::lroundf(p[1])); }
Point3i lroundf(Point3f &p) { return Point3i(::lroundf(p[0]), ::lroundf(p[1]), ::lroundf(p[2])); } */

/* mapping sphere to octahedron */
class Normal {
public:

};

} //namespace
#endif // CRT_POINT_H
