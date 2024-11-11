#include <iostream>
#include <vector>
#include <utility>
#include <cstdlib>
#include <ctime>
#include <cmath>  // For distance calculations

using namespace std;

enum CellStatus { Dead, Prey, Predator };

struct Cell {
    CellStatus status;
    int energy = 0; // Used only for predators
    int reproductionCooldown = 0; // Used only for prey
};

using Grid = vector<vector<Cell>>;

Grid initializeGrid(int rows, int cols, int numPrey, int numPredators) {
    Grid grid(rows, vector<Cell>(cols, {Dead}));
    
    srand(static_cast<unsigned>(time(0)));

    for (int i = 0; i < numPrey; ++i) {
        int randomRow = rand() % rows;
        int randomCol = rand() % cols;
        grid[randomRow][randomCol] = {Prey};
    }

    for (int i = 0; i < numPredators; ++i) {
        int randomRow = rand() % rows;
        int randomCol = rand() % cols;
        grid[randomRow][randomCol] = {Predator, 5}; // Initial energy of 5
    }

    return grid;
}

vector<pair<int, int>> getNeighbors(int row, int col, const Grid& grid) {
    vector<pair<int, int>> neighbors;
    int rows = grid.size();
    int cols = grid[0].size();

    int offsets[6][2];
    if (row % 2 == 0) {
        int temp[6][2] = {{-1, -1}, {-1, 0}, {0, -1}, {0, 1}, {1, -1}, {1, 0}};
        copy(&temp[0][0], &temp[0][0] + 6 * 2, &offsets[0][0]);
    } else {
        int temp[6][2] = {{-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, 0}, {1, 1}};
        copy(&temp[0][0], &temp[0][0] + 6 * 2, &offsets[0][0]);
    }

    for (const auto& offset : offsets) {
        int newRow = row + offset[0];
        int newCol = col + offset[1];
        if (newRow >= 0 && newRow < rows && newCol >= 0 && newCol < cols) {
            neighbors.emplace_back(newRow, newCol);
        }
    }
    return neighbors;
}

double calculateDistance(int row1, int col1, int row2, int col2) {
    return sqrt(pow(row1 - row2, 2) + pow(col1 - col2, 2));
}

void updatePrey(int row, int col, Grid& grid, Grid& nextGrid) {
    Cell& cell = grid[row][col];
    if (cell.status != Prey) return;

    // Get neighboring cells
    vector<pair<int, int>> neighbors = getNeighbors(row, col, grid);

    // Separate neighbors into prey and empty (Dead) cells
    vector<pair<int, int>> preyNeighbors, emptyNeighbors;
    for (const auto& neighbor : neighbors) {
        int nRow = neighbor.first;
        int nCol = neighbor.second;
        if (grid[nRow][nCol].status == Prey) {
            preyNeighbors.push_back({nRow, nCol});
        } else if (grid[nRow][nCol].status == Dead) {
            emptyNeighbors.push_back({nRow, nCol});
        }
    }

    // Attempt reproduction if there's another prey nearby and cooldown is 0
    if (!preyNeighbors.empty() && cell.reproductionCooldown == 0) {
        if (!emptyNeighbors.empty()) {
            // Choose a random empty cell for the new prey
            pair<int, int> newLocation = emptyNeighbors[rand() % emptyNeighbors.size()];
            int newRow = newLocation.first;
            int newCol = newLocation.second;
            nextGrid[newRow][newCol] = {Prey}; // Spawn new prey
            cell.reproductionCooldown = 2; // Set reproduction cooldown to 2 moves
        }
    }

    // Move to a neighboring cell with a bias toward other prey
    pair<int, int> bestLocation = {row, col};
    if (!emptyNeighbors.empty()) {
        // Choose a random empty cell for movement if available
        bestLocation = emptyNeighbors[rand() % emptyNeighbors.size()];
    }

    // Move prey to the new location and update cooldown
    int newRow = bestLocation.first;
    int newCol = bestLocation.second;
    nextGrid[newRow][newCol] = {Prey, 0, max(0, cell.reproductionCooldown - 1)}; // Move prey to new location

    // Leave the original cell as Dead if the prey moved
    if (bestLocation != make_pair(row, col)) {
        nextGrid[row][col].status = Dead;
    }
}

void updatePredator(int row, int col, Grid& grid, Grid& nextGrid) {
    Cell& cell = grid[row][col];
    if (cell.status != Predator) return;

    // Check if predator has energy to move; if not, it dies
    if (cell.energy <= 0) {
        nextGrid[row][col].status = Dead;
        return;
    }

    // Get neighboring cells
    vector<pair<int, int>> neighbors = getNeighbors(row, col, grid);

    // Separate neighbors into prey and empty cells
    vector<pair<int, int>> preyNeighbors, emptyNeighbors, predatorNeighbors;
    for (const auto& neighbor : neighbors) {
        int nRow = neighbor.first;
        int nCol = neighbor.second;
        if (grid[nRow][nCol].status == Prey) {
            preyNeighbors.push_back({nRow, nCol});
        } else if (grid[nRow][nCol].status == Dead) {
            emptyNeighbors.push_back({nRow, nCol});
        } else if (grid[nRow][nCol].status == Predator) {
            predatorNeighbors.push_back({nRow, nCol});
        }
    }

    // Move towards prey if available; otherwise, move to an empty cell
    pair<int, int> newLocation = {row, col};
    bool consumedPrey = false;
    if (!preyNeighbors.empty()) {
        newLocation = preyNeighbors[rand() % preyNeighbors.size()];
        consumedPrey = true;
    } else if (!emptyNeighbors.empty()) {
        newLocation = emptyNeighbors[rand() % emptyNeighbors.size()];
    }

    // Adjust energy based on movement and prey consumption
    int newEnergy = cell.energy - 1 + (consumedPrey ? 5 : 0);
    nextGrid[newLocation.first][newLocation.second] = {Predator, newEnergy};

    // Mark the original cell as dead if the predator moved
    if (newLocation != make_pair(row, col)) {
        nextGrid[row][col].status = Dead;
    }

    // Attempt to reproduce if there's another predator adjacent with sufficient energy
    if (newEnergy >= 5) {
        for (const auto& predatorNeighbor : predatorNeighbors) {
            int pRow = predatorNeighbor.first;
            int pCol = predatorNeighbor.second;

            if (grid[pRow][pCol].energy >= 5) {  // Check the adjacent predator's energy
                for (const auto& emptyNeighbor : emptyNeighbors) {
                    int eRow = emptyNeighbor.first;
                    int eCol = emptyNeighbor.second;
                    nextGrid[eRow][eCol] = {Predator, 5}; // Spawn new predator with initial energy

                    // Deduct energy from both parents
                    nextGrid[newLocation.first][newLocation.second].energy -= 3;
                    nextGrid[pRow][pCol].energy -= 3;
                    break;
                }
                break;
            }
        }
    }
}

void updateGrid(Grid& grid) {
    Grid nextGrid = grid; // Create a copy to store the next state

    // Phase 1: Update all Prey cells
    for (int row = 0; row < grid.size(); ++row) {
        for (int col = 0; col < grid[0].size(); ++col) {
            if (grid[row][col].status == Prey) {
                updatePrey(row, col, grid, nextGrid); // Update Prey in nextGrid
            }
        }
    }

    // Phase 2: Update all Predator cells
    for (int row = 0; row < grid.size(); ++row) {
        for (int col = 0; col < grid[0].size(); ++col) {
            if (grid[row][col].status == Predator) {
                updatePredator(row, col, grid, nextGrid); // Update Predator in nextGrid
            }
        }
    }

    // Apply all updates from nextGrid to grid
    grid = nextGrid;
}

void displayGrid(const Grid& grid) {
    for (int row = 0; row < grid.size(); ++row) {
        if (row % 2 != 0) cout << " ";
        for (int col = 0; col < grid[row].size(); ++col) {
            char symbol = (grid[row][col].status == Dead) ? '-' : (grid[row][col].status == Prey) ? 'P' : 'X';
            cout << symbol << " ";
        }
        cout << endl;
    }
    cout << endl;
}

int main() {
    int rows = 10, cols = 10, numPrey = 10, numPredators = 6;
    Grid grid = initializeGrid(rows, cols, numPrey, numPredators);

    int steps = 20;
    for (int i = 0; i < steps; ++i) {
        cout << "Step " << i + 1 << ":\n";
        displayGrid(grid);
        updateGrid(grid);
    }

    return 0;
}