from PySide6.QtWidgets import QApplication, QWidget, QPushButton, QSlider, QLabel, QComboBox, QGroupBox, QLineEdit, QMessageBox
from PySide6.QtCore import Qt
import json

combo_window = None

def create_combo_window(on_saved):
    
    print("CREATE COMBO CLICKED")

    global combo_window


    combo_window = QWidget()
    combo_window.setWindowTitle("Create Your Combo")
    combo_window.resize(350, 180)

    # Name
    name_label = QLabel("Combo name", combo_window)
    name_label.move(20, 20)

    name_input = QLineEdit(combo_window)
    name_input.move(20, 45)
    name_input.resize(200, 25)

    # Pattern
    pattern_label = QLabel("Pattern", combo_window)
    pattern_label.move(20, 75)

    pattern_input = QLineEdit(combo_window)
    pattern_input.setPlaceholderText("e.g. 1,2,3,4")
    pattern_input.move(20, 100)
    pattern_input.resize(250, 25)

    # Save
    save_button = QPushButton("SAVE", combo_window)
    save_button.move(190, 140)
    save_button.clicked.connect(
    lambda: save_combo(
        name_input,
        pattern_input,
        save_button,
        combo_window,
        on_saved       
    )
)

    # Close
    close_button = QPushButton("CLOSE", combo_window)
    close_button.move(270, 140)

    close_button.clicked.connect(combo_window.close)

    combo_window.show()

    return combo_window


def save_combo(name_input, pattern_input, save_button, combo_window, on_saved):

    name = name_input.text()

    pattern_text = pattern_input.text()

    pattern = [
        int(x)
        for x in pattern_text.split(",")
    ]

    with open("combos.json", "r") as file:
        data = json.load(file)

    data["sequences"].append({
        "name": name,
        "pattern": pattern
    })

    with open("combos.json", "w") as file:
        json.dump(data, file, indent=4)

    QMessageBox.information(
        combo_window,
        "Combo saved",
        "Combo saved successfully!"
    )

    save_button.setEnabled(False)
    
    on_saved()

    combo_window.close()

