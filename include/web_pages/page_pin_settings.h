#ifndef PAGE_PIN_SETTINGS_H
#define PAGE_PIN_SETTINGS_H

const char pin_settings_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PIN Settings</title>
    <style>
        body { font-family: Arial, sans-serif; background-color: #222; color: #eee; padding: 20px; }
        .container { max-width: 500px; margin: auto; background: #333; padding: 20px; border-radius: 8px; }
        h1 { color: #00aaff; }
        label { display: block; margin-top: 10px; }
        input[type="password"], input[type="text"], input[type="number"] { width: calc(100% - 22px); padding: 10px; margin-top: 5px; border-radius: 4px; border: 1px solid #555; background: #444; color: #eee; }
        .button { background-color: #00aaff; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; text-decoration: none; display: inline-block; margin-top: 20px; }
        .message { margin-top: 15px; padding: 10px; border-radius: 4px; }
        .success { background-color: #4CAF50; color: white; }
        .error { background-color: #f44336; color: white; }
        .nav { margin-bottom: 20px; }
        .nav a { color: #00aaff; text-decoration: none; margin-right: 15px; }
        .checkbox-label { display: flex; align-items: center; margin-top: 20px; }
        .checkbox-label input { margin-right: 10px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="nav">
            <a href="/">Home</a>
            <a href="/logout">Logout</a>
        </div>
        <h1>PIN Code Settings</h1>
        <div id="message" class="message" style="display:none;"></div>

        <form id="pinSettingsForm">
            <div class="checkbox-label">
                <input type="checkbox" id="pin_enabled" name="enabled">
                <label for="pin_enabled">Enable PIN Protection</label>
            </div>

            <label for="pin_length">PIN Length (4-10)</label>
            <input type="number" id="pin_length" name="length" min="4" max="10">

            <label for="new_pin">Set/Change PIN</label>
            <input type="password" id="new_pin" name="pin" pattern="\d{4,10}" title="4 to 10 digits">
            
            <label for="pin_confirm">Confirm PIN</label>
            <input type="password" id="pin_confirm" name="pin_confirm">

            <button type="submit" class="button">Save Settings</button>
        </form>
    </div>

    <script>
        function showMessage(text, type) {
            const messageDiv = document.getElementById('message');
            messageDiv.textContent = text;
            messageDiv.className = 'message ' + type;
            messageDiv.style.display = 'block';
            setTimeout(() => { messageDiv.style.display = 'none'; }, 4000);
        }

        document.addEventListener('DOMContentLoaded', function() {
            fetch('/api/pincode_settings')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('pin_enabled').checked = data.enabled;
                    document.getElementById('pin_length').value = data.length;
                })
                .catch(error => console.error('Error loading PIN status:', error));
        });

        document.getElementById('pinSettingsForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const isEnabled = document.getElementById('pin_enabled').checked;
            const newPin = document.getElementById('new_pin').value;
            const confirmPin = document.getElementById('pin_confirm').value;
            const length = document.getElementById('pin_length').value;
            
            const formData = new FormData();
            formData.append('enabled', isEnabled);
            formData.append('length', length);

            if (isEnabled && newPin.length > 0) {
                if (newPin.length < 4 || newPin.length > 10) {
                    showMessage('PIN must be between 4 and 10 digits.', 'error');
                    return;
                }
                if (newPin !== confirmPin) {
                    showMessage('PINs do not match.', 'error');
                    return;
                }
                formData.append('pin', newPin);
                formData.append('pin_confirm', confirmPin);
            }

            fetch('/api/pincode_settings', {
                method: 'POST',
                body: formData
            })
            .then(response => {
                if (!response.ok) {
                    return response.text().then(text => { throw new Error(text || 'Server error') });
                }
                return response.text();
            })
            .then(data => {
                showMessage(data, 'success');
                document.getElementById('new_pin').value = '';
                document.getElementById('pin_confirm').value = '';
            })
            .catch(error => {
                showMessage(error.message, 'error');
            });
        });
    </script>
</body>
</html>
)rawliteral";

#endif