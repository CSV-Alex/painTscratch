#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include "editor.h"
#include "render.h"
#include "input.h"

EditorState ed;

Vec2 screenToWorld(int mx, int my) {
	return Vec2(
				(mx - ed.W * 0.5f) / ed.scale,
				-(my - ed.H * 0.5f) / ed.scale
				);
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(ed.W, ed.H);
	glutCreateWindow("Proyecto painT");
	
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKey);
	glutPassiveMotionFunc(passiveMotion);
	
	glutMainLoop();
	return 0;
}
