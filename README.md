# Flipper TD - A Tower Defense Game for Flipper Zero

A classic tower defense game built from the ground up for the Flipper Zero. Place towers, defeat waves of enemies, and protect your base in this fun homebrew project.

---

### ⚠️ Disclaimer: Work in Progress ⚠️

This project is currently in active development. The core gameplay is functional, but many features are still being implemented. Expect bugs, placeholder graphics, and frequent changes.

---

## Gameplay

The objective of Flipper TD is to prevent waves of enemies from crossing the screen from the top-left corner to the bottom-right corner.

![Flipper TD Gameplay](https://purplefox.io/images/flipper_td_gameplay.png)

* **Place Towers:** Use the D-pad to move your cursor and the OK button to place towers on the grid. Each tower costs gold.
* **Earn Gold:** Defeating an enemy rewards you with gold.
* **Manage Lives:** You start with a set number of lives. Each enemy that reaches the exit will cost you one life. If you run out of lives, the game is over.
* **Survive Waves:** Each wave brings stronger and faster enemies. After all enemies in a wave are defeated, a new wave will begin after a short delay.

#### Tower Types
* **(N) Normal Tower:** Your standard, reliable damage dealer.
* **(R) Range Tower:** Sacrifices some power for the ability to shoot enemies from further away.
* **(S) Splash Tower:** Deals damage in a small area of effect, perfect for handling clustered groups of enemies.
* **(F) Freeze Tower:** Doesn't deal much damage, but its projectiles temporarily freeze enemies, making them easy targets for other towers.

## How It Works

Flipper TD is written in C using the official Flipper Zero Furi SDK. The game's logic is managed by a few key components:

* **Game State:** A central `GameState` struct holds all runtime information, including player stats (lives, gold), grid layout, and arrays for all active enemies and projectiles.
* **Pathfinding:** Enemies navigate the grid using a **Breadth-First Search (BFS)** algorithm. This allows them to dynamically find the shortest path to the exit, even as you place new towers. The game also uses this algorithm to prevent you from placing a tower that would completely block the enemy's path.
* **Game Loop:** The core application is a continuous loop that processes user input, updates the state of all game objects (towers, enemies, projectiles), and renders the final image to the screen for each frame.

## Getting Started

To compile and run Flipper TD on your device, you need to have the Flipper Zero build toolchain (`ufbt`) set up.

**1. Clone the Repository:**
Clone this repository to your local machine into your Flipper Zero applications folder.

```bash
git clone [https://github.com/purplefox-io/flipper-td.git](https://github.com/purplefox-io/flipper-td.git)
```
**2. Build and Launch**
Connect your Flipper Zero to your computer via USB. Then, navigate to the cloned directory and run the following command:

```bash
ufbt launch
```
This command will compile the C code, create a `.fap` application file, and install and run it on your connected Flipper Zero.

### Project Roadmap

The project is still in its early stages. Here are some of the features planned for the future:
- Pixel Art: Replace placeholder letters with actual sprites for towers and enemies.
- Tower Upgrades: Allow players to spend gold to upgrade the damage, range, or abilities of existing towers.
- Expanded Economy: Introduce varying tower costs and enemy gold rewards.
- UI/UX Polish: Implement a main menu, a proper "Game Over" screen, and better visual feedback.
- Sound Effects: Add sounds for tower attacks, enemy deaths, and UI interactions.
- More Tower & Enemy Types: Introduce new strategic elements to the game.

### Contributions

Contributions are welcome! If you have an idea for a new feature, find a bug, or want to help with development, feel free to open an issue or submit a pull request. This project can be followed on here and https://purplefox.io/blog/flipper-td

