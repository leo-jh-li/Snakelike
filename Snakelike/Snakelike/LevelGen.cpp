#include "player.cpp"
using namespace std;

std::random_device rd;
std::mt19937 gen(rd());

int FOOD_DISTRIBUTION[NUM_OF_LEVELS - 1] { 0 };

static int generatedLevelsCount = 0;

Level* levelStorage[NUM_OF_LEVELS] {0};

// for debugging
/*#include <iostream>
short printIndex = 55;
void DebugPrint(string s) {
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0,printIndex });
	cout << s << "\n";
	printIndex++;
}
void DebugPrint(int i) {
	DebugPrint(to_string(i));
}*/

void RandomizeFoodDistribution() {
	std::fill_n(FOOD_DISTRIBUTION, NUM_OF_LEVELS - 1, FOOD_MIN);
	int foodToDistribute = EXTRA_FOOD;
	int attempts = 0;
	while (foodToDistribute > 0 && attempts < MAX_RAND_ATTEMPTS) {
		int levelIndex = rand() % (NUM_OF_LEVELS - 1);
		if (FOOD_DISTRIBUTION[levelIndex] < FOOD_MAX) {
			FOOD_DISTRIBUTION[levelIndex]++;
			foodToDistribute--;
		}
		attempts++;
	}
}


// Make a vertical path between Point a and Point b on given map.
void MakeVerticalPath(int** map, Point* a, Point* b) {
	for (int i = min(a->x, b->x); i <= max(a->x, b->x); i++) {
		if (map[i][a->y] == Terrain::WALL) {
			map[i][a->y] = Terrain::FLOOR;
		}
	}
}

void MakeHorizontalPath(int** map, Point* a, Point* b) {
	for (int i = min(a->y, b->y); i <= max(a->y, b->y); i++) {
		if (map[a->x][i] == Terrain::WALL) {
			map[a->x][i] = Terrain::FLOOR;
		}
	}
}

// Creates a path, one tile wide, connecting Point a to Point corner and Point pathCorner to Point b.
void ConnectPoints(int** map, Point* a, Point* b, Point* pathCorner) {
	MakeVerticalPath(map, (a->y == pathCorner->y) ? a : b, pathCorner);
	MakeHorizontalPath(map, (a->x == pathCorner->x) ? a : b, pathCorner);
}

Point GetRandomTileInRoom(Room room) {
	int x = room.originPoint.x + round(rand() % room.height);
	int y = room.originPoint.y + round(rand() % room.width);
	return Point(x, y);
}

Point GetRandomEdgeFloorInRoom(Level* level, Room room) {
	int i = 0;
	Point p(0, 0);
	int attempts = 0;
	do {
		if (rand() > RAND_MAX / 2) {
			p.x = room.originPoint.x + round(rand() % room.height);
			p.y = (rand() > RAND_MAX / 2) ? room.originPoint.y : room.originPoint.y + (room.width - 1);
		}
		else {
			p.x = (rand() > RAND_MAX / 2) ? room.originPoint.x : room.originPoint.x + (room.height - 1);
			p.y = room.originPoint.y + round(rand() % room.width);
		}
		attempts++;
	} while (!IsEmptyFloor(level, p) && attempts < MAX_RAND_ATTEMPTS);
	return p;
}

// Return true iff blocking the Floor at p will block p's bordering Floors from accessing each other or if it's adjacent to an entity
bool VitalCrossroads(Level* level, Point p) {
	// Vector of adjacent points that border p
	vector<Point> neighbours;
	for (int dir = Direction::UP; dir <= Direction::LEFT; dir++) {
		Point neighbour = ApplyMovement(p, (Direction)dir);
		if (IsTraversableTerrain(level, neighbour)) {
			if (!IsEmptyFloor(level, neighbour)) {
				return true;
			}
			neighbours.push_back(neighbour);
		}
	}
	if (neighbours.size() == 1) {
		return false;
	}
	if (neighbours.size() == 2 || neighbours.size() == 3) {
		vector<Point> requiredFloors;
		for (int i = 0; i < neighbours.size(); i++) {
			for (int j = 0; j < neighbours.size(); j++) {
				if (i != j) {
					requiredFloors.push_back(Point(neighbours[i].x, neighbours[j].y));
				}
			}
		}
		// Case where p might be in a 1 tile wide hallway - means this is a vital crossroads
		if (neighbours.size() == 2) {
			if (neighbours[0].x == neighbours[1].x || neighbours[0].y == neighbours[1].y) {
				return true;
			}
		}
		for (int i = 0; i < requiredFloors.size(); i++) {
			if (!IsEmptyFloor(level, Point(requiredFloors[i]))) {
				return true;
			}
		}
		return false;
	}
	if (neighbours.size() == 4) {
		int numDiagonalFloors = 0;
		for (int dir = Direction::NE; dir <= Direction::NW; dir++) {
			if (IsEmptyFloor(level, ApplyMovement(p, (Direction)dir))) {
				numDiagonalFloors++;
			}
		}
		return numDiagonalFloors < 3;
	}
	return true;
}

bool IsNonVitalFloor(Level* level, Point p) {
	return IsEmptyFloor(level, p) && !VitalCrossroads(level, p);
}

// Returns the point of an Entity-less Floor in the given room that would not block nearby pathways if it had any obstructive terrain on entities on it
Point GetNonVitalFloor(Level* level, Room room) {
	int i = 0;
	Point p(0, 0);
	int attempts = 0;
	do {
		p = GetRandomTileInRoom(room);
		attempts++;
	} while (!IsNonVitalFloor(level, p) && attempts < MAX_RAND_ATTEMPTS);
	return p;
}

// Creates an arbitrary path, w tiles wide, from Point a to Point b on level->terrain.
void GenerateCorridor(Level* level, int w, Point* a, Point* b) {
	if (a == b) {
		return;
	}

	// Place the corner of this path at one of the two points' "intersections" at random
	int pathCornerX = (rand() > RAND_MAX / 2) ? a->x : b->x;
	int pathCornerY = (pathCornerX == a->x) ? b->y : a->y;
	Point pathCorner = Point(pathCornerX, pathCornerY);

	// The widths of this path diagonally inwards and diagonally outwards of this path corner
	int extensionInwards = 0;
	int extensionOutwards = 0;
	ConnectPoints(level->terrain, a, b, &pathCorner);
	w -= 1;

	// Extend the width of this path
	while (w > 0) {
		// Randomly decide whether to extend the path on the inside or outside...
		bool inwards = (rand() > RAND_MAX / 2) ? true : false;
		// ...unless one of the directions has been marked as invalid
		if (extensionInwards < 0) {
			inwards = false;
		}
		else if (extensionOutwards < 0) {
			inwards = true;
		}
		int currExtension = inwards ? ++extensionInwards : ++extensionOutwards;
		int newPathCornerX = RelativeStep(pathCornerX, currExtension, (pathCornerX == a->x) ? b->x : a->x, inwards);
		int newPathCornerY = RelativeStep(pathCornerY, currExtension, (pathCornerY == a->y) ? b->y : a->y, inwards);
		Point newPointA = Point(a->x, a->y);
		Point newPointB = Point(b->x, b->y);
		if (pathCornerX == a->x) {
			newPointA.x = RelativeStep(a->x, currExtension, b->x, inwards);
			newPointB.y = RelativeStep(b->y, currExtension, a->y, inwards);
		}
		else {
			newPointB.x = RelativeStep(b->x, currExtension, a->x, inwards);
			newPointA.y = RelativeStep(a->y, currExtension, b->y, inwards);
		}

		// Validate the prospective path
		Point newCorner = Point(newPathCornerX, newPathCornerY);
		if (CheckWithinEdges(newCorner)) {
			ConnectPoints(level->terrain, &newPointA, &newPointB, &newCorner);
			w -= 1;
		}
		else {
			// Mark the corresponding direction as invalid for future path generation
			inwards ? extensionInwards = -1 : extensionOutwards = -1;
		}
		// Stop if this corridor can expand no more
		if (extensionInwards < 0 && extensionOutwards < 0) {
			break;
		}
	}
}

void CreateCorridorBetweenRooms(Level* level, Room r1, Room r2) {
	// Get random points on the edges of the rooms
	Point* a = &GetRandomEdgeFloorInRoom(level, r1);
	Point* b = &GetRandomEdgeFloorInRoom(level, r2);
	int corridorWidthMeanChange = round(GetDifficultyPercent(level->levelNum) * HARD_CORRIDOR_WIDTH_MEAN_CHANGE);
	std::normal_distribution<double> d(CORRIDOR_WIDTH_MEAN + corridorWidthMeanChange, CORRIDOR_WIDTH_SD);
	int corridorWidth = round(d(gen));
	corridorWidth = clamp(corridorWidth, CORRIDOR_WIDTH_MIN, CORRIDOR_WIDTH_MAX);
	GenerateCorridor(level, corridorWidth, a, b);
}

void GenerateLevel(Level* level) {
	// Create rooms
	double difficultyPercent = GetDifficultyPercent(level->levelNum);
	int border = max(1, round(difficultyPercent * HARD_BORDER_WIDTH));
	int dimensionChange = round(difficultyPercent * HARD_ROOM_DIMENSION_MEAN_CHANGE);
	int roomQuantityChange = round(difficultyPercent * HARD_ROOM_QUANTITY_CHANGE);
	std::normal_distribution<double> d(ROOM_QUANTITY_MEAN, ROOM_QUANTITY_SD);
	double roomsToGenerate = round(d(gen));
	roomsToGenerate = max(roomsToGenerate, ROOM_QUANTITY_MIN);
	int currRoomCount = 0;
	int x = 0;
	int y = 0;
	int height;
	int width;
	Room* prevRoom = NULL;
	vector<Room> rooms;
	while (currRoomCount < roomsToGenerate) {
		// Choose the upper left corner for this room
		x = rand() % (HEIGHT - 1 - border * 2) + border;
		y = rand() % (WIDTH - 1 - border * 2) + border;
		std::normal_distribution<double> d(ROOM_DIMENSION_MEAN, ROOM_DIMENSION_SD);
		height = round(d(gen)) + dimensionChange;
		width = round(d(gen)) + dimensionChange;
		height = clamp(height, ROOM_DIMENSION_MIN, ROOM_DIMENSION_MAX);
		width = clamp(width, ROOM_DIMENSION_MIN, ROOM_DIMENSION_MAX);

		// Discard this room if it crosses the boundaries
		if (!CheckWithinEdges(Point(x + height, y + width))) {
			continue;
		}

		// Create the room and place its floors onto this level
		for (int i = x; i < x + height; i++) {
			for (int j = y; j < y + width; j++) {
				if (level->terrain[i][j] == Terrain::WALL) {
					level->terrain[i][j] = Terrain::FLOOR;
				}
			}
		}
		rooms.push_back(Room(Point(x, y), width, height));

		// Create a corridor connecting this room to the previous room generated, if there is one
		if (rooms.size() > 1) {
			CreateCorridorBetweenRooms(level, rooms[currRoomCount], rooms[currRoomCount - 1]);
		}
		prevRoom = &Room(Point(x, y), width, height);
		currRoomCount++;
	}
	// Create more corridors on more difficult floors
	if (difficultyPercent > 0) {
		// Connect the final room to the first room
		CreateCorridorBetweenRooms(level, rooms[rooms.size() - 1], rooms[0]);
		// Connect the final room to a random room that's not the first one
		CreateCorridorBetweenRooms(level, rooms[rooms.size() - 1], rooms[round(rand() % (rooms.size() - 2) + 1)]);
		// Connect the first room to a random room that's not the first one
		CreateCorridorBetweenRooms(level, rooms[0], rooms[round(rand() % (rooms.size() - 2) + 1)]);
	}
	// Spawn potions in random rooms
	int potionsToSpawn = (rand() % 100 < POTION_SPAWN_CHANCE) ? 1 : 0;
	int potionsSpawned = 0;
	while (potionsSpawned < potionsToSpawn) {
		Point potionPoint = GetNonVitalFloor(level, rooms[round(rand() % rooms.size())]);
		level->entities[potionPoint.x][potionPoint.y] = Entity::POTION;
		potionsSpawned++;
	}
	// Pick two different rooms and place the up and down stairs in them
	int upRoomIndex = round(rand() % rooms.size());
	int downRoomIndex = 0;
	int attempts = 0;
	do {
		downRoomIndex = round(rand() % rooms.size());
		attempts++;
	} while (downRoomIndex == upRoomIndex && attempts < MAX_RAND_ATTEMPTS);
	Point stairs = GetNonVitalFloor(level, rooms[upRoomIndex]);
	level->terrain[stairs.x][stairs.y] = Terrain::STAIRS_UP;
	level->stairsUp = new Point(stairs.x, stairs.y);
	stairs = GetNonVitalFloor(level, rooms[downRoomIndex]);
	level->terrain[stairs.x][stairs.y] = Terrain::STAIRS_DOWN;
	level->stairsDown = new Point(stairs.x, stairs.y);
	
	// Place food
	int foodToPlace = FOOD_DISTRIBUTION[level->levelNum - 1];
	attempts = 0;
	while (foodToPlace > 0 && attempts < MAX_RAND_ATTEMPTS) {
		Point food = GetNonVitalFloor(level, rooms[rand() % rooms.size()]);
		if (IsNonVitalFloor(level, food)) {
			level->entities[food.x][food.y] = Entity::FOOD;
			foodToPlace -= 1;
		}
		attempts++;
	}
}

// Creates a new level and moves the player to its (up) stairs
void AddNewLevel() {
	Level* newLevel = new Level(++generatedLevelsCount);
	GenerateLevel(newLevel);
	levelStorage[generatedLevelsCount - 1] = newLevel;
}

void AddLastLevel() {
	Level* newLevel = new Level(++generatedLevelsCount);
	
	// Create central room
	int finalRoomWidth = WIDTH - FINAL_ROOM_DIST_FROM_SIDES*2;
	for (int i = FINAL_ROOM_DIST_FROM_TOP; i < FINAL_ROOM_DIST_FROM_TOP + FINAL_ROOM_HEIGHT; i++) {
		for (int j = FINAL_ROOM_DIST_FROM_SIDES; j < FINAL_ROOM_DIST_FROM_SIDES + finalRoomWidth; j++) {
			if (newLevel->terrain[i][j] == Terrain::WALL) {
				newLevel->terrain[i][j] = Terrain::FLOOR;
			}
		}
	}
	int roomCentreY = FINAL_ROOM_DIST_FROM_SIDES + floor((double)finalRoomWidth / 2);
	// Create corridor to that room
	for (int i = HEIGHT - finalRoomWidth - FINAL_ROOM_DIST_FROM_TOP; i < HEIGHT - FINAL_CORRIDOR_DIST_FROM_BOTTOM; i++) {
		for (int j = roomCentreY - floor((double)FINAL_CORRIDOR_WIDTH / 2); j < roomCentreY + FINAL_CORRIDOR_WIDTH - 1; j++) {
			if (newLevel->terrain[i][j] == Terrain::WALL) {
				newLevel->terrain[i][j] = Terrain::FLOOR;
			}
		}
	}
	// Place stairs up in centre of corridor
	int stairsX = HEIGHT - FINAL_CORRIDOR_DIST_FROM_BOTTOM - 1;
	int stairsY = roomCentreY - floor((double)FINAL_CORRIDOR_WIDTH / 2) + 1;
	newLevel->terrain[stairsX][stairsY] = Terrain::STAIRS_UP;
	newLevel->stairsUp = new Point(stairsX, stairsY);
	// Place golden apple
	int goldAppleX = FINAL_ROOM_DIST_FROM_TOP + floor((double)FINAL_ROOM_HEIGHT / 2);
	newLevel->entities[goldAppleX][stairsY] = Entity::GOLDEN_APPLE;
	// Add glow effect around golden apple
	ProjectVisionLinesFromPoint(newLevel, Point(goldAppleX, stairsY), GOLDEN_APPLE_GLOW_RADIUS);
	levelStorage[generatedLevelsCount - 1] = newLevel;
}