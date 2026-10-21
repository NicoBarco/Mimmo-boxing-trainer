# MIMMO — Interactive Boxing Trainer

**Work in progress | Python, PySide6, Arduino, C/C++**

Mimmo is a personal software and hardware project designed to create an interactive boxing training system.

The project started as a Python desktop application and has expanded into an Arduino-based physical controller, combining software development, real-time interaction, electronics and hardware integration.

## Features

### Python Desktop Application
- Graphical interface built with Python and PySide6
- Configurable training rounds and speed
- Boxing combinations stored in JSON
- Custom combination editor
- Adjustable randomization of training sequences
- Defensive actions and visual timing feedback

### Arduino Controller (C/C++)
- Arduino Nano-based standalone controller
- LCD interface and infrared remote control
- Adjustable training speed
- Randomized punching sequences and defensive actions
- LED target control and buzzer feedback
- Training statistics and response evaluation
- Experimental servo-controlled defensive mechanism

## Technologies

- **Languages:** Python, C/C++
- **GUI:** PySide6 / Qt
- **Hardware:** Arduino Nano, LCD, LEDs, IR receiver, potentiometers and servo motors
- **Data:** JSON
- **Version control:** Git / GitHub

## Project Structure

```text
mimmo/
├── app.py
├── combos.json
├── mimmo_qt.py
├── mimmo_game_qt.py
├── mimmo_combo_qt.py
├── arduino/
│   └── mimmo_controller/
│       └── mimmo_controller.ino
└── README.md
```

## Development Status

Mimmo is an ongoing prototype. The desktop application and Arduino controller are under active development, with continued work on hardware integration, training logic, scoring and physical feedback.

The long-term goal is a standalone interactive boxing trainer capable of generating training sequences, detecting user responses and providing immediate feedback.

## Author

[NicoBarco](https://github.com/NicoBarco)