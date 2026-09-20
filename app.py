import streamlit as st
import time
from game import countdown, sequence, creation
import json



if "game_state" not in st.session_state:
    st.session_state.game_state = "idle"

st.title ("Boxing Trainer, Mimmo")

st.write(
    "Mimmo status:",
    st.session_state.game_state
)


with open("combos.json", "r") as file:
    data = json.load(file)

combo_names = []

for combo in data["sequences"]:
    combo_names.append(combo["name"])



selected_name = st.selectbox(
    "Choose training",
    combo_names
)
with st.expander("Advanced combos"):
    st.subheader("Create new combo")

    combo_name = st.text_input("Combo name")

    combo_pattern = st.text_input(
        "Pattern (example: 1,2,3,6,5,4)"
    )

    save = st.button("Save combo")

if save:
    pattern = [
        int(x)
        for x in combo_pattern.split(",")
    ]

    new_combo = {
        "name": combo_name,
        "pattern": pattern
    }
    with open("combos.json", "r") as file:
        data = json.load(file)

        data["sequences"].append(new_combo)

    with open("combos.json", "w") as file:
        json.dump(data, file, indent=4)
    st.rerun()



selected_pattern = None

for combo in data["sequences"]:
    if combo["name"] == selected_name:
        selected_pattern = combo["pattern"]


speed = st.slider(
    "Speed",
    min_value=0.1,
    max_value=2.0,
    value=0.5,
    step=0.1
)

st.write(speed)


randomness = st.slider(
    "Randomness",
    min_value=0,
    max_value=100,
    value=0
)

st.write(randomness)

rounds = st.slider(
    "Rounds",
    min_value=0,
    max_value=100,
    value=3
)

st.write(randomness)

start = st.button("Start")


row1 = st.columns(3)
row2 = st.columns(3)

columns = row1 + row2

buttons = [] 

for col in columns: 
    with col: buttons.append(st.empty())

if start:
    creation(buttons)
    countdown()
    st.session_state.game_state = "running"
    if st.session_state.game_state == "running":

        sequence(
            rounds,
            speed,
            buttons,
            selected_pattern
        )

    st.session_state.game_state = "idle"



