# lucky-box-game-show
Just for Fun ;)
# Lucky Box Game Show

A terminal-based game show developed using **C programming**. The game features randomly assigned prize boxes, banker's deals, mystery rewards, lifelines, lucky spins and bonus challenges.

This project demonstrates modular C programming, structures, pointers, file handling, dynamic library creation and Makefile-based compilation.

## Features

### Game Mechanics

- **Random Prize Distribution:** Prize amounts are randomly assigned to boxes at the beginning of every game.
- **Banker's Deal:** The banker offers a cash amount at specific stages. Accept the deal or continue playing.
- **Mystery Box:** Unlock random bonuses, cash rewards or penalties.
- **Lucky Spin:** Spin a virtual wheel to win additional cash, multipliers or deductions.
- **Double or Nothing:** Guess a randomly generated number to double your winnings or risk losing your accumulated bonus.

### Lifeline System

Each player receives three lifelines per game.

| Lifeline | Description |
|---|---|
| 50:50 | Eliminates two possible mystery-box outcomes. |
| Box Swap | Exchange your selected box with another unopened box. |
| Prize Peek | Reveal the prize inside another unopened box. |

Each lifeline can only be used once per game.

## Project Structure

```text
lucky-box-game-show/
│
├── game.h          # Function declarations and structures
├── game.c          # Function definitions and game logic
├── main.c          # Main program and gameplay flow
├── Makefile        # Automated compilation
├── data.txt        # Input file containing prize amounts
└── results.txt     # Generated game results
```

## Technologies Used

- C Programming
- Structures and Arrays
- Functions and Modular Programming
- Pointers
- File Handling
- Dynamic Link Libraries (DLL)
- GCC (MinGW)
- GNU Make

## Prerequisites

- Windows 10/11
- MinGW GCC compiler
- GNU Make
- VS Code (optional)

Make sure GCC and Make are added to your system PATH.

Verify the installation:

```bash
gcc --version
make --version
```

## Compilation and Execution

### 1. Clone the Repository

```bash
git clone https://github.com/YOUR_USERNAME/lucky-box-game-show.git
```

### 2. Navigate to the Project Directory

```bash
cd lucky-box-game-show
```

### 3. Compile the Project

```bash
make
```

This generates:

- `game.dll` - Windows dynamic link library containing the game functions.
- `luckybox.exe` - Executable program.

### 4. Run the Game

```bash
./luckybox.exe
```

### 5. Clean Generated Files

```bash
make clean
```

This removes the generated DLL, executable and game result file.

## File Handling

The project uses text files to read input data and maintain game records.

### Input File: `data.txt`

Contains the prize amounts assigned randomly to the boxes at runtime.

Example:

```text
100
500
1000
5000
10000
25000
50000
100000
```

### Output File: `results.txt`

Stores completed game results, including:

- Game timestamp
- Initial box selection
- Final box selection
- Total prize amount
- Bonus cash
- Whether the player swapped boxes

Game results are appended to the file, preserving previous records even after the program exits.

## Dynamic Library Architecture

The project is organized into three primary C files.

| File | Responsibility |
|---|---|
| `game.h` | Structures, constants and function declarations |
| `game.c` | Function definitions and reusable game logic |
| `main.c` | Main function and interaction with library functions |

### Creating the DLL

The `game.c` source file is compiled into a Windows dynamic link library using MinGW GCC:

```bash
gcc -Wall -Wextra -std=c11 -shared -o game.dll game.c
```

### Linking the Main Program

The main program links against the dynamic library:

```bash
gcc -Wall -Wextra -std=c11 -o luckybox.exe main.c -L. -lgame
```

The Makefile automates these compilation steps.

## Learning Objectives

This project provides practical experience with:

- Designing modular C applications.
- Working with structures and arrays of structures.
- Implementing reusable functions.
- Reading and writing persistent data using file handling.
- Creating and linking dynamic libraries.
- Automating compilation using Makefiles.
- Implementing randomization and game logic.
- Managing project files using Git and GitHub.

## Future Improvements

- Multiple difficulty levels.
- Persistent player leaderboard.
- Multiplayer support.
- Additional game rounds and achievements.
- Improved game statistics.

---

**Developed as a C programming project to demonstrate modular programming, dynamic libraries and file handling.**
