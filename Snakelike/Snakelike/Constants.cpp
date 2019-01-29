#include <windows.h>
#include <string>

const std::string VERSION = "1.0.1";

const int NUM_OF_LEVELS = 20;
const int WIDTH = 49;
const int HEIGHT = 49;

const int ROOM_DIMENSION_MIN = 2;
const int ROOM_DIMENSION_MEAN = 8;
const int ROOM_DIMENSION_MAX = 10;
const int ROOM_DIMENSION_SD = 2;

const double ROOM_QUANTITY_MIN = 3;
const double ROOM_QUANTITY_MEAN = 9;
const double ROOM_QUANTITY_SD = 1;

const double CORRIDOR_WIDTH_MIN = 1;
const double CORRIDOR_WIDTH_MEAN = 2;
const double CORRIDOR_WIDTH_MAX = 4;
const double CORRIDOR_WIDTH_SD = 0.25;

// Values for increasing difficulty
const int HARD_LEVEL_START = NUM_OF_LEVELS / 2;
const int HARD_LEVEL_END = NUM_OF_LEVELS - 1;
const int HARD_BORDER_WIDTH = 3;
const int HARD_ROOM_QUANTITY_CHANGE = 3;
const int HARD_ROOM_DIMENSION_MEAN_CHANGE = -2;
const int HARD_CORRIDOR_WIDTH_MEAN_CHANGE = -1;

// Values for the bottom-most level
const int FINAL_ROOM_DIST_FROM_TOP = 8;
const int FINAL_ROOM_DIST_FROM_SIDES = 15;
const int FINAL_ROOM_HEIGHT = 19;
const int FINAL_CORRIDOR_DIST_FROM_BOTTOM = 3;
const int FINAL_CORRIDOR_WIDTH = 3;
const int GOLDEN_APPLE_GLOW_RADIUS = 5;
const int GOLDEN_APPLE_SCORE_MULTIPLIER = 2;
const int GOLDEN_APPLE_SPEED_INCREMENT = 10;

const int FOOD_MIN = 1;
const int FOOD_MAX = 3;
const int EXTRA_FOOD = 6;
const int POTION_HEAL_VALUE = 3;
const int POTION_SPAWN_CHANCE = 15;
const int DELAY_INCREMENT = 2;
const int START_SPEED = 30;
const int START_DELAY = 70;
const int IMMOBILIZE_DELAY = 300;
const int PARTICLE_UPDATE_DELAY = 5;
const int PARTICLE_BURST_TICKS = 80;

const int MAX_HEALTH = 15;
const int HEALTH_BAR_CHUNK_SIZE = 3;
const int MAX_VISION_RADIUS = 15;
const int MIN_VISION_RADIUS = 5;

const int MAX_RAND_ATTEMPTS = 100;

const int FOOD_POINTS = 100;
const int VICTORY_POINTS = 5000;

const unsigned short COLOUR_IMMOBILE = FOREGROUND_INTENSITY;
const unsigned short COLOUR_GOLDEN_APPLE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;

// Debugging variables
const bool DEBUG_VISION = false;
const bool DEBUG_MSGS = false;
const bool DEBUG_INVINCIBLE = false;