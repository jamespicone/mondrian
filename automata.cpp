#include "stdafx.h"
#include "automata.h"

#include <list>
#include <set>
#include <ctime>
#include <vector>
#include <thread>

static int width = 0;
static int height = 0;

static const GLuint WHITE = 0xffffffffu;
static const GLuint BLACK = 0;

static std::set<int> updateColumns;
static std::set<int> updateRows;

struct Border {
	Border(int _xstep, int _ystep, int _x, int _y)
		:xstep(_xstep), ystep(_ystep), x(_x), y(_y), started(true) {}

	int xstep;
	int ystep;

	int x;
	int y;

	bool started;
};

std::list<Border> borders;

GLuint** frontbuffer;
GLuint** backbuffer;

GLuint* gridLoc(int x, int y, GLuint** buffer) {
	return (GLuint*)(buffer + x + width * y);
}

GLuint handleNeighbours(int x, int y, GLuint color, GLuint** buffer) {
	if (color) {
		for (int xstep = x - 1; xstep < width && xstep < x + 2; ++xstep) {
			for (int ystep = y - 1; ystep < height && ystep < y + 2; ++ystep) {
				if (xstep < 0 || ystep < 0 || (xstep == 0 && ystep == 0)) { continue; }

				GLuint curcolor = *gridLoc(xstep, ystep, buffer);

				if (color == WHITE && curcolor != BLACK) { color = curcolor; }
				if (color != curcolor && curcolor != WHITE && curcolor != BLACK) { color = BLACK; }
			}
		}
	}
	else {
		return color;
	}

	if (!color)
	{
		for (int xstep = -1; x + xstep < width && xstep < 2; ++xstep) {
			for (int ystep = -1; y + ystep < height && ystep < 2; ++ystep) {
				if (x + xstep < 0 || y + ystep < 0 || (xstep == 0 && ystep == 0) || (xstep != 0 && ystep != 0)) {
					continue;
				}

				GLuint curcolor = *gridLoc(x + xstep, y + ystep, buffer);

				if (curcolor == WHITE) { borders.push_back(Border(xstep, ystep, x, y)); }
			}
		}
	}

	return color;
}

void flipBuffers() {
	memcpy(backbuffer, frontbuffer, width * height * sizeof(GLuint));

	std::swap(frontbuffer, backbuffer);
}

void Update() {
	if (!borders.empty()) {
		for (auto i = borders.begin(); i != borders.end();) {
			i->x += i->xstep;
			i->y += i->ystep;

			if (i->x > -1 && i->x < width && i->y > -1 && i->y < height) {
				GLuint step = *gridLoc(i->x, i->y, backbuffer);

				if (step == WHITE || (i->started && step != BLACK)) {
					if (step == WHITE) { i->started = false; }

					*gridLoc(i->x, i->y, frontbuffer) = BLACK;
					*gridLoc(i->x, i->y, backbuffer) = BLACK;
					continue;
				}
				else {
					if (step != BLACK) {
						i->x -= i->xstep;
						i->y -= i->ystep;

						int xstep = i->xstep ? 0 : 1;
						int ystep = i->ystep ? 0 : 1;

						borders.insert(borders.end(), Border(xstep, ystep, i->x, i->y));
						borders.insert(borders.end(), Border(-xstep, -ystep, i->x, i->y));
					}
				}
			}

			i = borders.erase(i);
		}
	}

	std::set<int> oldcolumns, oldrows;
	oldcolumns.swap(updateColumns);
	oldrows.swap(updateRows);

	for (auto i = oldrows.cbegin(); i != oldrows.cend(); ++i) {
		for (int x = 0; x < width; ++x) {
			*gridLoc(x, *i, frontbuffer) = handleNeighbours(x, *i, *gridLoc(x, *i, frontbuffer), backbuffer);
		}
	}

	for (auto i = oldcolumns.cbegin(); i != oldcolumns.cend(); ++i) {
		for (int y = 0; y < height; ++y) {
			*gridLoc(*i, y, frontbuffer) = handleNeighbours(*i, y, *gridLoc(*i, y, frontbuffer), backbuffer);
		}
	}

	for (auto i = oldrows.cbegin(); i != oldrows.cend(); ++i) {
		for (int x = 0; x < width; ++x) {
			GLuint curcolour = *gridLoc(x, *i, frontbuffer);

			if (curcolour != *gridLoc(x, *i, backbuffer)) {
				if (*i > 0 && *gridLoc(x, *i - 1, frontbuffer) != curcolour) {
					updateRows.insert(*i - 1);
				}

				if (*i < height - 1 && *gridLoc(x, *i + 1, frontbuffer) != curcolour) {
					updateRows.insert(*i + 1);
				}
			}
		}
	}

	for (auto i = oldcolumns.cbegin(); i != oldcolumns.cend(); ++i) {
		for (int y = 0; y < height; ++y) {
			GLuint curcolour = *gridLoc(*i, y, frontbuffer);

			if (curcolour != *gridLoc(*i, y, backbuffer)) {
				if (*i > 0 && *gridLoc(*i - 1, y, frontbuffer) != curcolour) {
					updateColumns.insert(*i - 1);
				}

				if (*i < width - 1 && *gridLoc(*i + 1, y, frontbuffer) != curcolour) {
					updateColumns.insert(*i + 1);
				}
			}
		}
	}

	flipBuffers();
}

void placeColor(GLuint c) {
	int x = int((width - 1) * std::rand() / double(RAND_MAX));
	int y = int((height - 1) * std::rand() / double(RAND_MAX));

	if (x > 0) { updateColumns.insert(x - 1); }
	if (x < width - 1) { updateColumns.insert(x + 1); }

	if (y > 0) { updateRows.insert(y - 1); }
	if (y < height - 1) { updateRows.insert(y + 1); }

	*gridLoc(x, y, backbuffer) = c;
}

void InitAutomata() {
	memset(frontbuffer, WHITE, height * width * sizeof(GLuint));
	memset(backbuffer, WHITE, height * width * sizeof(GLuint));

	int numColors = int(std::sqrt(max(height, width)) / 2);
	int colourRepeats = 3;

	for (int j = 0; j < colourRepeats; ++j) {
		std::uint32_t newColour = static_cast<std::uint32_t>(std::rand());

		for (int i = 0; i < numColors / colourRepeats; ++i) {
			placeColor(newColour);
		}
	}

	for (int i = 0; i < numColors; ++i) {
		placeColor(0x00ffffff);
	}
}

void SetupAutomata(int _width, int _height) {
	width = _width;
	height = _height;

	std::srand((unsigned int)std::time(nullptr));

	gluOrtho2D(0, width, 0, height);

	frontbuffer = (GLuint**)calloc(height * width, sizeof(GLuint));
	backbuffer = (GLuint**)calloc(height * width, sizeof(GLuint));

	InitAutomata();
}

void ResetAutomata() {
	InitAutomata();
}

void TickAutomata() {
	if (updateRows.empty() && updateColumns.empty()) {
		InitAutomata();
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glRasterPos2i(0, 0);
	glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)frontbuffer);

	GLenum error = glGetError();
	if (error != 0) { std::exit(error); }

	Update();
};