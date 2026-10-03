#pragma once
#include "editor.h"
#include <string>

void drawText(float wx, float wy, const std::string& txt);
void drawControlPoint(Vec2 p);
void drawCircle(Vec2 center, float radius, bool filled,
                float fr, float fg, float fb,
                float cr, float cg, float cb, bool selected);
void dibujarGrilla();
void renderShape(const Shape& s);
void dibujarBarraEstado();
void dibujarEnConstruccion();
void display();
