#include "flipper_td.h"

/**
 * @brief Helper function to convert 2D grid coordinates to a 1D array index.
 * @param x The x-coordinate on the grid.
 * @param y The y-coordinate on the grid.
 * @return The calculated 1D array index.
 */
static inline int idx(int x, int y) {
    return x * GRID_HEIGHT + y;
}

/**
 * @brief Calculates the parameters for a given wave number.
 * @param wave_number The current wave number.
 * @return A Wave struct populated with properties for the enemies of that wave.
 */
Wave get_wave_params(int wave_number) {
    Wave w;
    w.wave_number = wave_number;
    w.enemy_speed = 0.20f + 0.05f * wave_number;
    w.enemy_hp = 3 + wave_number;
    w.enemy_count = wave_number + 2;
    if(w.enemy_count > MAX_ENEMIES) w.enemy_count = MAX_ENEMIES;
    if(w.wave_number < 7)
        w.spawn_interval_ticks = 10 - w.wave_number;
    else
        w.spawn_interval_ticks = 3;
    return w;
}

/**
 * @brief Cycles through the available tower types.
 * @param current The current TowerType.
 * @return The next TowerType in the sequence.
 */
TowerType next_tower_type(TowerType current) {
    switch(current) {
    case TOWER_NONE:
        return TOWER_NORMAL;
    case TOWER_NORMAL:
        return TOWER_RANGE;
    case TOWER_RANGE:
        return TOWER_SPLASH;
    case TOWER_SPLASH:
        return TOWER_FREEZE;
    case TOWER_FREEZE:
        return TOWER_NONE;
    default:
        return TOWER_NONE;
    }
}

/**
 * @brief Finds the shortest path from a start to an end coordinate using Breadth-First Search (BFS).
 * @param game Pointer to the current game state.
 * @param start The starting coordinate.
 * @param end The ending coordinate.
 * @param path An array to be populated with the coordinates of the found path.
 * @param path_length Pointer to an integer to store the length of the path.
 * @return True if a path is found, false otherwise.
 */
bool find_path(GameState* game, Coord start, Coord end, Coord path[], int* path_length) {
    int total_cells = GRID_WIDTH * GRID_HEIGHT;
    bool* visited = malloc(total_cells * sizeof(bool));
    Coord* parent = malloc(total_cells * sizeof(Coord));
    Coord* queue = malloc(total_cells * sizeof(Coord));
    if(!visited || !parent || !queue) {
        free(visited);
        free(parent);
        free(queue);
        return false;
    }
    memset(visited, 0, total_cells * sizeof(bool));
    int front = 0, rear = 0;
    queue[rear++] = start;
    visited[idx(start.x, start.y)] = true;
    parent[idx(start.x, start.y)] = (Coord){-1, -1};

    bool found = false;
    while(front < rear) {
        Coord current = queue[front++];
        if(current.x == end.x && current.y == end.y) {
            found = true;
            break;
        }
        int dx[4] = {1, -1, 0, 0};
        int dy[4] = {0, 0, 1, -1};
        for(int i = 0; i < 4; i++) {
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];
            if(nx < 0 || nx >= GRID_WIDTH || ny < 0 || ny >= GRID_HEIGHT) continue;
            if(!visited[idx(nx, ny)] && game->grid[nx][ny] == TOWER_NONE) {
                visited[idx(nx, ny)] = true;
                parent[idx(nx, ny)] = current;
                queue[rear++] = (Coord){nx, ny};
            }
        }
    }
    if(!found) {
        free(visited);
        free(parent);
        free(queue);
        return false;
    }
    int count = 0;
    Coord step = end;
    while(step.x != -1 && step.y != -1) {
        path[count++] = step;
        step = parent[idx(step.x, step.y)];
    }
    for(int i = 0; i < count / 2; i++) {
        Coord temp = path[i];
        path[i] = path[count - i - 1];
        path[count - i - 1] = temp;
    }
    *path_length = count;
    free(visited);
    free(parent);
    free(queue);
    return true;
}

/**
 * @brief Initializes a new wave of enemies.
 * @param game Pointer to the current game state.
 */
void spawn_wave(GameState* game) {
    Wave wave_params = get_wave_params(game->wave);
    for(int i = 0; i < MAX_ENEMIES; i++) {
        game->enemies[i].active = false;
    }
    game->wave_spawn_index = 0;
    game->pre_wave_timer = PRE_WAVE_TICKS;
    game->wave_spawn_timer = wave_params.spawn_interval_ticks;
}

/**
 * @brief Checks if all enemies in the game are currently inactive.
 * @param game Pointer to the current game state.
 * @return True if all enemies are inactive, false otherwise.
 */
bool all_enemies_inactive(GameState* game) {
    for(int i = 0; i < MAX_ENEMIES; i++) {
        if(game->enemies[i].active) return false;
    }
    return true;
}

/**
 * @brief Updates the position of all active enemies based on the current path.
 * @param game Pointer to the current game state.
 */
void update_enemies(GameState* game) {
    Coord global_path[GRID_WIDTH * GRID_HEIGHT];
    int global_path_length = 0;
    Coord start = {0, 0};
    Coord end = {GRID_WIDTH - 1, GRID_HEIGHT - 1};
    bool valid_path = find_path(game, start, end, global_path, &global_path_length);

    Wave wave_params = get_wave_params(game->wave);
    for(int i = 0; i < MAX_ENEMIES; i++) {
        if(game->enemies[i].active) {
            if(game->enemies[i].freeze_timer > 0) {
                game->enemies[i].freeze_timer--;
            } else if(valid_path && game->enemies[i].path_index < global_path_length - 1) {
                game->enemies[i].progress += wave_params.enemy_speed;
                while(game->enemies[i].progress >= 1.0f &&
                      game->enemies[i].path_index < global_path_length - 1) {
                    game->enemies[i].progress -= 1.0f;
                    game->enemies[i].path_index++;
                    game->enemies[i].pos = global_path[game->enemies[i].path_index];
                    if(game->enemies[i].path_index == global_path_length - 1) {
                        game->lives--;
                        game->enemies[i].active = false;
                        break;
                    }
                }
            }
        }
    }
}

/**
 * @brief Spawns a new projectile from a tower towards a target.
 * @param game Pointer to the current game state.
 * @param tx The tower's x grid coordinate.
 * @param ty The tower's y grid coordinate.
 * @param type The type of the tower firing.
 * @param target The coordinate of the target enemy.
 */
void spawn_projectile(GameState* game, int tx, int ty, TowerType type, Coord target) {
    int grid_top = STATUS_BAR_HEIGHT;
    float tower_cx = tx * CELL_SIZE + CELL_SIZE / 2.0f;
    float tower_cy = grid_top + ty * CELL_SIZE + CELL_SIZE / 2.0f;
    float enemy_cx = target.x * CELL_SIZE + CELL_SIZE / 2.0f;
    float enemy_cy = grid_top + target.y * CELL_SIZE + CELL_SIZE / 2.0f;
    float dx = enemy_cx - tower_cx;
    float dy = enemy_cy - tower_cy;
    float dist = sqrtf(dx * dx + dy * dy);
    if(dist == 0) dist = 1;
    float vx = PROJECTILE_SPEED * dx / dist;
    float vy = PROJECTILE_SPEED * dy / dist;
    for(int p = 0; p < MAX_PROJECTILES; p++) {
        if(!game->projectiles[p].active) {
            game->projectiles[p].active = true;
            game->projectiles[p].x = tower_cx;
            game->projectiles[p].y = tower_cy;
            game->projectiles[p].vx = vx;
            game->projectiles[p].vy = vy;
            game->projectiles[p].damage = 1;
            game->projectiles[p].tower_type = type;
            break;
        }
    }
}

/**
 * @brief Manages tower firing logic for each game tick.
 * @param game Pointer to the current game state.
 */
void update_tower_firing(GameState* game) {
    for(int tx = 0; tx < GRID_WIDTH; tx++) {
        for(int ty = 0; ty < GRID_HEIGHT; ty++) {
            TowerType tower = game->grid[tx][ty];
            if(tower != TOWER_NONE) {
                int tower_range;
                switch(tower) {
                case TOWER_NORMAL:
                    tower_range = 1;
                    break;
                case TOWER_RANGE:
                    tower_range = 2;
                    break;
                case TOWER_SPLASH:
                case TOWER_FREEZE:
                default:
                    tower_range = 1;
                    break;
                }
                for(int i = 0; i < MAX_ENEMIES; i++) {
                    if(game->enemies[i].active) {
                        int dx = abs(game->enemies[i].pos.x - tx);
                        int dy = abs(game->enemies[i].pos.y - ty);
                        if(dx <= tower_range && dy <= tower_range) {
                            spawn_projectile(game, tx, ty, tower, game->enemies[i].pos);
                            break;
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief Updates projectile positions and handles collisions with enemies.
 * @param game Pointer to the current game state.
 */
void update_projectiles(GameState* game) {
    int grid_top = STATUS_BAR_HEIGHT;
    for(int p = 0; p < MAX_PROJECTILES; p++) {
        if(game->projectiles[p].active) {
            game->projectiles[p].x += game->projectiles[p].vx;
            game->projectiles[p].y += game->projectiles[p].vy;
            if(game->projectiles[p].x < 0 || game->projectiles[p].x >= SCREEN_WIDTH ||
               game->projectiles[p].y < grid_top || game->projectiles[p].y >= SCREEN_HEIGHT) {
                game->projectiles[p].active = false;
                continue;
            }
            for(int i = 0; i < MAX_ENEMIES; i++) {
                if(game->enemies[i].active) {
                    int enemy_left = game->enemies[i].pos.x * CELL_SIZE;
                    int enemy_top = grid_top + game->enemies[i].pos.y * CELL_SIZE;
                    int enemy_right = enemy_left + CELL_SIZE;
                    int enemy_bottom = enemy_top + CELL_SIZE;
                    if(game->projectiles[p].x >= enemy_left &&
                       game->projectiles[p].x < enemy_right &&
                       game->projectiles[p].y >= enemy_top &&
                       game->projectiles[p].y < enemy_bottom) {
                        game->enemies[i].hp -= game->projectiles[p].damage;
                        if(game->projectiles[p].tower_type == TOWER_FREEZE)
                            game->enemies[i].freeze_timer = 3;
                        if(game->projectiles[p].tower_type == TOWER_SPLASH) {
                            for(int j = 0; j < MAX_ENEMIES; j++) {
                                if(game->enemies[j].active) {
                                    int ddx = abs(game->enemies[j].pos.x - game->enemies[i].pos.x);
                                    int ddy = abs(game->enemies[j].pos.y - game->enemies[i].pos.y);
                                    if(ddx <= 1 && ddy <= 1)
                                        game->enemies[j].hp -= game->projectiles[p].damage;
                                }
                            }
                        }
                        game->projectiles[p].active = false;
                        break;
                    }
                }
            }
        }
    }
    for(int i = 0; i < MAX_ENEMIES; i++) {
        if(game->enemies[i].active && game->enemies[i].hp <= 0) {
            game->enemies[i].active = false;
            game->gold += 5;
        }
    }
}

/**
 * @brief Renders the entire game state to the canvas.
 * @param canvas The canvas to draw on.
 * @param game Pointer to the current game state.
 */
void draw_game(Canvas* canvas, GameState* game) {
    canvas_reset(canvas);
    char status[32];
    snprintf(
        status, sizeof(status), "Lives:%d Gold:%d Wave:%d", game->lives, game->gold, game->wave);
    canvas_draw_str(canvas, 0, 7, status);

    int grid_top = STATUS_BAR_HEIGHT;
    for(int x = 0; x <= SCREEN_WIDTH; x += CELL_SIZE)
        canvas_draw_line(canvas, x, grid_top, x, SCREEN_HEIGHT);
    for(int y = grid_top; y <= SCREEN_HEIGHT; y += CELL_SIZE)
        canvas_draw_line(canvas, 0, y, SCREEN_WIDTH, y);

    for(int cx = 0; cx < GRID_WIDTH; cx++) {
        for(int cy = 0; cy < GRID_HEIGHT; cy++) {
            if(game->grid[cx][cy] != TOWER_NONE) {
                int pos_x = cx * CELL_SIZE;
                int pos_y = grid_top + cy * CELL_SIZE;
                const char* label = "?";
                switch(game->grid[cx][cy]) {
                case TOWER_NORMAL:
                    label = "N";
                    break;
                case TOWER_RANGE:
                    label = "R";
                    break;
                case TOWER_SPLASH:
                    label = "S";
                    break;
                case TOWER_FREEZE:
                    label = "F";
                    break;
                default:
                    break;
                }
                canvas_draw_str(canvas, pos_x + 2, pos_y + 7, label);
            }
        }
    }

    for(int i = 0; i < MAX_ENEMIES; i++) {
        if(game->enemies[i].active) {
            int pos_x = game->enemies[i].pos.x * CELL_SIZE;
            int pos_y = grid_top + game->enemies[i].pos.y * CELL_SIZE;
            int center_x = pos_x + CELL_SIZE / 2;
            int center_y = pos_y + CELL_SIZE / 2;
            canvas_draw_circle(canvas, center_x, center_y, 3);
        }
    }

    for(int p = 0; p < MAX_PROJECTILES; p++) {
        if(game->projectiles[p].active) {
            canvas_draw_dot(canvas, (int)game->projectiles[p].x, (int)game->projectiles[p].y);
        }
    }

    int cur_x = game->cursor.x * CELL_SIZE;
    int cur_y = grid_top + game->cursor.y * CELL_SIZE;
    canvas_draw_box(canvas, cur_x, cur_y, CELL_SIZE, CELL_SIZE);
}

/**
 * @brief Initializes the game state to default values at the start of the game.
 * @param game Pointer to the game state to initialize.
 */
void init_game_state(GameState* game) {
    game->lives = 10;
    game->gold = 100;
    game->wave = 1;
    for(int x = 0; x < GRID_WIDTH; x++) {
        for(int y = 0; y < GRID_HEIGHT; y++) {
            game->grid[x][y] = TOWER_NONE;
        }
    }
    game->grid[2][2] = TOWER_NORMAL;
    game->grid[4][2] = TOWER_RANGE;
    game->grid[6][2] = TOWER_SPLASH;
    game->grid[8][2] = TOWER_FREEZE;

    game->cursor = (Coord){0, 0};
    for(int i = 0; i < MAX_ENEMIES; i++) {
        game->enemies[i].active = false;
    }
    for(int i = 0; i < MAX_PROJECTILES; i++) {
        game->projectiles[i].active = false;
    }
    spawn_wave(game);
}

// A struct to hold the game state and mutex together for callbacks
typedef struct {
    FuriMutex* mutex;
    GameState* game;
} GameContext;

/**
 * @brief The render callback function passed to the GUI.
 * @param canvas The canvas to draw on.
 * @param ctx A void pointer to the GameContext.
 */
static void render_callback(Canvas* const canvas, void* ctx) {
    GameContext* context = (GameContext*)ctx;
    furi_mutex_acquire(context->mutex, FuriWaitForever);
    draw_game(canvas, context->game);
    furi_mutex_release(context->mutex);
}

/**
 * @brief The input callback function passed to the GUI.
 * @param input_event The event that triggered the callback.
 * @param ctx A void pointer to the event queue.
 */
static void input_callback(InputEvent* input_event, void* ctx) {
    FuriMessageQueue* event_queue = (FuriMessageQueue*)ctx;
    furi_assert(event_queue);
    PluginEvent event = {.type = EventTypeKey, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

/**
 * @brief The main entry point for the Tower Defense application.
 * @param p Unused parameter.
 * @return 0 on success.
 */
int32_t flipper_td_app(void* p) {
    UNUSED(p);
    FURI_LOG_I("flipper_td", "Starting Tower Defense App");

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(PluginEvent));
    GameState* game = malloc(sizeof(GameState));
    FuriMutex* game_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    if(!event_queue || !game || !game_mutex) {
        FURI_LOG_E("flipper_td", "Failed to allocate game resources");
        free(game);
        free(event_queue);
        free(game_mutex);
        return 1;
    }

    init_game_state(game);
    GameContext game_context = {.mutex = game_mutex, .game = game};
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, render_callback, &game_context);
    view_port_input_callback_set(view_port, input_callback, event_queue);

    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    PluginEvent event;
    while(true) {
        FuriStatus event_status = furi_message_queue_get(event_queue, &event, 100);
        furi_mutex_acquire(game_mutex, FuriWaitForever);
        if(event_status == FuriStatusOk) {
            if(event.type == EventTypeKey && event.input.type == InputTypePress) {
                switch(event.input.key) {
                case InputKeyUp:
                    if(game->cursor.y > 0) game->cursor.y--;
                    break;
                case InputKeyDown:
                    if(game->cursor.y < GRID_HEIGHT - 1) game->cursor.y++;
                    break;
                case InputKeyLeft:
                    if(game->cursor.x > 0) game->cursor.x--;
                    break;
                case InputKeyRight:
                    if(game->cursor.x < GRID_WIDTH - 1) game->cursor.x++;
                    break;
                case InputKeyOk: {
                    Coord test_path[GRID_WIDTH * GRID_HEIGHT];
                    int test_path_length = 0;
                    Coord start = {0, 0};
                    Coord end = {GRID_WIDTH - 1, GRID_HEIGHT - 1};

                    if(game->grid[game->cursor.x][game->cursor.y] == TOWER_NONE) {
                        if(game->gold >= 10) {
                            game->grid[game->cursor.x][game->cursor.y] = TOWER_NORMAL;
                            if(find_path(game, start, end, test_path, &test_path_length)) {
                                game->gold -= 10;
                            } else {
                                game->grid[game->cursor.x][game->cursor.y] = TOWER_NONE;
                            }
                        }
                    } else {
                        TowerType current = game->grid[game->cursor.x][game->cursor.y];
                        TowerType new_type = next_tower_type(current);
                        game->grid[game->cursor.x][game->cursor.y] = new_type;
                        if(new_type != TOWER_NONE &&
                           !find_path(game, start, end, test_path, &test_path_length)) {
                            game->grid[game->cursor.x][game->cursor.y] = current;
                        } else if(new_type == TOWER_NONE) {
                            game->gold += 5; // Sell tower
                        }
                    }
                    break;
                }
                case InputKeyBack:
                    // Exit the app
                    // To prevent memory leaks, you'd need to break the loop here.
                    // For now, it's an infinite loop as in the original code.
                    break;
                default:
                    break;
                }
            }
        }

        // Game logic updates
        if(game->pre_wave_timer > 0) {
            game->pre_wave_timer--;
        } else {
            Wave wave_params = get_wave_params(game->wave);
            if(game->wave_spawn_index < wave_params.enemy_count) {
                game->wave_spawn_timer--;
                if(game->wave_spawn_timer <= 0) {
                    int i = game->wave_spawn_index++;
                    game->enemies[i].active = true;
                    game->enemies[i].hp = wave_params.enemy_hp;
                    game->enemies[i].path_index = 0;
                    game->enemies[i].progress = 0.0f;
                    game->enemies[i].freeze_timer = 0;
                    game->enemies[i].pos = (Coord){0, 0};
                    game->wave_spawn_timer = wave_params.spawn_interval_ticks;
                }
            }
        }
        update_enemies(game);
        update_tower_firing(game);
        update_projectiles(game);
        if(game->wave_spawn_index >= get_wave_params(game->wave).enemy_count &&
           all_enemies_inactive(game)) {
            game->wave++;
            spawn_wave(game);
        }

        view_port_update(view_port);
        furi_mutex_release(game_mutex);
    }

    // Cleanup (This part is unreachable in the current code but is good practice)
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close("gui");
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    furi_mutex_free(game_mutex);
    free(game);
    return 0;
}
