//#include "player.h"
#include "levels.cpp"
#include <random>
#include <queue>
#include <chrono>
#include <ctime>

enum PlayerState {
	ACTIVE = 1,
	IMMOBILE = 2,
	CONSUMING_APPLE = 3
};
PlayerState playerState = PlayerState::ACTIVE;

Point playerCoord(0, 0);
int health = MAX_HEALTH;
Level* currLevel; // Pointer to the level the player is on
Direction playerDir = Direction::NO_DIRECTION;
int currVisionRadius = MAX_VISION_RADIUS;
std::queue<Direction> moveQueue;
int queueCap = 3;
std::vector<Point> tail;
int tailLength = 0;
std::chrono::system_clock::time_point immobileStart;

int score = 0;
int foodEaten = 0;
bool hasGoldenApple = false;
bool escaped = false;
int speed = START_SPEED;
int delay = START_DELAY;
int collisions = 0;

void EnqueueMove(Direction dir) {
	if (moveQueue.size() < queueCap)
	{
		moveQueue.push(dir);
	}
}

void EraseTail() {
	for (auto &p : tail) {
		currLevel->entities[p.x][p.y] = Entity::NO_ENTITY;
	}
	tail.clear();
}

void ProcessLeaveLevel() {
	moveQueue = {};
	EraseTail();
}

void EatFood() {
	score += FOOD_POINTS;
	foodEaten++;
	tailLength++;
	if (!hasGoldenApple) {
		speed--;
		delay += DELAY_INCREMENT;
	}
}

void IncreaseHp(int value) {
	health += value;
	health = clamp(health, 0, MAX_HEALTH);
}

void UpdateVisionRadius(int levelNum) {
	if (!hasGoldenApple) {
		// Lose 1 vision every two levels
		currVisionRadius = max(MIN_VISION_RADIUS, MAX_VISION_RADIUS - round((double)(levelNum - 1) / 2));
	}
}

void EatGoldenApple() {
	foodEaten++;
	tailLength++;
	hasGoldenApple = true;
	score *= GOLDEN_APPLE_SCORE_MULTIPLIER;
	speed = START_SPEED + GOLDEN_APPLE_SPEED_INCREMENT;
	delay = START_DELAY - DELAY_INCREMENT * GOLDEN_APPLE_SPEED_INCREMENT;
	//currVisionRadius = MAX_VISION_RADIUS;
}

void SetImmobile() {
	playerState = PlayerState::IMMOBILE;
	immobileStart = std::chrono::system_clock::now();
}

void Collide() {
	health--;
	EraseTail();
	playerDir = Direction::NO_DIRECTION;
	SetImmobile();
	moveQueue = {};
}

void MovePlayerToStairs(Terrain stairsType) {
	if (stairsType == Terrain::STAIRS_UP) {
		playerCoord = *(currLevel->stairsUp);
	}
	else if (stairsType == Terrain::STAIRS_DOWN) {
		playerCoord = *(currLevel->stairsDown);
	}
	currLevel->entities[playerCoord.x][playerCoord.y] = Entity::HEAD;
}