#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <cmath>
#include <iostream>
#include <optional>

// Настройки игры
struct GameConfig {
    static constexpr int GridSize = 10;      
    static constexpr int ColorCount = 5;     
    static constexpr int TileSize = 60;      
    static constexpr int BoardOffset = 40;   
};


struct Point {
    int row = -1;
    int col = -1;

    bool isValid() const { return row != -1 && col != -1; }
    void reset() { row = -1; col = -1; }
    bool operator==(const Point& other) const { return row == other.row && col == other.col; }
};


class Tile {
private:
    int colorId = 0;
public:
    Tile() = default;
    Tile(int color) : colorId(color) {}

    int getColor() const { return colorId; }
    void setColor(int color) { colorId = color; }
};


class GameBoard {
private:
    std::vector<std::vector<Tile>> grid;
    std::mt19937 gen;

    bool hasMatch(int row, int col, int color) const {
        if (col >= 2 && grid[row][col - 1].getColor() == color && grid[row][col - 2].getColor() == color) return true;
        if (row >= 2 && grid[row - 1][col].getColor() == color && grid[row - 2][col].getColor() == color) return true;
        return false;
    }


    bool isNeighbor(int r1, int c1, int r2, int c2) const {
        return std::abs(r1 - r2) <= 1 && std::abs(c1 - c2) <= 1;
    }

    void triggerBonus(int sourceRow, int sourceCol, int originalColor) {
        std::uniform_int_distribution<int> chanceDist(1, 100);
        if (chanceDist(gen) > 15) return; 

        std::vector<Point> neighborhood;
        for (int r = sourceRow - 3; r <= sourceRow + 3; ++r) {
            for (int c = sourceCol - 3; c <= sourceCol + 3; ++c) {
                if (r >= 0 && r < GameConfig::GridSize && c >= 0 && c < GameConfig::GridSize) {
                    neighborhood.push_back({ r, c });
                }
            }
        }

        if (neighborhood.empty()) return;

        std::uniform_int_distribution<int> indexDist(0, neighborhood.size() - 1);
        Point target = neighborhood[indexDist(gen)];

        std::uniform_int_distribution<int> bonusTypeDist(1, 2);
        int bonusType = bonusTypeDist(gen);

        if (bonusType == 1) {
            std::cout << "activate bonus 1: redraw in area [" << target.row << ", " << target.col << "]\n";

            grid[target.row][target.col].setColor(originalColor);

            std::vector<Point> nonNeighbors;
            for (const auto& p : neighborhood) {
                if (!isNeighbor(target.row, target.col, p.row, p.col)) {
                    nonNeighbors.push_back(p);
                }
            }

            if (nonNeighbors.size() >= 2) {
                std::shuffle(nonNeighbors.begin(), nonNeighbors.end(), gen);
                grid[nonNeighbors[0].row][nonNeighbors[0].col].setColor(originalColor);
                grid[nonNeighbors[1].row][nonNeighbors[1].col].setColor(originalColor);
            }
        }
        else if (bonusType == 2) {
            std::cout << "activate bonus 2: we are have a BOMB(( in [" << target.row << ", " << target.col << "]\n";

            grid[target.row][target.col].setColor(-1);

            std::vector<Point> allTiles;
            for (int r = 0; r < GameConfig::GridSize; ++r) {
                for (int c = 0; c < GameConfig::GridSize; ++c) {
                    if (!(r == target.row && c == target.col)) {
                        allTiles.push_back({ r, c });
                    }
                }
            }

            std::shuffle(allTiles.begin(), allTiles.end(), gen);
            for (int i = 0; i < std::min(4, (int)allTiles.size()); ++i) {
                grid[allTiles[i].row][allTiles[i].col].setColor(-1);
            }
        }
    }

public:
    GameBoard() : grid(GameConfig::GridSize, std::vector<Tile>(GameConfig::GridSize)), gen(std::random_device{}()) {
        init();
    }

    void init() {
        std::uniform_int_distribution<int> distrib(0, GameConfig::ColorCount - 1);
        for (int i = 0; i < GameConfig::GridSize; ++i) {
            for (int j = 0; j < GameConfig::GridSize; ++j) {
                int color;
                int attempts = 0;
                do {
                    color = distrib(gen);
                    attempts++;
                } while (hasMatch(i, j, color) && attempts < 100);
                grid[i][j].setColor(color);
            }
        }
    }

    int getColor(int row, int col) const { return grid[row][col].getColor(); }

    bool isAdjacent(Point p1, Point p2) const {
        return (std::abs(p1.row - p2.row) == 1 && p1.col == p2.col) ||
            (std::abs(p1.col - p2.col) == 1 && p1.row == p2.row);
    }

    void swapTiles(Point p1, Point p2) {
        int tempColor = grid[p1.row][p1.col].getColor();
        grid[p1.row][p1.col].setColor(grid[p2.row][p2.col].getColor());
        grid[p2.row][p2.col].setColor(tempColor);
    }

    bool findAndRemoveMatches() {
        std::vector<std::vector<bool>> markedToRemove(GameConfig::GridSize, std::vector<bool>(GameConfig::GridSize, false));
        bool anyMatches = false;


        for (int r = 0; r < GameConfig::GridSize; ++r) {
            for (int c = 0; c < GameConfig::GridSize - 2; ++c) {
                int color = grid[r][c].getColor();
                if (color != -1 && grid[r][c + 1].getColor() == color && grid[r][c + 2].getColor() == color) {
                    markedToRemove[r][c] = true;
                    markedToRemove[r][c + 1] = true;
                    markedToRemove[r][c + 2] = true;
                    anyMatches = true;
                }
            }
        }

        for (int r = 0; r < GameConfig::GridSize - 2; ++r) {
            for (int c = 0; c < GameConfig::GridSize; ++c) {
                int color = grid[r][c].getColor();
                if (color != -1 && grid[r + 1][c].getColor() == color && grid[r + 2][c].getColor() == color) {
                    markedToRemove[r][c] = true;
                    markedToRemove[r + 1][c] = true;
                    markedToRemove[r + 2][c] = true;
                    anyMatches = true;
                }
            }
        }


        if (anyMatches) {
            for (int r = 0; r < GameConfig::GridSize; ++r) {
                for (int c = 0; c < GameConfig::GridSize; ++c) {
                    if (markedToRemove[r][c]) {
                        int origColor = grid[r][c].getColor();
                        grid[r][c].setColor(-1);

                        triggerBonus(r, c, origColor);
                    }
                }
            }
        }

        return anyMatches;
    }

    void updateGravity() {
        std::uniform_int_distribution<int> distrib(0, GameConfig::ColorCount - 1);


        for (int c = 0; c < GameConfig::GridSize; ++c) {

            for (int r = GameConfig::GridSize - 1; r >= 0; --r) {
                // Если нашли пустую ячейку (-1)
                if (grid[r][c].getColor() == -1) {

                    int nextRow = r - 1;
                    while (nextRow >= 0 && grid[nextRow][c].getColor() == -1) {
                        nextRow--;
                    }

                    if (nextRow >= 0) {
                        grid[r][c].setColor(grid[nextRow][c].getColor());
                        grid[nextRow][c].setColor(-1); 
                    }
                    else {
                        grid[r][c].setColor(distrib(gen));
                    }
                }
            }
        }
    }

};


// Класс отображения графики
class GameView {
private:
    sf::Texture bgTexture;
    std::vector<sf::Texture> tileTextures;
    sf::RectangleShape selectionMarker;

public:
    GameView() {
        if (!bgTexture.loadFromFile("assets/background.png")) {
            std::cout << "Ошибка загрузки assets/background.png!" << std::endl;
        }

        tileTextures.resize(GameConfig::ColorCount);
        for (int i = 0; i < GameConfig::ColorCount; ++i) {
            std::string filename = "assets/tile_" + std::to_string(i) + ".png";

            if (!tileTextures[i].loadFromFile(filename)) {
                std::cout << "Ошибка загрузки файла: " << filename << std::endl;
            }

            tileTextures[i].setSmooth(true);
            tileTextures[i].generateMipmap();
        }

        selectionMarker.setSize({ static_cast<float>(GameConfig::TileSize - 2), static_cast<float>(GameConfig::TileSize - 2) });
        selectionMarker.setFillColor(sf::Color::Transparent);
        selectionMarker.setOutlineColor(sf::Color::White);
        selectionMarker.setOutlineThickness(3);
    }

    Point getCellAtMouse(sf::Vector2i mousePos) {
        int col = (mousePos.x - GameConfig::BoardOffset) / GameConfig::TileSize;
        int row = (mousePos.y - GameConfig::BoardOffset) / GameConfig::TileSize;

        if (row >= 0 && row < GameConfig::GridSize && col >= 0 && col < GameConfig::GridSize) {
            return { row, col };
        }
        return { -1, -1 };
    }

    void draw(sf::RenderWindow& window, const GameBoard& board, Point selectedPoint) {
        window.clear();

        sf::Sprite bgSprite(bgTexture);
        sf::Vector2u windowSize = window.getSize();
        sf::Vector2u bgSize = bgTexture.getSize();

        float scaleX = static_cast<float>(windowSize.x) / bgSize.x;
        float scaleY = static_cast<float>(windowSize.y) / bgSize.y;
        bgSprite.setScale({ scaleX, scaleY });
        window.draw(bgSprite);

        for (int i = 0; i < GameConfig::GridSize; ++i) {
            for (int j = 0; j < GameConfig::GridSize; ++j) {
                float posX = GameConfig::BoardOffset + j * GameConfig::TileSize + GameConfig::TileSize / 2.f;
                float posY = GameConfig::BoardOffset + i * GameConfig::TileSize + GameConfig::TileSize / 2.f;

                int colorId = board.getColor(i, j);

                if (colorId == -1) {
                    continue;
                }

                sf::Sprite tileSprite(tileTextures[colorId]);
                sf::Vector2u textureSize = tileTextures[colorId].getSize();
                tileSprite.setOrigin({ textureSize.x / 2.f, textureSize.y / 2.f });
                float scale = static_cast<float>(GameConfig::TileSize) / textureSize.x;
                tileSprite.setScale({ scale, scale });
                tileSprite.setPosition({ posX, posY });
                window.draw(tileSprite);
            }
        }

        if (selectedPoint.isValid()) {
            float posX = static_cast<float>(GameConfig::BoardOffset + selectedPoint.col * GameConfig::TileSize);
            float posY = static_cast<float>(GameConfig::BoardOffset + selectedPoint.row * GameConfig::TileSize);
            selectionMarker.setPosition({ posX, posY });
            window.draw(selectionMarker);
        }

        window.display();
    }
};

int main() {
    unsigned int windowSize = GameConfig::GridSize * GameConfig::TileSize + GameConfig::BoardOffset * 2;
    sf::RenderWindow window(sf::VideoMode({ windowSize, windowSize }), "Match-3 Game Structure SFML 3.0");

    GameBoard board;
    GameView view;
    Point selectedCell;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* mouseButtonPress = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseButtonPress->button == sf::Mouse::Button::Left) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    Point clickedCell = view.getCellAtMouse(mousePos);

                    if (clickedCell.isValid()) {
                        if (!selectedCell.isValid()) {
                            selectedCell = clickedCell;
                        }
                        else {
                            if (selectedCell == clickedCell) {
                                selectedCell.reset();
                            }
                            else if (board.isAdjacent(selectedCell, clickedCell)) {
                                // 1. Меняем местами
                                board.swapTiles(selectedCell, clickedCell);

                                if (board.findAndRemoveMatches()) {

                                    do {
                                        view.draw(window, board, selectedCell);
                                        sf::sleep(sf::milliseconds(250)); 

                                        board.updateGravity();

                                        view.draw(window, board, selectedCell);
                                        sf::sleep(sf::milliseconds(150));

                                    } while (board.findAndRemoveMatches());
                                }
                                else {
                                    board.swapTiles(selectedCell, clickedCell);
                                }
                                selectedCell.reset();
                            }

                            else {
                                selectedCell = clickedCell;
                            }
                        }
                    }
                    else {
                        selectedCell.reset();
                    }
                }
            }
        }

        view.draw(window, board, selectedCell);
    }

    return 0;
}
