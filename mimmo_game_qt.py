import time
from PySide6.QtWidgets import QApplication, QWidget, QPushButton, QSlider, QLabel
from PySide6.QtCore import QTimer
import random


timer = QTimer()



current_target = 0
correct_hits = 0 
game_index = 0
pattern = []
total_targets = 0
score_label = None
feedback_label = None
hit_registered = False


def start(buttons, new_pattern, speed,rounds, rand_hit, new_score_label, new_feedback_label):

    global pattern
    global correct_hits
    global score_label
    global feedback_label
    global total_targets

    score_label = new_score_label
    feedback_label = new_feedback_label

    total_targets = len(new_pattern) * rounds

    score_label.setText(f"0/{total_targets}")
    feedback_label.setText("")

    correct_hits=0

    pattern = new_pattern

    try:
        timer.timeout.disconnect()
    except TypeError:
        pass

    timer.timeout.connect(
        lambda: next_target(buttons, speed, rounds,rand_hit)
    )
    

    countdown(feedback_label)
    sequence(buttons, pattern, speed,rounds)

def randomise_hit(target, rand_hit):

    # --------------------------------
    # 1. How often do we change
    #    the original target?
    # --------------------------------

    random_probability = rand_hit * 0.5

    if random.random() >= random_probability:
        return target


    # --------------------------------
    # 2. Higher randomness = more slips
    # --------------------------------

    if rand_hit > 0.5:

        defensive_probability = (rand_hit - 0.5) * 2
        defensive_probability *= 0.5

        if random.random() < defensive_probability:
            return 0


    # --------------------------------
    # 3. Otherwise, choose another
    #    target using the same hand
    # --------------------------------

    if target % 2 == 1:
        possible_targets = [1, 3, 5]

    else:
        possible_targets = [2, 4, 6]

    # Don't repeat the original punch
    possible_targets.remove(target)

    return random.choice(possible_targets)
    current_target = possible_targets



def countdown(feedback_label):
    for i in range (3,0,-1):
        print(i)
        feedback_label.setStyleSheet("""
            QLabel {
                color: green;
                font-size: 48px;
                font-weight: bold;
            }
        """)
        feedback_label.setText(str(i))
        QApplication.processEvents()


        time.sleep(1)

    feedback_label.setText("GO!")
    QApplication.processEvents()
    print("GO!")

    time.sleep(1)

    feedback_label.setText("Playing")

def next_target(buttons, speed, rounds, rand_hit):
    global game_index
    global current_target
    global current_round
    global flashing
    global hit_registered
    global correct_hits

    flash_ratio = 0.15

    # --------------------------------------------------
    # The button is currently green -> turn it OFF
    # --------------------------------------------------
    if flashing:

        buttons[current_target - 1].setStyleSheet(
            "background-color: gray;"
        )

        flashing = False
        if current_target == 0:
            # no click = correct
            correct_hits += 1
         
            score_label.setText(
                f"{correct_hits}/{total_targets}"
            )

            feedback_label.setText("SLIP")
            feedback_label.setStyleSheet("""
                QLabel {
                    color: green;
                    font-size: 24px;
                    font-weight: bold;
                }
            """)   
            ...
        elif not hit_registered:
            # normal punch wasn't hit
            feedback_label.setText("TOO SLOW")
            feedback_label.setStyleSheet("""
                QLabel {
                    color: red;
                    font-size: 24px;
                    font-weight: bold;
                }
            """)

        # Wait for the remaining 85% of the interval
        timer.start(
            int(speed * (flash_ratio) * 1000)
        )

        return

    # --------------------------------------------------
    # We are ready to show the next target
    # --------------------------------------------------

    # End of current round
    if game_index >= len(pattern):

        game_index = 0
        current_round += 1

        print("ROUND:", current_round)

        # All rounds completed
        if current_round > rounds:
            timer.stop()
            flashing=False
            current_round=0
           
           # Final score
            percentage = (correct_hits / total_targets) * 100

            print(
                f"SCORE: {correct_hits}/{total_targets}"
                        f"({percentage:.0f}%)"
                )

            # Easter egg 😎
            if percentage == 100:
                message = "PERFECT"
            elif percentage >= 75:
                message = "YOU'RE A CHAMP"
            elif percentage >= 50:
                message = "GETTING THERE PAL"
            elif percentage >= 25:
                message = "YOU CAN DO MUCH BETTER"
            elif percentage >= 10: 
                message = "GOTTA TRAIN MORE MATE"
            else:
                message = "DUDE YOU SUCK"

            feedback_label.setText(message)

            return


    feedback_label.setText("")
    current_target = pattern[game_index]
    current_target = randomise_hit(current_target, rand_hit)

    hit_registered = False
   

    print("TARGET =", current_target)

    buttons[current_target - 1].setStyleSheet(
        "background-color: green;"
    )

    flashing = True

    # Keep button green for 15% of the speed
    timer.start(
        int(speed * (1-flash_ratio) * 1000)
    )

    game_index += 1


def sequence(buttons, pattern, speed, rounds):
    global current_round
    global game_index
    global flashing
    global hit_registered

    # Reset game state
    timer.stop()
    current_round = 1
    game_index = 0
    flashing = False

    # Make sure all buttons start gray
    for button in buttons:
        button.setStyleSheet(
            "background-color: gray;"
        )

    # IMPORTANT:
    # Do NOT connect timer.timeout here.
    # It should already be connected once when the UI is initialized.

    timer.start(speed*1000)



def register_hit(number):

    global correct_hits
    global hit_registered

    print(f"CLICK={number}  CURRENT_TARGET={current_target}")

    if not pattern:
        print("Game not started")
        return

    # Correct target, and it hasn't already been hit
    if number == current_target and not hit_registered:

        correct_hits += 1
        hit_registered = True

        score_label.setText(
        f"{correct_hits}/{total_targets}"
        )

        feedback_label.setText("HIT")
        feedback_label.setStyleSheet("""
            QLabel {
                color: green;
                font-size: 24px;
                font-weight: bold;
            }
        """)

        print("Correct")

    # Same target clicked again
    elif number == current_target and hit_registered:

        feedback_label.setText("ALREADY HIT")
        feedback_label.setStyleSheet("""
            QLabel {
                color: red;
                font-size: 20px;
                font-weight: bold;
            }
        """)

        print("Already registered")

    else:

        feedback_label.setText("WRONG")
        feedback_label.setStyleSheet("""
            QLabel {
                color: red;
                font-size: 24px;
                font-weight: bold;
            }
        """)

        print("Wrong")