#pragma once
#include <vector>
#include <cmath>
#include <algorithm>   // ? NUEVO: necesario para std::min

// matematica 2d
struct Vec2 {
	float x, y;
	Vec2(float x = 0.f, float y = 0.f) : x(x), y(y) {}
	Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
	Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
	Vec2 operator*(float s) const { return {x * s, y * s}; }
};

inline float dot(Vec2 a, Vec2 b)           { return a.x*b.x + a.y*b.y; }
inline float lenSq(Vec2 v)                 { return dot(v, v); }
inline float len(Vec2 v)                   { return sqrtf(lenSq(v)); }
inline float distancia(Vec2 a, Vec2 b)     { return len(a - b); }

inline float distanciaSegmento(Vec2 p, Vec2 a, Vec2 b) {
	Vec2 ab = b - a;
	float t = (lenSq(ab) > 1e-10f) ? dot(p - a, ab) / lenSq(ab) : 0.f;
	if (t < 0.f) t = 0.f;
	if (t > 1.f) t = 1.f;
	return distancia(p, { a.x + t*ab.x, a.y + t*ab.y });
}

inline Vec2 centroide(const std::vector<Vec2>& v) {
	Vec2 c;
	for (const auto& p : v) { c.x += p.x; c.y += p.y; }
	if (!v.empty()) { c.x /= v.size(); c.y /= v.size(); }
	return c;
}

enum ShapeType {
	SHAPE_POINT, SHAPE_LINE, SHAPE_POLYLINE, SHAPE_POLYGON, SHAPE_CIRCLE
};

struct Shape {
	ShapeType         type;
	std::vector<Vec2> verts;
	float r, g, b;
	float fr, fg, fb;
	bool  filled   = false;
	bool  selected = false;
	
	Shape(ShapeType t, float r = 1.f, float g = 1.f, float b = 1.f)
		: type(t), r(r), g(g), b(b), fr(0.8f), fg(0.9f), fb(1.f) {}
};

inline float distanciaShape(Vec2 p, const Shape& s) {
	if (s.verts.empty()) return 1e9f;
	switch (s.type) {
	case SHAPE_POINT:
		return distancia(p, s.verts[0]);
	case SHAPE_CIRCLE: {
		if (s.verts.size() < 2) return 1e9f;
		float r = distancia(s.verts[0], s.verts[1]);
		return fabsf(distancia(p, s.verts[0]) - r);
	}
	default: {
		float d = 1e9f;
		int n     = (int)s.verts.size();
		int edges = (s.type == SHAPE_POLYGON) ? n : n - 1;
		for (int i = 0; i < edges; i++)
			d = std::min(d, distanciaSegmento(p, s.verts[i], s.verts[(i+1) % n]));
		return d;
	}
	}
}

struct Mat3 {
	float m[3][3];
	Mat3() {
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				m[i][j] = (i == j) ? 1.f : 0.f;
	}
	Mat3 operator*(const Mat3& o) const {
		Mat3 r;
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++) {
				r.m[i][j] = 0.f;
				for (int k = 0; k < 3; k++) r.m[i][j] += m[i][k] * o.m[k][j];
		}
			return r;
	}
	Vec2 apply(Vec2 p) const {
		return { m[0][0]*p.x + m[0][1]*p.y + m[0][2],
			m[1][0]*p.x + m[1][1]*p.y + m[1][2] };
	}
};

inline Mat3 matTranslate(float tx, float ty) {
	Mat3 M; M.m[0][2] = tx; M.m[1][2] = ty; return M;
}
inline Mat3 matRotate(float a) {
	Mat3 M; float c = cosf(a), s = sinf(a);
	M.m[0][0] = c; M.m[0][1] = -s;
	M.m[1][0] = s; M.m[1][1] =  c;
	return M;
}
inline Mat3 matScale(float sx, float sy) {
	Mat3 M; M.m[0][0] = sx; M.m[1][1] = sy; return M;
}
inline Mat3 matRotateAround(Vec2 p, float a) {
	return matTranslate(p.x, p.y) * matRotate(a) * matTranslate(-p.x, -p.y);
}
inline Mat3 matScaleAround(Vec2 p, float sx, float sy) {
	return matTranslate(p.x, p.y) * matScale(sx, sy) * matTranslate(-p.x, -p.y);
}
inline void applyTransform(Shape& s, const Mat3& M) {
	for (auto& v : s.verts) v = M.apply(v);
}

enum Mode {
	MODE_SELECT, MODE_POINT, MODE_LINE, MODE_POLYLINE, MODE_POLYGON, MODE_CIRCLE
};

struct EditorState {
	int   W = 900, H = 650;
	float scale = 40.f;
	Mode modo = MODE_POLYGON;    // ? empezamos en POLYGON para que sea útil directo
	
	std::vector<Shape> escena;
	std::vector<Vec2>  tmpVerts;
	
	Vec2 mouseWorld;
	int  selectedIdx = -1;
	
	int   colorIdx    = 0;
	int   fillColorIdx = 0;
	float colorR = 0.1f, colorG = 0.4f, colorB = 0.9f;
	float fillR  = 0.8f, fillG  = 0.9f, fillB  = 1.0f;
};

extern EditorState ed;
Vec2 screenToWorld(int mx, int my);
