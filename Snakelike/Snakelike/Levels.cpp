#include "constants.cpp"
#include "helper.cpp"

enum Terrain {
	UNKNOWN = 0,
	FLOOR = 1,
	WALL = 2,
	STAIRS_UP = 3,
	STAIRS_DOWN = 4,
	T_DEBUG = 5,
	TERRAIN_ITEMS = 6
};
std::string terrainImg[Terrain::TERRAIN_ITEMS] = { " ", ".", "#", "<", "V", "T" };
Terrain colourfulTerrain[] = { Terrain::STAIRS_UP, Terrain::STAIRS_DOWN };

enum Entity {
	NO_ENTITY = 0,
	FOOD = 1,
	HEAD = 2,
	TAIL = 3,
	GOLDEN_APPLE = 4,
	POTION = 5,
	E_DEBUG = 6,
	ENTITIY_ITEMS = 7
};
std::string entityImg[Entity::ENTITIY_ITEMS] = { " ", "@", "O", "O", "@", "%", "E" };
Entity colourfulEntities[] = { Entity::HEAD, Entity::GOLDEN_APPLE };

enum Particle {
	NO_PARTICLE = 0,
	NEW_SPARKLE = 1,
	SPARKLE = 2,
	SPARKLE_FINISHED = 3,
	PARTICLE_ITEMS = 4
};
std::string particleImg[Particle::PARTICLE_ITEMS] = { " ", "*", "*", " " };

enum Visibility {
	INVISIBLE = 0,
	VISIBLE = 1
};

enum Direction {
	NO_DIRECTION = 0,
	UP = 1,
	RIGHT = 2,
	DOWN = 3,
	LEFT = 4,
	NE = 5,
	SE = 6,
	SW = 7,
	NW = 8
};

struct Point {
	int x, y;
	Point() {
		this->x = 0;
		this->y = 0;
	}
	Point(int a, int b) {
		this->x = a;
		this->y = b;
	}
	bool operator==(const Point &other)
	{
		return this->x == other.x && this->y == other.y;
	}
	bool operator!=(const Point &other)
	{
		return !(*this == other);
	}
};

struct Room {
	Point originPoint;
	int width, height;
	Room(Point o, int w, int h) {
		this->originPoint = o;
		this->width = w;
		this->height = h;
	}
	void SetPoint(int x, int y) {
		this->originPoint = Point(x, y);
	}
};

struct Level {
	int levelNum = 0;
	int** terrain = 0;
	int** entities = 0;
	int** playerMap = 0;
	int** particles = 0;
	Point* stairsUp = NULL;
	Point* stairsDown = NULL;
	Level(int levelNum) {
		terrain = new int*[HEIGHT];
		for (int i = 0; i < HEIGHT; i++) {
			terrain[i] = new int[WIDTH];
			for (int j = 0; j < WIDTH; j++) {
				terrain[i][j] = Terrain::WALL;
			}
		}
		entities = new int*[HEIGHT];
		for (int i = 0; i < HEIGHT; i++) {
			entities[i] = new int[WIDTH];
			for (int j = 0; j < WIDTH; j++) {
				entities[i][j] = Entity::NO_ENTITY;
			}
		}
		playerMap = new int*[HEIGHT];
		for (int i = 0; i < HEIGHT; i++) {
			playerMap[i] = new int[WIDTH];
			for (int j = 0; j < WIDTH; j++) {
				if (!DEBUG_VISION) {
					playerMap[i][j] = Visibility::INVISIBLE;
				}
				else {
					playerMap[i][j] = Visibility::VISIBLE;
				}
			}
		}
		particles = new int*[HEIGHT];
		for (int i = 0; i < HEIGHT; i++) {
			particles[i] = new int[WIDTH];
			for (int j = 0; j < WIDTH; j++) {
				particles[i][j] = Particle::NO_PARTICLE;
			}
		}
		this->levelNum = levelNum;
	}
};

// Returns true iff the given point is on the level
bool CheckWithinBounds(Point p) {
	return p.x >= 0 && p.x < HEIGHT && p.y >= 0 && p.y < WIDTH;
}

bool CheckWithinBounds(int x, int y) {
	return x >= 0 && x < HEIGHT && y >= 0 && y < WIDTH;
}

// Returns true iff the given point lies within the outer edges of the level
bool CheckWithinEdges(Point p) {
	return p.x > 0 && p.x < HEIGHT - 1 && p.y > 0 && p.y < WIDTH - 1;
}

// Returns true iff p is a Floor with no Entity on it
bool IsEmptyFloor(Level* level, Point p) {
	if (!CheckWithinBounds(p)) {
		return false;
	}
	return level->terrain[p.x][p.y] == Terrain::FLOOR && level->entities[p.x][p.y] == Entity::NO_ENTITY;
}

bool IsPathway(Level* level, Point p) {
	if (!CheckWithinBounds(p)) {
		return false;
	}
	bool enterableEntity = level->entities[p.x][p.y] != Entity::TAIL;
	return level->terrain[p.x][p.y] == Terrain::FLOOR && enterableEntity;
}

bool IsTraversableTerrain(Level* level, Point p) {
	if (!CheckWithinBounds(p)) {
		return false;
	}
	bool traversableTerrain = level->terrain[p.x][p.y] == Terrain::FLOOR || level->terrain[p.x][p.y] == Terrain::STAIRS_UP || level->terrain[p.x][p.y] == Terrain::STAIRS_DOWN;
	return traversableTerrain;
}

bool IsTraversableTile(Level* level, Point p, Point* tailEnd) {
	bool traversableTerrain = IsTraversableTerrain(level, p);
	bool enterableEntity = level->entities[p.x][p.y] != Entity::TAIL;
	if (tailEnd) {
		enterableEntity |= Point(p.x, p.y) == *tailEnd;
	}
	return traversableTerrain && enterableEntity;
}

Point ApplyMovement(Point p, Direction dir) {
	if (dir == Direction::UP) {
		p.x -= 1;
	}
	else if (dir == Direction::DOWN) {
		p.x += 1;
	}
	else if (dir == Direction::LEFT) {
		p.y -= 1;
	}
	else if (dir == Direction::RIGHT) {
		p.y += 1;
	}
	else if (dir == Direction::NE) {
		p.x -= 1;
		p.y += 1;
	}
	else if (dir == Direction::SE) {
		p.x += 1;
		p.y += 1;
	}
	else if (dir == Direction::SW) {
		p.x += 1;
		p.y -= 1;
	}
	else if (dir == Direction::NW) {
		p.x -= 1;
		p.y -= 1;
	}
	return p;
}

// If toward is true, returns num, n closer to dest. Otherwise, returns num, n further away from dest.
int RelativeStep(int num, int n, int dest, bool toward) {
	if (toward) {
		if (num == dest) {
			return num;
		}
		return (num < dest) ? num + n : num - n;
	}
	else {
		return (num < dest) ? num - n : num + n;
	}
}

// Reveals a valid set of coordinates and returns true iff vision projection should continue through this tile.
bool AttemptTileReveal(Level* level, Point origin, Point p) {
	// Don't let vision pass between diagonal walls (unless this tile is a wall)
	if (level->terrain[p.x][p.y] != Terrain::WALL) {
		int xTowardsOrigin = RelativeStep(p.x, 1, origin.x, true);
		int yTowardsOrigin = RelativeStep(p.y, 1, origin.y, true);
		if (level->terrain[xTowardsOrigin][p.y] == Terrain::WALL && level->terrain[p.x][yTowardsOrigin] == Terrain::WALL) {
			return false;
		}
	}
	level->playerMap[p.x][p.y] = Visibility::VISIBLE;
	if (level->terrain[p.x][p.y] == Terrain::WALL) {
		return false;
	}
	return true;
}

// Reveal tiles on the player's map up to the given point, or stopping early if there is a wall in the way
void ProjectVisionLine(Level* level, Point origin, Point p) {
	float width = p.y - origin.y;
	float height = p.x - origin.x;
	float slope = 0;
	int longerSide = 0;
	int horiFlip = 1;
	int vertFlip = 1;
	bool widthLonger = abs(width) >= abs(height);
	if (widthLonger) {
		longerSide = abs(width);
		slope = fabs(height / width);
	}
	else {
		longerSide = abs(height);
		slope = fabs(width / height);
	}
	if (width < 0) {
		vertFlip = -1;
	}
	if (height > 0) {
		horiFlip = -1;
	}
	float currX = origin.x;
	float currY = origin.y;
	for (int i = 0; i < longerSide; i++) {
		if (widthLonger) {
			currX -= slope * horiFlip;
			currY += vertFlip;
		}
		else {
			currX -= horiFlip;
			currY += slope * vertFlip;

		}
		if (CheckWithinBounds(round(currX), round(currY))) {
			if (!AttemptTileReveal(level, origin, Point(round(currX), round(currY)))) {
				break;
			}
		}
	}
}

void ProjectVisionLinesFromPoint(Level* level, Point origin, int visionRadius) {
	level->playerMap[origin.x][origin.y] = Visibility::VISIBLE;
	for (int i = -visionRadius; i <= visionRadius; i++) {
		for (int j = -visionRadius; j <= visionRadius; j++) {
			if (abs(i) + abs(j) <= visionRadius) {
				if (CheckWithinBounds(origin.x + i, origin.y + j) && level->playerMap[origin.x + i][origin.y + j] != Visibility::VISIBLE) {
					ProjectVisionLine(level, origin, Point(origin.x + i, origin.y + j));
				}
			}
		}
	}
}

void UpdatePlayerMap(Level* level, Point origin, int visionRadius) {
	ProjectVisionLinesFromPoint(level, origin, visionRadius);
}

// Returns the percentage to increase difficulty given the level number.
double GetDifficultyPercent(int levelNum) {
	// Increase difficulty from floors 11-19
	if (HARD_LEVEL_START < levelNum && levelNum < HARD_LEVEL_END) {
		return (double)(levelNum - HARD_LEVEL_START) / (HARD_LEVEL_END - HARD_LEVEL_START);
	}
	return 0;
}