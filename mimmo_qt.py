from PySide6.QtWidgets import QApplication, QWidget, QPushButton, QSlider, QLabel, QComboBox, QGroupBox, QLineEdit
from PySide6.QtCore import Qt
from mimmo_game_qt import start, register_hit
from mimmo_combo_qt import create_combo_window, save_combo
import sys
import json


app = QApplication(sys.argv)

window = QWidget()

window.setWindowTitle("Mimmo")

window.resize(600, 520)

with open("combos.json", "r") as file:
    data = json.load(file)


def create_buttons(window, score_label, feedback_label):
    buttons = []

    positions = [

        (150, 150),
        (250, 150),
        (350, 150),

        (150, 240),
        (250, 240),
        (350, 240),
        (250, 330)

    ]

    for i, pos in enumerate(positions):

        if i == 6:
            button = QPushButton("SLIP", window)
        else:
            button = QPushButton(str(i + 1), window)

        button.move(*pos)

        button.resize(80, 80)

        buttons.append(button)
        button.clicked.connect(
        lambda checked=False, n=i+1: register_hit(n)
    )
    return buttons

score_label = QLabel("0/0", window)
score_label.move(430, 150)
score_label.resize(150, 80)

score_label.setAlignment(Qt.AlignCenter)

score_label.setStyleSheet("""
    QLabel {
        color: red;
        font-size: 42px;
        font-weight: bold;
    }
""")

feedback_label = QLabel("", window)
feedback_label.move(40, 450)
feedback_label.resize(400, 50)

feedback_label.setAlignment(Qt.AlignCenter)

feedback_label.setStyleSheet("""
    QLabel {
        font-size: 48px;
        font-weight: bold;
    }
""")



player_sequence = []



combo_box = QComboBox(window)
combo_box.move(320,50)    

for combo in data["sequences"]:
    combo_box.addItem(combo["name"])

create_combo_button = QPushButton("CREATE COMBO", window)
create_combo_button.move(450, 50)

create_combo_button.clicked.connect(create_combo_window)


def refresh_combos():
    combo_box.clear()

    with open("combos.json", "r") as file:
        data = json.load(file)

    for combo in data["sequences"]:
        combo_box.addItem(combo["name"])
    
create_combo_button.clicked.connect(
    lambda: create_combo_window(refresh_combos)
)

speed_label = QLabel("Speed", window)
speed_label.move(15, 30)
speed_slider = QSlider(Qt.Horizontal, window)
speed_slider.move(70, 30)

speed_slider.setMinimum(1)
speed_slider.setMaximum(20)
speed_slider.setValue(10)
speed_slider.resize(200, 20)

speed_value = QLabel("1.0", window)
speed_value.move(280, 30)

def update_speed(value):
    speed_value.setText(str((21-value) / 10))

speed_slider.valueChanged.connect(update_speed)

random_label = QLabel("Random", window)
random_label.move(15, 60)
random_slider = QSlider(Qt.Horizontal, window)
random_slider.setMinimum(0)
random_slider.setMaximum(10)
random_slider.setValue(0)
random_slider.move(70, 60)
random_slider.resize(200, 20)

random_value = QLabel("0.0", window)
random_value.move(280, 60)

def update_random(value):
    random_value.setText(str(value/10))

random_slider.valueChanged.connect(update_random)

rounds_label = QLabel("Rounds", window)
rounds_label.move(15, 90)
rounds_slider = QSlider(Qt.Horizontal, window)
rounds_slider.move(70, 90)
rounds_slider.resize(200, 20)

rounds_slider.setMinimum(1)
rounds_slider.setMaximum(10)
rounds_slider.setValue(2)

rounds_value = QLabel("2", window)
rounds_value.move(280, 90)

def update_rounds(value):
    rounds_value.setText(str(value))

rounds_slider.valueChanged.connect(update_rounds)

rounds = rounds_slider.value()

buttons = create_buttons(window, score_label, feedback_label)




def start_game():
    
    selected = combo_box.currentText()

    pattern = None

    for combo in data["sequences"]:
        if combo["name"] == selected:
            pattern = combo["pattern"]


    speed = (21-speed_slider.value())/ 10
    rounds = rounds_slider.value()
    rand_hit = random_slider.value()/10
    start(buttons, pattern, speed,rounds, rand_hit, score_label, feedback_label)


start_button = QPushButton("START", window)
start_button.clicked.connect(start_game)
start_button.move(10,120)




window.show()

app.exec()
