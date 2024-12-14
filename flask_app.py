from flask import Flask, request, jsonify

app = Flask(__name__)

# Store the LED state, brightness, and mode
led_state = {"brightness": 0, "status": "OFF"}
mode = "DIM"  # Default mode
light_value = 0  # Default light value, to be updated by ESP32

@app.route('/')
def home():
    # Return a simple HTML page with inline JavaScript
    return '''
    <html>
        <head>
            <title>ESP32 LED Control</title>
            <script src="https://code.jquery.com/jquery-3.6.0.min.js"></script>
        </head>
        <body>
            <h1>ESP32 LED Controller</h1>
            
            <!-- Light Sensor Data -->
            <h2>Light Sensor Value: <span id="sensor_value">No Data</span></h2>

            <!-- LED State -->
            <h3>LED Status: <span id="led_status">OFF</span></h3>
            <h3>LED Brightness: <span id="led_brightness">0</span></h3>

            <h3>Mode: <span id="current_mode">DIM</span></h3>

            <div>
                <h3>Control LED:</h3>
                <button id="turn_on">Turn On</button>
                <button id="turn_off">Turn Off</button>
                <br>
                <label for="brightness">Set Brightness:</label>
                <input type="range" id="brightness" min="0" max="255" step="1" value="0">
            </div>

            <h3>Set Mode:</h3>
            <button class="mode" data-mode="DIM">DIM</button>
            <button class="mode" data-mode="READ">READ</button>
            <button class="mode" data-mode="FULL">FULL</button>

            <script>
                $(document).ready(function() {
                    // Poll the server for updates every second
                    setInterval(function() {
                        $.get("/get_led_status", function(data) {
                            $('#led_status').text(data.status);
                            $('#led_brightness').text(data.brightness);
                        });
                        $.get("/get_mode", function(data) {
                            $('#current_mode').text(data.mode);
                        });
                        $.get("/get_light_value", function(data) {
                            $('#sensor_value').text(data.light_value);  // Update light sensor value
                        });
                    }, 1000); // Poll every second

                    // Function to update LED state
                    function update_led(brightness, status) {
                        $.ajax({
                            url: '/led',
                            type: 'POST',
                            contentType: 'application/json',
                            data: JSON.stringify({ brightness: brightness, status: status }),
                            success: function(response) {
                                $('#led_status').text(response.status);
                                $('#led_brightness').text(response.brightness);
                            }
                        });
                    }

                    // Function to set mode
                    function set_mode(mode) {
                        $.ajax({
                            url: '/set_mode',
                            type: 'POST',
                            data: { mode: mode },
                            success: function(response) {
                                $('#current_mode').text(response.mode);
                            }
                        });
                    }

                    // Turn LED on
                    $('#turn_on').click(function() {
                        update_led($('#brightness').val(), 'ON');
                    });

                    // Turn LED off
                    $('#turn_off').click(function() {
                        update_led(0, 'OFF');
                    });

                    // Change brightness
                    $('#brightness').on('input', function() {
                        update_led($(this).val(), $('#led_status').text());
                    });

                    // Change mode
                    $('.mode').click(function() {
                        const mode = $(this).data('mode');
                        set_mode(mode);
                    });
                });
            </script>
        </body>
    </html>
    '''

@app.route('/update_sensor', methods=['POST'])
def update_sensor():
    try:
        # Get light sensor data from the request
        data = request.get_json()
        global light_value
        light_value = data.get('light_value')
        
        if light_value is not None:
            print(f"Received light value: {light_value}")
            # Here you can store or process the light_value as needed
            return jsonify({"status": "success", "light_value": light_value}), 200
        else:
            return jsonify({"status": "error", "message": "No light value in request"}), 400
    except Exception as e:
        print(f"Error: {e}")
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/led', methods=['POST'])
def update_led():
    try:
        # Get LED state data from the request
        data = request.get_json()
        brightness = data.get('brightness')
        status = data.get('status')
        
        if brightness is not None and status is not None:
            # Update the LED state
            led_state["brightness"] = brightness
            led_state["status"] = status
            print(f"LED State - Brightness: {brightness}, Status: {status}")
            return jsonify({"status": "success", "brightness": brightness, "status": status}), 200
        else:
            return jsonify({"status": "error", "message": "Missing brightness or status"}), 400
    except Exception as e:
        print(f"Error: {e}")
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/set_mode', methods=['POST'])
def set_mode():
    try:
        # Get mode from the request
        mode_value = request.form.get('mode')
        
        if mode_value:
            global mode
            mode = mode_value  # Update the current mode
            print(f"Mode set to: {mode}")
            return jsonify({"status": "success", "mode": mode}), 200
        else:
            return jsonify({"status": "error", "message": "No mode value in request"}), 400
    except Exception as e:
        print(f"Error: {e}")
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/get_led_status', methods=['GET'])
def get_led_status():
    return jsonify(led_state), 200


@app.route('/get_mode', methods=['GET'])
def get_mode():
    return jsonify({"mode": mode}), 200


@app.route('/get_light_value', methods=['GET'])
def get_light_value():
    return jsonify({"light_value": light_value}), 200


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
