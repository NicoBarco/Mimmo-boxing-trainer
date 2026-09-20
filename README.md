# Mimmo

**Mimmo** is a work-in-progress boxing training application written in Python using PySide6.

The project started as a software-based combination trainer and is intended to evolve into a physical interactive boxing training system using Arduino-controlled lights and hit sensors.

## Current Features

- PySide6 graphical interface
- Configurable training rounds
- Adjustable combination speed
- Visual countdown before game starts
- Boxing combinations loaded from JSON
- Combination editor for creating and saving custom patterns
- Adjustable randomisation of combinations
- Defensive actions including slip, duck, block and weave
- Visual target timing during training

## Project Structure

- `app.py` — old test, bedfore switching to Q
- `mimmo_qt.py` — main user interface
- `mimmo_game_qt.py` — training and game logic
- `mimmo_combo_qt.py` — combination editor
- `combos.JSON` — combination data

## Planned Development

The next stage of the project is hardware integration.

The planned physical trainer will use an Arduino connected to a boxing bag with six illuminated strike targets and an additional defensive-action indicator. Sensors will allow the application to detect whether the requested target was actually hit.

Future development will include:

- Arduino communication
- Physical LED targets
- Hit detection
- Defensive-action detection
- Training results and statistics

## Status

Mimmo is under active development.

The current version represents a functional software prototype. Some features and architecture are expected to change as the hardware integration and input system are developed.

## Technologies

- Python
- PySide6
- JSON
- Arduino / C++ *(planned hardware integration)*
