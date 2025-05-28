import streamlit as st
import serial
import serial.tools.list_ports
from openai import OpenAI
import time
import platform # Not strictly necessary for this improved version, but good to keep if platform-specific logic is added later

# --- Configuration Constants ---
BAUD_RATE = 9600
SERIAL_TIMEOUT = 3
ARDUINO_CONNECT_DELAY = 2 # Time to wait for Arduino to reset after serial connection
COMMAND_SEND_DELAY = 1 # Time to wait after sending a command
ARDUINO_READ_TIMEOUT = 0.5 # Shorter timeout for reading immediate responses

# --- Streamlit UI Enhancements ---
st.set_page_config(
    page_title="🧠 AI-Powered Arduino Control",
    page_icon="💡",
    layout="centered"
)

st.title("🧠 AI-Powered LED & 🔊 Piezo Speaker Control")
st.markdown("Communicate with your Arduino using natural language via OpenAI's GPT models.")

# --- Session State Management ---
if 'client' not in st.session_state:
    st.session_state.client = None
if 'api_key_entered' not in st.session_state:
    st.session_state.api_key_entered = False
if 'arduino_port' not in st.session_state:
    st.session_state.arduino_port = None
if 'serial_connection' not in st.session_state:
    st.session_state.serial_connection = None
if 'last_command' not in st.session_state:
    st.session_state.last_command = ""

# --- Helper Functions ---

@st.cache_data(ttl=60) # Cache port list for 60 seconds
def get_available_ports():
    """Fetches and returns a list of available serial ports."""
    return [port.device for port in serial.tools.list_ports.comports()]

def connect_to_arduino(port_name):
    """Establishes a serial connection to the Arduino."""
    try:
        if st.session_state.serial_connection and st.session_state.serial_connection.is_open:
            st.session_state.serial_connection.close() # Close existing connection if any

        arduino = serial.Serial(port_name, BAUD_RATE, timeout=SERIAL_TIMEOUT)
        time.sleep(ARDUINO_CONNECT_DELAY) # Wait for Arduino to reset
        st.session_state.serial_connection = arduino
        st.success(f"Connected to Arduino on port: **{port_name}**")
        return True
    except serial.SerialException as e:
        st.error(f"Failed to connect to Arduino: {e}. Please ensure the port is correct and not in use.")
        st.session_state.serial_connection = None
        return False
    except Exception as e:
        st.error(f"An unexpected error occurred during connection: {e}")
        st.session_state.serial_connection = None
        return False

def close_arduino_connection():
    """Closes the serial connection if it's open."""
    if st.session_state.serial_connection and st.session_state.serial_connection.is_open:
        st.session_state.serial_connection.close()
        st.session_state.serial_connection = None
        st.info("Arduino connection closed.")

def send_command_to_arduino(command):
    """Sends a command to the Arduino and reads responses."""
    if not st.session_state.serial_connection or not st.session_state.serial_connection.is_open:
        st.warning("Arduino is not connected. Please connect first.")
        return

    try:
        with st.spinner("📡 Sending to Arduino..."):
            st.session_state.serial_connection.write((command + "\n").encode('utf-8'))
            st.session_state.serial_connection.flush() # Ensure data is sent
            time.sleep(COMMAND_SEND_DELAY)

            # Read initial response (e.g., "Ready for commands")
            if st.session_state.serial_connection.in_waiting > 0:
                arduino_response = st.session_state.serial_connection.readline().decode('utf-8').strip()
                st.success(f"Arduino says: {arduino_response}")
            else:
                st.success("✅ Command sent to Arduino!")

            # Read processing response if any (optional, based on Arduino code logic)
            time.sleep(ARDUINO_READ_TIMEOUT) # Give Arduino a moment to process and respond
            if st.session_state.serial_connection.in_waiting > 0:
                processing_response = st.session_state.serial_connection.readline().decode('utf-8').strip()
                if processing_response: # Only display if there's actual content
                    st.info(f"Arduino processing: {processing_response}")

    except serial.SerialException as e:
        st.error(f"Serial communication error: {e}. Attempting to re-establish connection.")
        close_arduino_connection()
        # Optionally, try to reconnect here
    except Exception as e:
        st.error(f"An unexpected error occurred during command transmission: {e}")


# --- Sidebar for Configuration ---
with st.sidebar:
    st.header("⚙️ Configuration")

    api_key = st.text_input("Enter OpenAI API Key", type="password", key="openai_api_key")
    if api_key and not st.session_state.api_key_entered:
        st.session_state.api_key_entered = True
        st.session_state.client = OpenAI(api_key=api_key)
        st.success("OpenAI API Key set!")
    elif not api_key and st.session_state.api_key_entered: # Clear API key if user deletes it
        st.session_state.api_key_entered = False
        st.session_state.client = None
        st.info("OpenAI API Key cleared.")

    st.subheader("Arduino Connection")
    available_ports = get_available_ports()
    if available_ports:
        selected_port = st.selectbox(
            "Select Arduino Port:",
            options=["-- Select --"] + available_ports,
            index=0 if not st.session_state.arduino_port else available_ports.index(st.session_state.arduino_port) + 1 if st.session_state.arduino_port in available_ports else 0,
            key="port_selector"
        )
        if selected_port != "-- Select --":
            if st.session_state.arduino_port != selected_port:
                st.session_state.arduino_port = selected_port
                if st.button("Connect to Arduino", key="connect_btn"):
                    connect_to_arduino(st.session_state.arduino_port)
            elif not st.session_state.serial_connection or not st.session_state.serial_connection.is_open:
                if st.button("Connect to Arduino", key="connect_btn_reconnect"):
                    connect_to_arduino(st.session_state.arduino_port)
            else:
                st.success(f"Currently connected to {st.session_state.arduino_port}")
                if st.button("Disconnect from Arduino", key="disconnect_btn"):
                    close_arduino_connection()
        else:
            st.warning("No port selected or available.")
    else:
        st.warning("No Arduino ports found. Please connect your Arduino.")
        manual_port = st.text_input("Enter Arduino port manually (e.g., COM3, /dev/ttyACM0):", key="manual_port_input")
        if manual_port:
            if st.button("Connect to Manual Port", key="manual_connect_btn"):
                st.session_state.arduino_port = manual_port
                connect_to_arduino(st.session_state.arduino_port)

    # Display connection status
    if st.session_state.serial_connection and st.session_state.serial_connection.is_open:
        st.success("Arduino is LIVE!")
    else:
        st.error("Arduino is DISCONNECTED.")

# --- Main Application Area ---
st.header("🗣️ Your Command")
user_input = st.text_area(
    "Describe your desired LED and Sound action:",
    placeholder="e.g., 'Make the red LED blink fast and play a beep', 'Turn off yellow and stop sound', 'Play alarm with green LED'",
    height=100,
    key="user_command_input"
)

col1, col2 = st.columns(2)

with col1:
    send_button = st.button("🚀 Send Command", use_container_width=True, type="primary")
with col2:
    if st.session_state.last_command:
        st.button("♻️ Repeat Last Command", on_click=lambda: send_command_to_arduino(st.session_state.last_command), use_container_width=True)


if send_button:
    if not st.session_state.api_key_entered or not st.session_state.client:
        st.error("Please enter your OpenAI API Key in the sidebar.")
    elif not st.session_state.serial_connection or not st.session_state.serial_connection.is_open:
        st.error("Please connect to your Arduino first in the sidebar.")
    elif not user_input.strip():
        st.warning("Please enter a command.")
    else:
        try:
            with st.spinner("🤖 Interpreting command with AI..."):
                response = st.session_state.client.chat.completions.create(
                    model="gpt-4o", # Using gpt-4o for better performance
                    messages=[
                        {
                            "role": "system",
                            "content": """Convert natural language to simplified LED and piezo speaker control commands for an Arduino.
                                          Only output plain commands, no explanations, no greetings.
                                          Combine multiple actions on a single line if applicable, separated by spaces.
                                          Strictly adhere to the following command format:

LED commands:
- 'red off'
- 'red on'
- 'red fast' (for fast blink)
- 'red slow' (for slow blink)
- 'red blink' (for default blink)

Sound commands:
- 'beep'
- 'alarm'
- 'melody'
- 'tone high'
- 'tone low'
- 'sound off'

Examples of combined commands:
- 'red blink beep'
- 'green on tone high'
- 'yellow off sound off'
- 'alarm red on'
- 'melody red fast'
- 'red off yellow off green off sound off' (to turn everything off)
- 'red blink yellow blink green blink' (to make all blink)
"""
                        },
                        {"role": "user", "content": user_input},
                    ],
                    max_tokens=50 # Adjusted max_tokens for concise commands
                )

            ai_generated_command = response.choices[0].message.content.strip()
            st.session_state.last_command = ai_generated_command # Store for repeat button

            st.markdown(f"**AI-Generated Command:**")
            st.code(ai_generated_command, language='text')

            send_command_to_arduino(ai_generated_command)

        except Exception as e:
            st.error(f"Error processing command with AI or Arduino: {e}")
            if "authentication" in str(e).lower() or "api_key" in str(e).lower():
                st.error("Please double-check your OpenAI API Key.")

# --- Instructions and Setup ---
st.markdown("---") # Separator
st.header("📚 Instructions & Setup")

st.markdown("""
### 💡 Command Examples:
You can be conversational! The AI will convert your input to simple commands.

- **LEDs**:
    - "Turn on the red light" -> `red on`
    - "Make the green LED blink quickly" -> `green fast`
    - "Stop the yellow light" -> `yellow off`
    - "Have all LEDs blink" -> `red blink yellow blink green blink`

- **Sounds**:
    - "Play a short beeping sound" -> `beep`
    - "Start the siren" -> `alarm`
    - "Play a nice tune" -> `melody`
    - "Make a continuous high pitch" -> `tone high`
    - "Silence the speaker" -> `sound off`

- **Combined Actions**:
    - "Red LED on with a beep" -> `red on beep`
    - "Green LED blinking slow and play an alarm" -> `green slow alarm`
    - "Turn everything off" -> `red off yellow off green off sound off`
""")

st.markdown("""
### 🔌 Arduino Pin Setup:
Ensure your Arduino is wired as follows:

- **Red LED**: Digital Pin 13
- **Yellow LED**: Digital Pin 12
- **Green LED**: Digital Pin 11
- **Piezo Speaker**: Digital Pin 8 (one leg to pin 8, other leg to GND, often with a 100-220 ohm resistor in series)

Make sure the Arduino sketch provided is uploaded to your board.
""")

st.markdown("""
### Troubleshooting:
- If the Arduino doesn't respond, try pressing the "Connect to Arduino" button again or checking the serial monitor in the Arduino IDE to ensure the port is correct and the Arduino is responding.
- Close the Arduino IDE's Serial Monitor if it's open, as it can block the port.
- Check your wiring!
""")
