#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <cmath>
#include <cstdio>
#include <cfloat>
#include <climits>
#include <algorithm>
#include "render.h"
#include "editor.h"

static const float PI = 3.14159265f;

// ---------------------------------------------------------------------------
// CONVERSIÓN MUNDO ? PÍXEL  (Bresenham y Scan-Line trabajan en pixel)
// ---------------------------------------------------------------------------
static inline void worldToPixel(Vec2 w, int& px, int& py) {
	px = (int)roundf(w.x * ed.scale + ed.W * 0.5f);
	py = (int)roundf(w.y * ed.scale + ed.H * 0.5f);
}
static inline void pixelToWorld(int px, int py, float& wx, float& wy) {
	wx = (px - ed.W * 0.5f) / ed.scale;
	wy = (py - ed.H * 0.5f) / ed.scale;
}

// ---------------------------------------------------------------------------
// BRESENHAM — dibuja el segmento P1?P2 con GL_POINTS
// Usa el color activo fijado con glColor3f antes de llamar.
// ---------------------------------------------------------------------------
static void bresenhamLine(Vec2 p1w, Vec2 p2w) {
	int x1, y1, x2, y2;
	worldToPixel(p1w, x1, y1);
	worldToPixel(p2w, x2, y2);
	
	int dx =  abs(x2 - x1);
	int sx = (x1 < x2) ? 1 : -1;
	int dy = -abs(y2 - y1);
	int sy = (y1 < y2) ? 1 : -1;
	int err = dx + dy;
	
	glBegin(GL_POINTS);
	while (true) {
		float wx, wy;
		pixelToWorld(x1, y1, wx, wy);
		glVertex2f(wx, wy);
		if (x1 == x2 && y1 == y2) break;
		int e2 = 2 * err;
		if (e2 >= dy) { err += dy; x1 += sx; }
		if (e2 <= dx) { err += dx; y1 += sy; }
	}
	glEnd();
}

// Contorno completo de un polígono con Bresenham (incluye cierre Pn?P1)
static void bresenhamContorno(const std::vector<Vec2>& verts, bool cerrado) {
	int n = (int)verts.size();
	if (n < 2) return;
	int edges = cerrado ? n : n - 1;
	for (int i = 0; i < edges; i++) {
		bresenhamLine(verts[i], verts[(i + 1) % n]);
	}
}

// ---------------------------------------------------------------------------
// SCAN-LINE  con ET (Edge Table) y EAT (Active Edge Table)
// ---------------------------------------------------------------------------
struct Edge {
	int   ymax;    // pixel y donde la arista deja de estar activa (exclusivo)
	float x;       // intersección x actual (en pixel)
	float dxdy;    // ?x/?y — pendiente inversa
};

// Rellena el interior de un polígono (verts en coords mundo) con color (r,g,b)
static void scanLineFill(const std::vector<Vec2>& verts,
						 float r, float g, float b) {
	int n = (int)verts.size();
	if (n < 3) return;
	
	// 1. Convertir vértices a pixel y obtener ymin / ymax
	std::vector<Vec2> px(n);
	int yminPx = INT_MAX, ymaxPx = INT_MIN;
	for (int i = 0; i < n; i++) {
		int ix, iy;
		worldToPixel(verts[i], ix, iy);
		px[i] = Vec2((float)ix, (float)iy);
		if (iy < yminPx) yminPx = iy;
		if (iy > ymaxPx) ymaxPx = iy;
	}
	if (ymaxPx <= yminPx) return;
	
	// 2. Construir ET: una lista de aristas por cada scanline y
	std::vector<std::vector<Edge>> ET(ymaxPx - yminPx + 1);
	
	for (int i = 0; i < n; i++) {
		Vec2 a = px[i];
		Vec2 c = px[(i + 1) % n];
		int ya = (int)a.y;
		int yc = (int)c.y;
		
		// Las aristas horizontales NO se incorporan a la ET
		if (ya == yc) continue;
		
		// Asegurar que "a" es el extremo inferior
		if (ya > yc) { std::swap(a, c); std::swap(ya, yc); }
		
		Edge e;
		e.ymax = yc;                          // extremo superior (exclusivo)
		e.x    = a.x;                         // x del extremo inferior
		e.dxdy = (c.x - a.x) / (c.y - a.y);   // ?x/?y
		
		int idx = ya - yminPx;
		if (idx >= 0 && idx < (int)ET.size())
			ET[idx].push_back(e);
	}
	
	// 3. Recorrer cada línea de barrido con la EAT
	std::vector<Edge> EAT;
	glColor3f(r, g, b);
	glBegin(GL_POINTS);
	
	for (int y = yminPx; y < ymaxPx; y++) {
		
		// 3.1 Incorporar a EAT las aristas que comienzan en esta y
		int idx = y - yminPx;
		if (idx >= 0 && idx < (int)ET.size())
			for (const auto& e : ET[idx]) EAT.push_back(e);
		
		// 3.2 Retirar las aristas cuyo ymax ya fue alcanzado
		EAT.erase(std::remove_if(EAT.begin(), EAT.end(),
								 [y](const Edge& e) { return e.ymax <= y; }),
				  EAT.end());
		
		// 3.3 Ordenar EAT por coordenada x
		std::sort(EAT.begin(), EAT.end(),
				  [](const Edge& a, const Edge& b) { return a.x < b.x; });
		
		// 3.4 Pintar entre pares de intersecciones
		for (size_t i = 0; i + 1 < EAT.size(); i += 2) {
			int xL = (int)ceilf (EAT[i].x);
			int xR = (int)floorf(EAT[i + 1].x);
			for (int x = xL; x <= xR; x++) {
				float wx, wy;
				pixelToWorld(x, y, wx, wy);
				glVertex2f(wx, wy);
			}
		}
		
		// 3.5 Actualizar x incrementalmente: x += ?x/?y
		for (auto& e : EAT) e.x += e.dxdy;
	}
	
	glEnd();
}
						 
						 // ---------------------------------------------------------------------------
						 // AYUDAS DE DIBUJO
						 // ---------------------------------------------------------------------------
						 void drawText(float wx, float wy, const std::string& txt) {
							 glRasterPos2f(wx, wy);
							 for (char c : txt)
								 glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
						 }
						 
						 // Marcador rojo en cada vértice (UI, no es primitiva de figura)
						 void drawControlPoint(Vec2 p) {
							 glColor3f(0.85f, 0.15f, 0.15f);
							 glPointSize(7.0f);
							 glBegin(GL_POINTS);
							 glVertex2f(p.x, p.y);
							 glEnd();
							 glPointSize(1.0f);
						 }
						 
						 // Grilla con ejes y etiquetas (UI)
						 void dibujarGrilla() {
							 float halfW = ed.W * 0.5f / ed.scale;
							 float halfH = ed.H * 0.5f / ed.scale;
							 int iniX = (int)floorf(-halfW), finX = (int)ceilf(halfW);
							 int iniY = (int)floorf(-halfH), finY = (int)ceilf(halfH);
							 
							 glLineWidth(1.0f);
							 glColor3f(0.88f, 0.88f, 0.92f);
							 glBegin(GL_LINES);
							 for (int i = iniX; i <= finX; i++) {
								 if (i % 5 == 0) continue;
								 glVertex2f((float)i, -halfH); glVertex2f((float)i, halfH);
							 }
							 for (int j = iniY; j <= finY; j++) {
								 if (j % 5 == 0) continue;
								 glVertex2f(-halfW, (float)j); glVertex2f(halfW, (float)j);
							 }
							 glEnd();
							 
							 glLineWidth(1.4f);
							 glColor3f(0.75f, 0.75f, 0.82f);
							 glBegin(GL_LINES);
							 for (int i = iniX; i <= finX; i++) {
								 if (i % 5 != 0 || i == 0) continue;
								 glVertex2f((float)i, -halfH); glVertex2f((float)i, halfH);
							 }
							 for (int j = iniY; j <= finY; j++) {
								 if (j % 5 != 0 || j == 0) continue;
								 glVertex2f(-halfW, (float)j); glVertex2f(halfW, (float)j);
							 }
							 glEnd();
							 
							 glLineWidth(2.0f);
							 glColor3f(0.5f, 0.5f, 0.6f);
							 glBegin(GL_LINES);
							 glVertex2f(-halfW, 0); glVertex2f(halfW, 0);
							 glVertex2f(0, -halfH); glVertex2f(0, halfH);
							 glEnd();
							 glLineWidth(1.0f);
							 
							 glColor3f(0.45f, 0.45f, 0.55f);
							 char buf[16];
							 for (int i = iniX; i <= finX; i++) {
								 if (i == 0 || i % 5 != 0) continue;
								 snprintf(buf, sizeof(buf), "%d", i);
								 drawText((float)i + 0.1f, 0.2f, buf);
							 }
							 for (int j = iniY; j <= finY; j++) {
								 if (j == 0 || j % 5 != 0) continue;
								 snprintf(buf, sizeof(buf), "%d", j);
								 drawText(0.15f, (float)j + 0.1f, buf);
							 }
						 }
						 
						 // ---------------------------------------------------------------------------
						 // RENDER DE UN SHAPE (Bresenham para contornos, ET/EAT para relleno)
						 // ---------------------------------------------------------------------------
						 void renderShape(const Shape& s) {
							 if (s.verts.empty()) return;
							 bool sel = s.selected;
							 
							 // Color del contorno (naranja si está seleccionado)
							 float cr = sel ? 1.0f  : s.r;
							 float cg = sel ? 0.65f : s.g;
							 float cb = sel ? 0.0f  : s.b;
							 
							 switch (s.type) {
								 
							 case SHAPE_POINT:
								 glColor3f(cr, cg, cb);
								 glPointSize(sel ? 9.0f : 7.0f);
								 glBegin(GL_POINTS);
								 glVertex2f(s.verts[0].x, s.verts[0].y);
								 glEnd();
								 glPointSize(1.0f);
								 break;
								 
							 case SHAPE_LINE:
								 if (s.verts.size() < 2) break;
								 glColor3f(cr, cg, cb);
								 bresenhamLine(s.verts[0], s.verts[1]);
								 for (const auto& v : s.verts) drawControlPoint(v);
								 break;
								 
							 case SHAPE_POLYLINE:
								 if (s.verts.size() < 2) break;
								 glColor3f(cr, cg, cb);
								 bresenhamContorno(s.verts, /*cerrado=*/false);
								 for (const auto& v : s.verts) drawControlPoint(v);
								 break;
								 
							 case SHAPE_POLYGON: {
								 if (s.verts.size() < 3) break;
								 
								 // Relleno con ET/EAT (solo si está activado)
								 if (s.filled) {
									 scanLineFill(s.verts, s.fr, s.fg, s.fb);
								 }
								 
								 // Contorno con Bresenham (incluye cierre Pn?P1)
								 glColor3f(cr, cg, cb);
								 bresenhamContorno(s.verts, /*cerrado=*/true);
								 
								 for (const auto& v : s.verts) drawControlPoint(v);
								 break;
							 }
							 
							 case SHAPE_CIRCLE: {
								 if (s.verts.size() < 2) break;
								 
								 // Aproximar el círculo con un polígono regular y reusar
								 // el mismo pipeline (Bresenham + ET/EAT) para ser consistente
								 // con la restricción de la práctica.
								 const int SEG = 48;
								 float radius = distancia(s.verts[0], s.verts[1]);
								 Vec2  c      = s.verts[0];
								 std::vector<Vec2> poly;
								 poly.reserve(SEG);
								 for (int i = 0; i < SEG; i++) {
									 float a = 2.f * PI * i / SEG;
									 poly.push_back(Vec2(c.x + radius * cosf(a),
														 c.y + radius * sinf(a)));
								 }
								 
								 if (s.filled) scanLineFill(poly, s.fr, s.fg, s.fb);
								 
								 glColor3f(cr, cg, cb);
								 bresenhamContorno(poly, /*cerrado=*/true);
								 
								 drawControlPoint(s.verts[0]);
								 drawControlPoint(s.verts[1]);
								 break;
							 }
							 }
							 glLineWidth(1.0f);
						 }
						 
						 // ---------------------------------------------------------------------------
						 // BARRA DE ESTADO
						 // ---------------------------------------------------------------------------
						 void dibujarBarraEstado() {
							 static const char* nombres[] = {
								 "SELECCION", "PUNTO", "LINEA", "POLILINEA", "POLIGONO", "CIRCULO"
							 };
							 float bx = ed.W * 0.5f / ed.scale;
							 float by = ed.H * 0.5f / ed.scale;
							 
							 glColor3f(0.22f, 0.22f, 0.25f);
							 glBegin(GL_QUADS);
							 glVertex2f(-bx, -by);
							 glVertex2f( bx, -by);
							 glVertex2f( bx, -by + 0.7f);
							 glVertex2f(-bx, -by + 0.7f);
							 glEnd();
							 
							 glColor3f(0.9f, 0.9f, 0.5f);
							 char buf[240];
							 snprintf(buf, sizeof(buf),
									  " Modo: %s  |  Objetos: %d  |  F1-F6:Herramienta  0/1-9:Seleccionar  Clic-Izq:Vert  Clic-Der:Cerrar",
									  nombres[(int)ed.modo], (int)ed.escena.size());
							 drawText(-bx + 0.2f, -by + 0.42f, buf);
							 
							 if (ed.selectedIdx >= 0) {
								 glColor3f(0.6f, 1.0f, 0.7f);
								 snprintf(buf, sizeof(buf),
										  " Activo #%d  |  Flechas:Mover  D/I:Rotar  S/s:Escalar  r,g,b:Relleno  f:Toggle  P:Rellenar  C:Limpiar  Del:Borrar",
										  ed.selectedIdx);
								 drawText(-bx + 0.2f, -by + 0.20f, buf);
							 }
						 }
						 
						 // ---------------------------------------------------------------------------
						 // PREVIEW (mientras se está construyendo)
						 // ---------------------------------------------------------------------------
						 void dibujarEnConstruccion() {
							 if (ed.tmpVerts.empty() && ed.modo != MODE_CIRCLE) return;
							 
							 glColor3f(ed.colorR, ed.colorG, ed.colorB);
							 
							 switch (ed.modo) {
							 case MODE_LINE:
								 if (ed.tmpVerts.size() == 1) {
									 bresenhamLine(ed.tmpVerts[0], ed.mouseWorld);
									 drawControlPoint(ed.tmpVerts[0]);
								 }
								 break;
								 
							 case MODE_POLYLINE:
								 if (!ed.tmpVerts.empty()) {
									 for (size_t i = 0; i + 1 < ed.tmpVerts.size(); i++)
										 bresenhamLine(ed.tmpVerts[i], ed.tmpVerts[i + 1]);
									 bresenhamLine(ed.tmpVerts.back(), ed.mouseWorld);
									 for (const auto& v : ed.tmpVerts) drawControlPoint(v);
								 }
								 break;
								 
							 case MODE_POLYGON:
								 if (!ed.tmpVerts.empty()) {
									 for (size_t i = 0; i + 1 < ed.tmpVerts.size(); i++)
										 bresenhamLine(ed.tmpVerts[i], ed.tmpVerts[i + 1]);
									 bresenhamLine(ed.tmpVerts.back(), ed.mouseWorld);
									 // Línea de cierre provisional
									 glColor3f(ed.colorR * 0.5f, ed.colorG * 0.5f, ed.colorB * 0.5f);
									 bresenhamLine(ed.mouseWorld, ed.tmpVerts[0]);
									 glColor3f(ed.colorR, ed.colorG, ed.colorB);
									 for (const auto& v : ed.tmpVerts) drawControlPoint(v);
								 }
								 break;
								 
							 case MODE_CIRCLE:
								 if (ed.tmpVerts.size() == 1) {
									 const int SEG = 48;
									 float radius = distancia(ed.tmpVerts[0], ed.mouseWorld);
									 Vec2  c      = ed.tmpVerts[0];
									 std::vector<Vec2> poly;
									 for (int i = 0; i < SEG; i++) {
										 float a = 2.f * PI * i / SEG;
										 poly.push_back(Vec2(c.x + radius * cosf(a),
															 c.y + radius * sinf(a)));
									 }
									 bresenhamContorno(poly, true);
									 drawControlPoint(ed.tmpVerts[0]);
								 }
								 break;
								 
							 default: break;
							 }
						 }
						 
						 // ---------------------------------------------------------------------------
						 // DISPLAY
						 // ---------------------------------------------------------------------------
						 void display() {
							 glClearColor(0.96f, 0.96f, 0.98f, 1.0f);
							 glClear(GL_COLOR_BUFFER_BIT);
							 glLoadIdentity();
							 
							 dibujarGrilla();
							 for (const auto& s : ed.escena) renderShape(s);
							 dibujarEnConstruccion();
							 dibujarBarraEstado();
							 
							 glutSwapBuffers();
						 }
