#include <chrono>    // for timing: delays, bullet/enemy spawn rate control
#include <cstdlib>   // for exit()
#include <fcntl.h>   // for manipulating file descriptor flags (non-blocking input)
#include <iostream>  // for I/O
#include <random>    // for random enemy spawn
#include <termios.h> // for terminal mode manipulation
#include <thread>    // for sleep_for
#include <unistd.h>  // for read(), STDIN_FILENO
#include <vector>    // for game board and bullet/enemy containers

using namespace std;
using namespace chrono;

// raw terminal mode: disables canonical input + echo for real-time keypresses
void set_terminal_raw_mode(bool enable) {
    static termios oldt, newt;

    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt); // save old terminal settings
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);        // turn off line buffering and echo
        tcsetattr(STDIN_FILENO, TCSANOW, &newt); // apply immediately
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // restore old terminal mode
    }
}

// enables/disables non-blocking input read() on stdin
void set_non_blocking_input(bool enable) {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);

    if (enable) {
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    } else {
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    }
}

// returns a key if pressed, otherwise '\0' (non-blocking)
char get_keypress() {
    char ch = '\0';
    read(STDIN_FILENO, &ch, 1); // non-blocking read
    return ch;
}

// generate a random column index in the valid range for enemy spawn
int generate_random() {
    int low = 1;
    int high = 158; // 0 and 159 are borders

    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<> dist(low, high);
    return dist(gen);
}

// move all enemy ships down 1 cell if possible, and check for loss condition
void update_board(vector<vector<char>> &space) {
    // iterate from bottom-up to avoid overwriting enemies as they move
    for (int i = space.size() - 2; i >= 1; --i) {
        for (int j = 1; j < space[0].size() - 1; ++j) {
            // if enemy and below is empty, move it down
            if (space[i][j] == '#' && space[i + 1][j] == ' ') {
                space[i][j] = ' ';
                space[i + 1][j] = '#';
            }

            // check if any enemy reached bottom row
            if (space[29][j] == '#') {
                set_terminal_raw_mode(false);
                set_non_blocking_input(false);
                cout << "\033[2J\033[1;1H";
                cout << "You lost" << endl;
                exit(0);
            }
        }
    }
}

// clear screen and print the current game board
void print_board(const vector<vector<char>> &space) {
    cout << "\033[2J\033[1;1H"; // ANSI escape to clear screen and move cursor to (1,1)

    int rows = space.size();
    int cols = space[0].size();

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // draw border as '0' if it's empty, otherwise draw cell
            if ((i == 0 || i == rows - 1 || j == 0 || j == cols - 1) && space[i][j] == ' ') {
                cout << '0';
            } else {
                cout << space[i][j];
            }
        }
        cout << '\n';
    }
}

int main() {
    int rows = 30, cols = 160;                                 // fixed screen size
    vector<vector<char>> space(rows, vector<char>(cols, ' ')); // 2D game board
    int shooter_position = 70;                                 // x-position of shooter

    vector<pair<int, int>> bullets;              // each bullet has (y, x) position
    auto last_bullet_time = steady_clock::now(); // last time a bullet was shot

    set_terminal_raw_mode(true);  // raw mode for real-time input
    set_non_blocking_input(true); // non-blocking key reads

    auto last_deploy_time = steady_clock::now(); // last time an enemy was spawned
    auto last_frame_time = steady_clock::now();  // last time enemies moved down

    while (true) {
        auto now = steady_clock::now();

        // read a key if pressed, otherwise returns '\0'
        char move = get_keypress();

        // shoot a bullet every 100ms from shooter's position (27th row)
        if (duration_cast<milliseconds>(now - last_bullet_time).count() >= 100) {
            bullets.push_back({27, shooter_position});
            last_bullet_time = now;
        }

        // clear shooter's old position
        space[28][shooter_position] = ' ';

        // move shooter
        switch (move) {
        case 'a':
            if (shooter_position > 1)
                shooter_position--;
            break;
        case 'd':
            if (shooter_position < cols - 2)
                shooter_position++;
            break;
        case 'q':
            set_terminal_raw_mode(false);
            set_non_blocking_input(false);
            cout << "Quit.\n";
            return 0;
        }

        // update shooter's new position
        space[28][shooter_position] = 'A';

        // every 2 seconds, deploy a new enemy at a random top column
        if (duration_cast<seconds>(now - last_deploy_time).count() >= 2) {
            int deploy_position = generate_random();
            space[1][deploy_position] = '#';
            last_deploy_time = now;
        }

        // every 500ms, move enemies down
        if (duration_cast<milliseconds>(now - last_frame_time).count() >= 500) {
            update_board(space);
            last_frame_time = now;
        }

        // erase old bullet positions (if still rendered)
        for (auto &b : bullets) {
            if (space[b.first][b.second] == '.')
                space[b.first][b.second] = ' ';
        }

        vector<pair<int, int>> new_bullets;
        for (auto &b : bullets) {
            int new_x = b.first - 1;
            int y = b.second;

            // if bullet is still within screen
            if (new_x >= 1) {
                // if bullet hits enemy, destroy both
                if (space[new_x][y] == '#') {
                    space[new_x][y] = ' ';
                    continue;
                } else {
                    new_bullets.emplace_back(new_x, y); // bullet moves up
                }
            }
        }

        // draw updated bullet positions as '.'
        for (auto &b : new_bullets) {
            space[b.first][b.second] = '.';
        }
        bullets = new_bullets;

        print_board(space); // redraw everything

        this_thread::sleep_for(milliseconds(10)); // main game loop delay
    }

    // restore terminal state if somehow the loop exits (redundant here)
    set_terminal_raw_mode(false);
    set_non_blocking_input(false);
}
