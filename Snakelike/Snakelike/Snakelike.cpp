#include "levelgen.cpp"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cwchar>
#include <wchar.h>
#include <conio.h>

enum GameMode {
	MENU = 1,
	PLAYING = 2,
	GAME_OVER = 3,
	EXIT = 4
};
GameMode gameMode = GameMode::MENU;
std::chrono::system_clock::time_point startTime;
std::chrono::system_clock::time_point endTime;

enum GameSize {
	SMALL = 1,
	LARGE = 2
};
GameSize gameSize = GameSize::SMALL;

void Draw();

string screenDisplay;
string gameMargin = "";
int screenCols = 0;

void EraseDisplay()
{
	COORD corner = { 0, 0 };
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO sbInfo;
	DWORD written;
	GetConsoleScreenBufferInfo(console, &sbInfo);
	FillConsoleOutputCharacterA(
		console, ' ', sbInfo.dwSize.X * sbInfo.dwSize.Y, corner, &written
	);
	FillConsoleOutputAttribute(
		console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
		sbInfo.dwSize.X * sbInfo.dwSize.Y, corner, &written
	);
	SetConsoleCursorPosition(console, corner);
}

void SetConsoleColour(unsigned short colour)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), colour);
}

void ResetConsoleColour()
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
}

void UpdateGameSize() {
	EraseDisplay();
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	HWND console = GetConsoleWindow();
	RECT rect;
	GetWindowRect(console, &rect);
	CONSOLE_SCREEN_BUFFER_INFO sbInfo;
	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.FontFamily = FF_DONTCARE;
	wcscpy_s(cfi.FaceName, L"Consolas");
	if (gameSize == GameSize::SMALL) {
		cfi.dwFontSize.X = 14;
		cfi.dwFontSize.Y = 14;
		SetCurrentConsoleFontEx(out, FALSE, &cfi);
		SetConsoleDisplayMode(out, CONSOLE_WINDOWED_MODE, NULL);
		MoveWindow(console, rect.left, rect.top, 770, 820, TRUE);
	}
	else if (gameSize == GameSize::LARGE) {
		cfi.dwFontSize.X = 18;
		cfi.dwFontSize.Y = 18;
		SetCurrentConsoleFontEx(out, TRUE, &cfi);
		SetConsoleDisplayMode(out, CONSOLE_FULLSCREEN_MODE, NULL);
	}
	GetConsoleScreenBufferInfo(out, &sbInfo);
	screenCols = (sbInfo.srWindow.Right - sbInfo.srWindow.Left + 1);
	int gameMarginSize = (sbInfo.srWindow.Right - sbInfo.srWindow.Left + 1 - WIDTH) / 2;
	gameMargin.assign(gameMarginSize, ' ');

	// Disable resizing
	SetWindowLong(console, GWL_STYLE, GetWindowLong(console, GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);

	// Disable cursor
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(out, &cursorInfo);
	cursorInfo.bVisible = false;
	cursorInfo.dwSize = 1;
	SetConsoleCursorInfo(out, &cursorInfo);

	// Remove the scrollbar
	GetConsoleScreenBufferInfo(out, &sbInfo);
	int status;
	COORD scrollbar = {
			sbInfo.srWindow.Right - sbInfo.srWindow.Left + 1,
			sbInfo.srWindow.Bottom - sbInfo.srWindow.Top + 1
	};
	status = SetConsoleScreenBufferSize(out, scrollbar);
	if (status == 0)
	{
		status = GetLastError();
		cout << "Error removing scrollbar : " << status << endl;
		exit(status);
	}
	Draw();
}

void ToggleGameSize() {
	if (gameSize == GameSize::SMALL) {
		gameSize = GameSize::LARGE;
	}
	else if (gameSize == GameSize::LARGE)
	{
		gameSize = GameSize::SMALL;
	}
	UpdateGameSize();
}

void SetConsoleSettings() {
	SetConsoleTitle(L"Snakelike");
	UpdateGameSize();
}

void Setup()
{
	SetConsoleSettings();
	srand(time(NULL));
}

void ReinitializeValues() {
	for (auto &l : levelStorage) {
		if (l != 0) {
			free(l);
		}
	}
	generatedLevelsCount = 0;
	health = MAX_HEALTH;
	playerDir = Direction::NO_DIRECTION;
	currVisionRadius = MAX_VISION_RADIUS;
	moveQueue = {};
	EraseTail();
	tailLength = 0;
	score = 0;
	foodEaten = 0;
	hasGoldenApple = false;
	escaped = false;
	speed = START_SPEED;
	delay = START_DELAY;
	collisions = 0;
}

void StartGame() {
	ReinitializeValues();
	RandomizeFoodDistribution();
	AddNewLevel();
	currLevel = levelStorage[0];
	MovePlayerToStairs(Terrain::STAIRS_UP);
	UpdatePlayerMap(currLevel, playerCoord, currVisionRadius);
	gameMode = GameMode::PLAYING;
	startTime = std::chrono::system_clock::now();
}

void EndGame() {
	
	if (DEBUG_INVINCIBLE) {
		return;
	}
	gameMode = GameMode::GAME_OVER;
	endTime = std::chrono::system_clock::now();
}

void ResetCursor()
{
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { 0, 0 });
}

void PropagateParticle(Point p) {
	currLevel->particles[p.x][p.y] = Particle::SPARKLE_FINISHED;
	for (int dir = Direction::UP; dir <= Direction::LEFT; dir++) {
		Point neighbour = ApplyMovement(p, (Direction)dir);
		if (CheckWithinBounds(neighbour.x, neighbour.y) && currLevel->particles[neighbour.x][neighbour.y] == Particle::NO_PARTICLE) {
			currLevel->particles[neighbour.x][neighbour.y] = Particle::NEW_SPARKLE;
		}
	}
}

void ParticleBurst(Point p) {
	currLevel->particles[p.x][p.y] = Particle::NEW_SPARKLE;
	for (int i = 0; i < PARTICLE_BURST_TICKS; i++) {
		Draw();
		Sleep(PARTICLE_UPDATE_DELAY);
		// Age all new particles
		for (int x = 0; x < HEIGHT; x++) {
			for (int y = 0; y < WIDTH; y++) {
				if (currLevel->particles[x][y] == Particle::NEW_SPARKLE) {
					currLevel->particles[x][y] = Particle::SPARKLE;
				}
			}
		}
		// Propagate all particles
		for (int x = 0; x < HEIGHT; x++) {
			for (int y = 0; y < WIDTH; y++) {
				if (currLevel->particles[x][y] == Particle::SPARKLE) {
					PropagateParticle(Point(x, y));
				}
			}
		}
		// Increase vision radius alongside particle burst
		if (currVisionRadius < min(i + 1, MAX_VISION_RADIUS)) {
			currVisionRadius = min(i + 1, MAX_VISION_RADIUS);
			UpdatePlayerMap(currLevel, playerCoord, currVisionRadius);
		}
	}
	FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
}

string ApplyTextSpacing(string str) {
	string margin = "";
	int printCol = floor(screenCols / 2) - str.length()/2;
	for (int i = 0; i < printCol; i++) {
		margin += " ";
	}
	return margin + str;
}

void DrawMenu() {
	ResetCursor();
	screenDisplay.clear();
	screenDisplay += "\n";
	screenDisplay += ApplyTextSpacing("+-----------+") + "\n";
	screenDisplay += ApplyTextSpacing("| SNAKELIKE |") + "\n";
	screenDisplay += ApplyTextSpacing("+-----------+") + "\n";
	screenDisplay += ApplyTextSpacing("v" + VERSION) + "\n";
	screenDisplay += ApplyTextSpacing("made by Leo Li") + "\n\n";
	screenDisplay += ApplyTextSpacing("Press enter to start.") + "\n\n";
	screenDisplay += ApplyTextSpacing("Press G to change game size.") + "\n";
	screenDisplay += ApplyTextSpacing("ESC to exit.") + "\n";
	screenDisplay += "\n\n" + ApplyTextSpacing("Legend") + "\n\n";

	string menuMargin = "";
	menuMargin.assign(screenCols / 5, ' ');

	screenDisplay += ApplyTextSpacing("      ooO You!                      \n");
	screenDisplay += ApplyTextSpacing("        V Stairs (Down)             \n");
	screenDisplay += ApplyTextSpacing("        < Stairs (Up)               \n");
	screenDisplay += ApplyTextSpacing("        @ Food   - Increases your   \n");
	screenDisplay += ApplyTextSpacing("                   score and length.\n");
	screenDisplay += ApplyTextSpacing("        % Potion - Restores 3 HP.   \n");
	screenDisplay += "\n" +
		ApplyTextSpacing("Brave the caves, eat the golden\n") +
		ApplyTextSpacing("apple at the deepest depths, and return \n") +
		ApplyTextSpacing("to the surface with your prize!\n");

	screenDisplay += "\n" + ApplyTextSpacing("Tips:") + "\n";
	screenDisplay += ApplyTextSpacing(" - The longer you are, the slower you'll move.    \n");
	screenDisplay += ApplyTextSpacing(" - The caves get darker as you descend, so        \n") +
		ApplyTextSpacing("   visibility will decrease as you go deeper.     \n");

	screenDisplay += ApplyTextSpacing(" - Eating the golden apple will make you emit     \n") +
		ApplyTextSpacing("   light and thereby increase visibility. However,\n") +
		ApplyTextSpacing("   it will also make you move very fast. In this  \n") +
		ApplyTextSpacing("   state, eating food will not slow you down.     \n");

	screenDisplay += ApplyTextSpacing(" - The golden apple doubles your score when eaten.\n");

	std::cout << screenDisplay;
}

void DrawInfoDisplay() {
	string infoDisplay = "\n";

	// Draw HP bar
	infoDisplay += gameMargin + " HP: ";
	string hpBar = "[";
	for (int i = 1; i <= MAX_HEALTH; i++) {
		if (i <= health) {
			hpBar += "/";
		}
		else {
			hpBar += "-";
		}
		if (i % HEALTH_BAR_CHUNK_SIZE == 0 && i != MAX_HEALTH) {
			hpBar += "|";
		}
	}
	hpBar += "]";
	infoDisplay += hpBar + "\n";

	infoDisplay += gameMargin + " Floor: " + to_string(currLevel->levelNum);
	for (int i = 0; i < 7 - to_string(currLevel->levelNum).length(); i++) {
		infoDisplay += " ";
	}
	infoDisplay += "Score: " + to_string(score) + "\n\n";
	std::cout << infoDisplay;
}

void DrawColourfulEntity(Entity entity) {
	if (entity == Entity::HEAD && playerState == PlayerState::IMMOBILE) {
		SetConsoleColour(COLOUR_IMMOBILE);
	} else if (entity == Entity::GOLDEN_APPLE) {
		SetConsoleColour(COLOUR_GOLDEN_APPLE);
	}
	std::cout << entityImg[entity];
	ResetConsoleColour();
}

void DrawMapDisplay() {
	string map = "";
	for (int i = 0; i < HEIGHT; i++) {
		map += gameMargin + " ";
		for (int j = 0; j < WIDTH; j++) {
			if (currLevel->particles[i][j] == Particle::SPARKLE || currLevel->particles[i][j] == Particle::NEW_SPARKLE) {
				map += particleImg[currLevel->particles[i][j]];
			}
			else if (currLevel->playerMap[i][j] == Visibility::INVISIBLE) {
				map += terrainImg[Terrain::UNKNOWN];
			}
			else if (currLevel->entities[i][j] != Entity::NO_ENTITY) {
				// Check if this entity needs to be drawn with a different colour
				Entity *currEntity = std::find(std::begin(colourfulEntities), std::end(colourfulEntities), currLevel->entities[i][j]);
				if (currEntity != std::end(colourfulEntities)) {
					std::cout << map;
					DrawColourfulEntity((Entity)currLevel->entities[i][j]);
					map = "";
				}
				else {
					map += entityImg[currLevel->entities[i][j]];
				}
			}
			else {
				map += terrainImg[currLevel->terrain[i][j]];
			}
			if (j == WIDTH - 1) {
				map += "\n";
			}
		}
	}
	std::cout << map;
}

void DrawGame() {
	ResetCursor();
	DrawInfoDisplay();
	DrawMapDisplay();
	if (DEBUG_MSGS) {
		screenDisplay.clear();
		screenDisplay += gameMargin + " [ Collisions: " + to_string(collisions) + "  Speed: " + to_string(speed) + "  Delay: " + to_string(delay) + " ]\n";
		std::cout << screenDisplay;
	}
}

string FormatTime(int seconds) {
	string ret = "";
	int remaining = seconds;
	int hoursInSeconds = remaining % 3600;
	int hours = ((remaining - hoursInSeconds) / 3600);
	if (hours > 0) {
		ret += to_string(hours) + "h";
	}
	remaining = remaining % 3600;
	int minutesInSeconds = remaining % 60;
	int minutes = ((remaining - minutesInSeconds) / 60);
	if (minutes > 0 || hours > 0) {
		ret += to_string(minutes) + "m";
	}
	ret += to_string(remaining % 60) + "s";
	return ret;
}

string FormatStatisticLine(string label, string stat) {
	string labelSpacing = "";
	string statSpacing = "";
	while ((labelSpacing + label).length() < stat.length()) {
		labelSpacing += " ";
	}
	while ((statSpacing + stat).length() < label.length()) {
		statSpacing += " ";
	}
	return " " + labelSpacing + label + ": " + stat + statSpacing;
}

void DrawGameOver() {
	EraseDisplay();
	screenDisplay.clear();
	screenDisplay += "\n";
	if (escaped) {
		screenDisplay += ApplyTextSpacing("+------------------+") + "\n";
		screenDisplay += ApplyTextSpacing("| Congratulations! |") + "\n";
		screenDisplay += ApplyTextSpacing("+------------------+") + "\n";
		screenDisplay += "\n" + ApplyTextSpacing("You escaped with the golden apple!") + "\n";
	}
	else {
		screenDisplay += ApplyTextSpacing("+-----------+") + "\n";
		screenDisplay += ApplyTextSpacing("| GAME OVER |") + "\n";
		screenDisplay += ApplyTextSpacing("+-----------+") + "\n";
		screenDisplay += "\n" + ApplyTextSpacing("You lost on floor " + to_string(currLevel->levelNum) + ".") + "\n";
	}
	string snakeDisplay = "";
	for (int i = 0; i < tailLength; i++) {
		snakeDisplay += "o";
	}
	snakeDisplay += "O";
	screenDisplay += "\n" + ApplyTextSpacing(snakeDisplay) + "\n\n";
	screenDisplay += ApplyTextSpacing(FormatStatisticLine("Length", to_string(tailLength + 1))) + "\n";
	screenDisplay += ApplyTextSpacing(FormatStatisticLine("Score", to_string(score))) + "\n";
	if (escaped) {
		screenDisplay += ApplyTextSpacing(FormatStatisticLine("Remaining HP", to_string(health))) + "\n";
	}
	screenDisplay += ApplyTextSpacing(FormatStatisticLine("Collisions", to_string(collisions))) + "\n";
	string timeInSeconds = FormatTime(GetDifferenceInSeconds(startTime, endTime));
	screenDisplay += ApplyTextSpacing(FormatStatisticLine("Time", timeInSeconds)) + "\n";
	screenDisplay += "\n" + ApplyTextSpacing("Press enter to return to the menu.");
	std::cout << screenDisplay;
}

void Draw()
{
	if (gameMode == GameMode::PLAYING) {
		DrawGame();
	}
	else if (gameMode == GameMode::MENU) {
		DrawMenu();
	}
	else if (gameMode == GameMode::GAME_OVER) {
		DrawGameOver();
	}
}

void MenuInput() {
	// Press enter to continue
	int ch = 0;
	while (true) {
		ch = _getch();
		if (ch == '\r') {
			gameMode = GameMode::PLAYING;
			break;
		}
		else if (ch == 27) {
			// ESC
			gameMode = GameMode::EXIT;
			break;
		}
		else if (ch == 'G' || ch == 'g') {
			ToggleGameSize();
		}
	}
	EraseDisplay();
}

void GameOverInput() {
	// Press enter to continue
	int ch = 0;
	do {
		ch = _getch();
	} while (ch != '\r');
	EraseDisplay();
	gameMode = GameMode::MENU;
}

void GameInput() {
	if (playerState == PlayerState::ACTIVE) {
		if (_kbhit()) {
			Direction dir;
			bool validButtonPressed = false;
			switch (_getch()) {
			case 72:
			case 'W':
			case 'w':
				dir = Direction::UP;
				validButtonPressed = true;
				break;
			case 75:
			case 'A':
			case 'a':
				dir = Direction::LEFT;
				validButtonPressed = true;
				break;
			case 77:
			case 'D':
			case 'd':
				dir = Direction::RIGHT;
				validButtonPressed = true;
				break;
			case 80:
			case 'S':
			case 's':
				dir = Direction::DOWN;
				validButtonPressed = true;
				break;
			}
			if (validButtonPressed) {
				Point dest = ApplyMovement(playerCoord, dir);
				EnqueueMove(dir);
			}
		}
	}
}

void Move() {
	// Update player direction
	if (!moveQueue.empty()) {
		Direction move = moveQueue.front();
		moveQueue.pop();
		// Disallow a player from moving backward into its tail
		Point desiredDest = ApplyMovement(playerCoord, move);
		if (!tail.empty()) {
			if (desiredDest != tail.back()) {
				playerDir = move;
			}
		}
		else {
			playerDir = move;
		}
	}
	if (playerDir == Direction::NO_DIRECTION) {
		return;
	}
	Point dest = ApplyMovement(playerCoord, playerDir);
	Point* tailEnd = NULL;
	if (!tail.empty() && tail.size() == tailLength) {
		tailEnd = &tail.front();
	}
	if (IsTraversableTile(currLevel, dest, tailEnd)) {
		// If player has a tail, leave it behind
		if (tailLength > 0) {
			currLevel->entities[playerCoord.x][playerCoord.y] = Entity::TAIL;
			tail.push_back(Point(playerCoord.x, playerCoord.y));
			// Remove the last piece of the tail unless the player just ate
			// food or if the tail is not fully propgated, like when going
			// through stairs
			if (currLevel->entities[dest.x][dest.y] != Entity::FOOD && tail.size() - 1 == tailLength) {
				currLevel->entities[tail[0].x][tail[0].y] = Entity::NO_ENTITY;
				if (!tail.empty()) {
					tail.erase(tail.begin());
				}
			}
		} else {
			// Empty the space the player left
			currLevel->entities[playerCoord.x][playerCoord.y] = Entity::NO_ENTITY;
		}
		switch (currLevel->entities[dest.x][dest.y]) {
			case (Entity::FOOD):
				currLevel->entities[playerCoord.x][playerCoord.y] = Entity::TAIL;
				// Start the tail
				if (tailLength == 0) {
					tail.push_back(Point(playerCoord.x, playerCoord.y));
				}
				EatFood();
				break;
			case (Entity::TAIL):
				// Allow moving into your tail if it's the last piece
				if (dest != tail.front()) {
					Collide();
					if (health <= 0) {
						EndGame();
					}
					immobileStart = std::chrono::system_clock::now();
				}
				break;
			case (Entity::GOLDEN_APPLE):
				EatGoldenApple();
				playerCoord = ApplyMovement(playerCoord, playerDir);
				playerDir = Direction::NO_DIRECTION;
				playerState = PlayerState::CONSUMING_APPLE;
				break;
			case (Entity::POTION):
				IncreaseHp(POTION_HEAL_VALUE);
				break;
		}
		currLevel->entities[dest.x][dest.y] = Entity::HEAD;
		switch (currLevel->terrain[dest.x][dest.y]) {
			case (Terrain::STAIRS_UP):
				if (currLevel->levelNum == 1) {
					if (hasGoldenApple) {
						score += VICTORY_POINTS;
						int maxScore = ((NUM_OF_LEVELS - 1) * FOOD_MIN + EXTRA_FOOD) * FOOD_POINTS * GOLDEN_APPLE_SCORE_MULTIPLIER + VICTORY_POINTS;
						if (score == maxScore && collisions == 0) {
							// Give bonus point if player has maximum score and didn't hit anything
							score += 1;
						}
						escaped = true;
						EndGame();
					}
					break;
				}
				ProcessLeaveLevel();
				currLevel = levelStorage[currLevel->levelNum - 2];
				playerDir = Direction::NO_DIRECTION;
				UpdateVisionRadius(currLevel->levelNum);
				MovePlayerToStairs(Terrain::STAIRS_DOWN);
				SetImmobile();
				break;
			case (Terrain::STAIRS_DOWN):
				if (currLevel->levelNum == NUM_OF_LEVELS) {
					break;
				}
				if (currLevel->levelNum == generatedLevelsCount) {
					if (currLevel->levelNum == NUM_OF_LEVELS - 1) {
						// Create the final level
						AddLastLevel();
					}
					else {
						AddNewLevel();
					}
				}
				// Move to the next level
				ProcessLeaveLevel();
				currLevel = levelStorage[currLevel->levelNum];
				playerDir = Direction::NO_DIRECTION;
				UpdateVisionRadius(currLevel->levelNum);
				MovePlayerToStairs(Terrain::STAIRS_UP);
				SetImmobile();
				break;
		}
		playerCoord = ApplyMovement(playerCoord, playerDir);
		UpdatePlayerMap(currLevel, playerCoord, currVisionRadius);
	}
	else {
		Collide();
		if (health <= 0) {
			EndGame();
		}
		collisions++;
	}
}

void ProcessTick() {
	Move();
	Draw();
	if (playerState == PlayerState::IMMOBILE) {
		Sleep(IMMOBILIZE_DELAY);
		FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
		playerState = PlayerState::ACTIVE;
	}
	else if (playerState == PlayerState::CONSUMING_APPLE) {
		ParticleBurst(playerCoord);
		playerState = PlayerState::ACTIVE;
	}
	else {
		Sleep(delay);
	}
}

void RunGame() {
	while (gameMode == GameMode::PLAYING) {
		GameInput();
		ProcessTick();
		GameInput();
	}
}

int main()
{
	Setup();
	while (gameMode != GameMode::EXIT) {
		if (gameMode == GameMode::PLAYING) {
			StartGame();
			RunGame();
		}
		else if (gameMode == GameMode::MENU) {
			EraseDisplay();
			Draw();
			MenuInput();
		}
		else if (gameMode == GameMode::GAME_OVER) {
			EraseDisplay();
			Draw();
			GameOverInput();
		}
	}
	return 0;
}

// TODO: whitespace

