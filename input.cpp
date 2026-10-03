#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <cmath>
#include <algorithm>
#include <cstdlib>          // ? NUEVO: exit()
#include "input.h"
#include "editor.h"

// -- Paleta de colores ciclica (contorno) -------------------------------------
static const float kContornos[][3] = {
	{0.1f, 0.4f, 0.9f},   // azul
{0.9f, 0.2f, 0.1f},   // rojo
	{0.1f, 0.7f, 0.2f},   // verde
{0.9f, 0.55f, 0.0f},  // naranja
	{0.6f, 0.1f, 0.8f},   // violeta
{0.0f, 0.6f, 0.7f},   // cian
	{0.85f, 0.85f, 0.1f}, // amarillo
{0.1f, 0.1f, 0.1f},   // negro
};
static const int kNColores = 8;

static void aplicarColorContorno(int idx) {
	ed.colorIdx = (idx + kNColores) % kNColores;
	ed.colorR = kContornos[ed.colorIdx][0];
	ed.colorG = kContornos[ed.colorIdx][1];
	ed.colorB = kContornos[ed.colorIdx][2];
}

// -- Cierra el polígono en construcción (útil para clic derecho) --------------
static void cerrarPoligono() {
	if (ed.modo == MODE_POLYGON && ed.tmpVerts.size() >= 3) {
		Shape s(SHAPE_POLYGON, ed.colorR, ed.colorG, ed.colorB);
		s.fr = ed.fillR; s.fg = ed.fillG; s.fb = ed.fillB;
		s.verts = ed.tmpVerts;
		ed.escena.push_back(s);
		ed.tmpVerts.clear();
		// Seleccionar el recién creado automáticamente
		for (auto& sh : ed.escena) sh.selected = false;
		ed.escena.back().selected = true;
		ed.selectedIdx = (int)ed.escena.size() - 1;
	} else if (ed.modo == MODE_POLYLINE && ed.tmpVerts.size() >= 2) {
		Shape s(SHAPE_POLYLINE, ed.colorR, ed.colorG, ed.colorB);
		s.verts = ed.tmpVerts;
		ed.escena.push_back(s);
		ed.tmpVerts.clear();
	}
}

// -- Selección por distancia mínima -------------------------------------------
static void seleccionar(Vec2 p) {
	const float UMBRAL = 0.6f;
	int mejorIdx = -1;
	float mejorDist = UMBRAL;
	for (int i = 0; i < (int)ed.escena.size(); i++) {
		float d = distanciaShape(p, ed.escena[i]);
		if (d < mejorDist) { mejorDist = d; mejorIdx = i; }
	}
	for (auto& s : ed.escena) s.selected = false;
	ed.selectedIdx = mejorIdx;
	if (mejorIdx >= 0) ed.escena[mejorIdx].selected = true;
}

// -- Selecciona el N-ésimo polígono cerrado (1-based) -------------------------
static void seleccionarPoligonoN(int n) {
	int count = 0;
	int found = -1;
	for (int i = 0; i < (int)ed.escena.size(); i++) {
		if (ed.escena[i].type == SHAPE_POLYGON && ed.escena[i].verts.size() >= 3) {
			count++;
			if (count == n) { found = i; break; }
		}
	}
	for (auto& s : ed.escena) s.selected = false;
	ed.selectedIdx = found;
	if (found >= 0) {
		ed.escena[found].selected = true;
		ed.modo = MODE_SELECT;
	}
}

// -- Aplica color a un canal r/g/b al polígono activo -------------------------
static void setFillColorR() {
	ed.fillR = 1.f; ed.fillG = 0.f; ed.fillB = 0.f;
	if (ed.selectedIdx >= 0) {
		Shape& s = ed.escena[ed.selectedIdx];
		s.fr = ed.fillR; s.fg = ed.fillG; s.fb = ed.fillB;
	}
}
static void setFillColorG() {
	ed.fillR = 0.f; ed.fillG = 1.f; ed.fillB = 0.f;
	if (ed.selectedIdx >= 0) {
		Shape& s = ed.escena[ed.selectedIdx];
		s.fr = ed.fillR; s.fg = ed.fillG; s.fb = ed.fillB;
	}
}
static void setFillColorB() {
	ed.fillR = 0.f; ed.fillG = 0.f; ed.fillB = 1.f;
	if (ed.selectedIdx >= 0) {
		Shape& s = ed.escena[ed.selectedIdx];
		s.fr = ed.fillR; s.fg = ed.fillG; s.fb = ed.fillB;
	}
}

// -- Clic del mouse (izquierdo crea, derecho cierra) --------------------------
void mouse(int button, int state, int x, int y) {
	if (state != GLUT_DOWN) return;
	Vec2 p = screenToWorld(x, y);
	
	// CLIC DERECHO: cierra el polígono en construcción
	if (button == GLUT_RIGHT_BUTTON) {
		cerrarPoligono();
		glutPostRedisplay();
		return;
	}
	
	if (button != GLUT_LEFT_BUTTON) return;
	
	switch (ed.modo) {
	case MODE_SELECT:
		seleccionar(p);
		break;
		
	case MODE_POINT: {
		Shape s(SHAPE_POINT, ed.colorR, ed.colorG, ed.colorB);
		s.verts.push_back(p);
		ed.escena.push_back(s);
		break;
	}
	
	case MODE_LINE:
		ed.tmpVerts.push_back(p);
		if (ed.tmpVerts.size() == 2) {
			Shape s(SHAPE_LINE, ed.colorR, ed.colorG, ed.colorB);
			s.verts = ed.tmpVerts;
			ed.escena.push_back(s);
			ed.tmpVerts.clear();
		}
		break;
		
	case MODE_POLYLINE:
	case MODE_POLYGON:
		ed.tmpVerts.push_back(p);
		break;
		
	case MODE_CIRCLE:
		ed.tmpVerts.push_back(p);
		if (ed.tmpVerts.size() == 2) {
			Shape s(SHAPE_CIRCLE, ed.colorR, ed.colorG, ed.colorB);
			s.fr = ed.fillR; s.fg = ed.fillG; s.fb = ed.fillB;
			s.verts = ed.tmpVerts;
			ed.escena.push_back(s);
			ed.tmpVerts.clear();
		}
		break;
	}
	glutPostRedisplay();
}

void passiveMotion(int x, int y) {
	ed.mouseWorld = screenToWorld(x, y);
	glutPostRedisplay();
}

// -- Teclado ------------------------------------------------------------------
void keyboard(unsigned char key, int x, int y) {
	Shape* sel = (ed.selectedIdx >= 0) ? &ed.escena[ed.selectedIdx] : nullptr;
	
	switch (key) {
		
		// -- Seleccionar polígono por índice: 1..9 ----------------------------
	case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		seleccionarPoligonoN(key - '0');
		break;
		
		// -- Modo SELECT ------------------------------------------------------
	case '0':
		ed.modo = MODE_SELECT; ed.tmpVerts.clear();
		break;
		
		// -- Enter: cierra igual que clic derecho -----------------------------
	case 13:
		cerrarPoligono();
		break;
		
		// -- ESC: cancelar o salir --------------------------------------------
	case 27:
		if (!ed.tmpVerts.empty()) ed.tmpVerts.clear();
		else exit(0);
		break;
		
		// -- Rotación: D = derecha, I = izquierda -----------------------------
	case 'D': case ']': {
		if (!sel) break;
		Vec2 c = centroide(sel->verts);
		applyTransform(*sel, matRotateAround(c, -5.f * 3.14159265f / 180.f));
		break;
	}
	case 'I': case '[': {
		if (!sel) break;
		Vec2 c = centroide(sel->verts);
		applyTransform(*sel, matRotateAround(c,  5.f * 3.14159265f / 180.f));
		break;
	}
	
	// -- Escala: S = +10%, s = -10% ---------------------------------------
	case 'S': {
		if (!sel) break;
		Vec2 c = centroide(sel->verts);
		applyTransform(*sel, matScaleAround(c, 1.1f, 1.1f));
		break;
	}
	case 's': {
		if (!sel) break;
		Vec2 c = centroide(sel->verts);
		applyTransform(*sel, matScaleAround(c, 0.9f, 0.9f));
		break;
	}
	// Alias con q/z
	case 'q': {
		if (!sel) break;
		Vec2 c = centroide(sel->verts);
		applyTransform(*sel, matScaleAround(c, 1.1f, 1.1f));
		break;
	}
	case 'z': {
		if (!sel) break;
		Vec2 c = centroide(sel->verts);
		applyTransform(*sel, matScaleAround(c, 0.9f, 0.9f));
		break;
	}
	
	// -- Color de relleno directo: r, g, b --------------------------------
	case 'r': setFillColorR(); break;
	case 'g': setFillColorG(); break;
	case 'b': setFillColorB(); break;
	
	// -- Color de contorno ciclico (c) ------------------------------------
	case 'c':
		aplicarColorContorno(ed.colorIdx + 1);
		if (sel) { sel->r = ed.colorR; sel->g = ed.colorG; sel->b = ed.colorB; }
		break;
		
		// -- Toggle relleno y forzar relleno ----------------------------------
	case 'f':
		if (sel && (sel->type == SHAPE_POLYGON || sel->type == SHAPE_CIRCLE))
			sel->filled = !sel->filled;
		break;
	case 'P':
		if (sel && sel->type == SHAPE_POLYGON && sel->verts.size() >= 3)
			sel->filled = true;
		break;
		
		// -- Limpiar escena ---------------------------------------------------
	case 'C':
		ed.escena.clear();
		ed.tmpVerts.clear();
		ed.selectedIdx = -1;
		break;
		
		// -- Borrar seleccionado ----------------------------------------------
	case 127: case 8:
		if (sel) {
			ed.escena.erase(ed.escena.begin() + ed.selectedIdx);
			ed.selectedIdx = -1;
		}
		break;
	}
	glutPostRedisplay();
}

// -- Teclas especiales: flechas (traslación) y F1-F6 (herramientas) -----------
void specialKey(int key, int x, int y) {
	
	// F1..F6: cambio de herramienta (reemplaza a las viejas 1-5)
	switch (key) {
	case GLUT_KEY_F1: ed.modo = MODE_POINT;    ed.tmpVerts.clear(); glutPostRedisplay(); return;
	case GLUT_KEY_F2: ed.modo = MODE_LINE;     ed.tmpVerts.clear(); glutPostRedisplay(); return;
	case GLUT_KEY_F3: ed.modo = MODE_POLYLINE; ed.tmpVerts.clear(); glutPostRedisplay(); return;
	case GLUT_KEY_F4: ed.modo = MODE_POLYGON;  ed.tmpVerts.clear(); glutPostRedisplay(); return;
	case GLUT_KEY_F5: ed.modo = MODE_CIRCLE;   ed.tmpVerts.clear(); glutPostRedisplay(); return;
	case GLUT_KEY_F6: ed.modo = MODE_SELECT;   ed.tmpVerts.clear(); glutPostRedisplay(); return;
	}
	
	// Flechas: traslación
	if (ed.selectedIdx < 0) { glutPostRedisplay(); return; }
	const float PASO = 0.5f;
	Shape& s = ed.escena[ed.selectedIdx];
	Mat3 M;
	switch (key) {
	case GLUT_KEY_LEFT:  M = matTranslate(-PASO, 0.f);  break;
	case GLUT_KEY_RIGHT: M = matTranslate( PASO, 0.f);  break;
	case GLUT_KEY_UP:    M = matTranslate(0.f,  PASO);  break;
	case GLUT_KEY_DOWN:  M = matTranslate(0.f, -PASO);  break;
	default: return;
	}
	applyTransform(s, M);
	glutPostRedisplay();
}

// -- Reshape ------------------------------------------------------------------
void reshape(int w, int h) {
	if (h == 0) h = 1;
	ed.W = w; ed.H = h;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float halfW = w * 0.5f / ed.scale;
	float halfH = h * 0.5f / ed.scale;
	glOrtho(-halfW, halfW, -halfH, halfH, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
}
