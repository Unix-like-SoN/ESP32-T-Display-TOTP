#pragma once

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head><title>TOTP Authenticator</title><meta name="viewport" content="width=device-width, initial-scale=1"><style>body{font-family:Arial,sans-serif;background-color:#f4f4f4;margin:0;padding:20px;}h2,h3{color:#333;text-align:center;}.form-container,.content-box{max-width:800px;margin:20px auto;padding:20px;background-color:#fff;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}table{width:100%;border-collapse:collapse;}th,td{padding:12px;border:1px solid #ddd;text-align:left;}th{background-color:#4CAF50;color:white;}td.code{font-family:monospace;font-size:1.5em;font-weight:bold;color:#005b96;}input[type="text"],input[type="password"],input[type="number"],input[type="file"]{width:calc(100% - 22px);padding:10px;margin-bottom:10px;border:1px solid #ccc;border-radius:4px;}.button,.button-delete,.button-action{display:inline-block;padding:10px 15px;border:none;border-radius:4px;color:white;cursor:pointer;text-decoration:none;margin-right:10px;}.button{background-color:#008CBA;}.button-delete{background-color:#f44336;}.button-action{background-color:#555;}.tabs{overflow:hidden;border-bottom:1px solid #ccc;background-color:#f1f1f1;max-width:820px;margin:auto;border-radius:8px 8px 0 0;}.tabs button{background-color:inherit;float:left;border:none;outline:none;cursor:pointer;padding:14px 16px;transition:0.3s;}.tabs button:hover{background-color:#ddd;}.tabs button.active{background-color:#ccc;}.tab-content{display:none;padding:6px 12px;border-top:none;}.status-message{text-align:center;padding:10px;margin:10px auto;border-radius:5px;max-width:800px;}.status-ok{background-color:#d4edda;color:#155724;}.status-err{background-color:#f8d7da;color:#721c24;}code{background-color:#eee;border-radius:3px;font-family:monospace;padding:2px 4px;}</style></head><body><h2>Authenticator Control Panel</h2><div id="status" class="status-message" style="display:none;"></div><div class="tabs"><button class="tab-link active" onclick="openTab(event, 'Keys')">Keys</button><button class="tab-link" onclick="openTab(event, 'Display')">Display</button><button class="tab-link" onclick="openTab(event, 'Settings')">Settings</button><button class="tab-link" onclick="openTab(event, 'Pin')">PIN</button></div><div id="Keys" class="tab-content">
    <h3>Manage Keys</h3>
    <div class="form-container">
        <h4>Add New Key</h4>
        <form id="add-key-form">
            <label for="key-name">Name:</label>
            <input type="text" id="key-name" name="name" required>
            <label for="key-secret">Secret (Base32):</label>
            <input type="text" id="key-secret" name="secret" required>
            <button type="submit" class="button">Add Key</button>
        </form>
    </div>
    <div class="content-box">
        <h4>Current Keys</h4>
        <table id="keys-table">
            <thead>
                <tr>
                    <th>Name</th>
                    <th>Code</th>
                    <th>Time Left</th>
                    <th>Actions</th>
                </tr>
            </thead>
            <tbody>
                <!-- Keys will be loaded here by JavaScript -->
            </tbody>
        </table>
    </div>
    <div class="form-container">
        <h4>Import/Export Keys</h4>
        <button id="export-keys-btn" class="button-action">Export Keys</button>
        <button id="import-keys-btn" class="button-action">Import Keys</button>
        <input type="file" id="import-file" style="display: none;" accept=".json">
    </div>
</div>
<div id="Display" class="tab-content">
    <h3>Display Settings</h3>
    <div class="form-container">
        <h4>Theme Selection</h4>
        <form id="theme-selection-form">
            <label>
                <input type="radio" name="theme" value="light" id="theme-light"> Light Theme
            </label><br>
            <label>
                <input type="radio" name="theme" value="dark" id="theme-dark"> Dark Theme
            </label><br>
            <button type="submit" class="button">Apply Theme</button>
        </form>
    </div>
</div><div id="Settings" class="tab-content"><h3>Device Settings</h3><div class="form-container"><h4>Change Admin Password</h4><form id="change-password-form"><input type="password" id="new-password" placeholder="New Password" required><input type="password" id="confirm-password" placeholder="Confirm New Password" required><button type="submit" class="button">Change Password</button></form></div><div class="form-container"><h4>Splash Screen</h4><form id="upload-splash-form" enctype="multipart/form-data"><label for="splash-file">Upload new splash screen (RAW, 135x240):</label><input type="file" id="splash-file" accept=".raw"><button type="submit" class="button">Upload</button></form><button id="delete-splash-btn" class="button-delete">Delete Splash</button></div><div class="form-container"><h4>System</h4><button id="reboot-btn" class="button-action">Reboot Device</button><button onclick="logout()" class="button-delete">Logout</button></div></div><div id="Pin" class="tab-content"><h3>PIN Code Settings</h3><div class="form-container"><form id="pincode-settings-form"><label for="pin-enabled">Enable PIN on startup:</label><input type="checkbox" id="pin-enabled" name="enabled"><br><br><label for="pin-length">PIN Length (4-10):</label><input type="number" id="pin-length" name="length" min="4" max="10" required><br><br><label for="new-pin">New PIN:</label><input type="password" id="new-pin" name="pin" placeholder="Leave blank to keep current"><label for="confirm-pin">Confirm New PIN:</label><input type="password" id="confirm-pin" name="pin_confirm" placeholder="Leave blank to keep current"><button type="submit" class="button">Save PIN Settings</button></form></div></div><script>function getCookie(name){const value=`; ${document.cookie}`;const parts=value.split(`; ${name}=`);if(parts.length===2)return parts.pop().split(';').shift();return null}
function logout(){window.location.href='/logout'}
function openTab(evt,tabName){var i,tabcontent,tablinks;tabcontent=document.getElementsByClassName("tab-content");for(i=0;i<tabcontent.length;i++){tabcontent[i].style.display="none"}tablinks=document.getElementsByClassName("tab-link");for(i=0;i<tablinks.length;i++){tablinks[i].className=tablinks[i].className.replace(" active","")}document.getElementById(tabName).style.display="block";evt.currentTarget.className+=" active"}
function showStatus(message,isError=false){const statusDiv=document.getElementById('status');statusDiv.textContent=message;statusDiv.className='status-message '+(isError?'status-err':'status-ok');statusDiv.style.display='block';setTimeout(()=>statusDiv.style.display='none',5000)}
function fetchKeys(){fetch('/api/keys').then(response=>response.json()).then(data=>{const tbody=document.querySelector('#keys-table tbody');tbody.innerHTML='';data.forEach((key,index)=>{const row=tbody.insertRow();row.innerHTML=`<td>${key.name}</td><td class="code">${key.code}</td><td><progress value="${key.timeLeft}" max="30"></progress></td><td><button class="button-delete" onclick="removeKey(${index})">Remove</button></td>`})}).catch(err=>showStatus('Error fetching keys.',true))}
document.getElementById('add-key-form').addEventListener('submit',function(e){e.preventDefault();const name=document.getElementById('key-name').value;const secret=document.getElementById('key-secret').value;const formData=new FormData();formData.append('name',name);formData.append('secret',secret);fetch('/api/add',{method:'POST',body:new URLSearchParams(formData)}).then(res=>{if(res.ok){showStatus('Key added successfully!');fetchKeys();this.reset()}else{showStatus('Failed to add key.',true)}}).catch(err=>showStatus('Error: '+err,true))});
function removeKey(index){if(!confirm('Are you sure?'))return;const formData=new FormData();formData.append('index',index);fetch('/api/remove',{method:'POST',body:new URLSearchParams(formData)}).then(res=>{if(res.ok){showStatus('Key removed successfully!');fetchKeys()}else{showStatus('Failed to remove key.',true)}}).catch(err=>showStatus('Error: '+err,true))};
document.getElementById('change-password-form').addEventListener('submit',function(e){e.preventDefault();const newPass=document.getElementById('new-password').value;const confirmPass=document.getElementById('confirm-password').value;if(newPass!==confirmPass){showStatus('Passwords do not match!',true);return}
const formData=new FormData();formData.append('password',newPass);fetch('/api/change_password',{method:'POST',body:new URLSearchParams(formData)}).then(res=>res.text().then(text=>{if(res.ok)showStatus(text);else showStatus(text,true)}))});
document.getElementById('upload-splash-form').addEventListener('submit',function(e){e.preventDefault();const fileInput=document.getElementById('splash-file');if(fileInput.files.length===0){showStatus('Please select a file first.',true);return}
const formData=new FormData();formData.append('splash',fileInput.files[0]);fetch('/api/upload_splash',{method:'POST',body:formData,headers:{'Authorization':'Bearer '+getCookie('session')}}).then(res=>res.text().then(text=>{if(res.ok)showStatus(text);else showStatus(text,true)}))});
document.getElementById('delete-splash-btn').addEventListener('click',()=>{if(!confirm('Are you sure you want to delete the splash screen?'))return;fetch('/api/delete_splash',{method:'POST'}).then(res=>res.text().then(text=>{if(res.ok)showStatus(text);else showStatus(text,true)}))});
document.getElementById('export-keys-btn').addEventListener('click',()=>{window.location.href='/api/export'});
document.getElementById('import-keys-btn').addEventListener('click',()=>{document.getElementById('import-file').click()});
document.getElementById('import-file').addEventListener('change',function(e){if(e.target.files.length===0)return;if(!confirm('This will overwrite all current keys. Are you sure?'))return;const formData=new FormData();formData.append('import',e.target.files[0]);fetch('/api/import',{method:'POST',body:formData,headers:{'Authorization':'Bearer '+getCookie('session')}}).then(res=>{if(res.ok){showStatus('Import successful!');fetchKeys()}else{showStatus('Import failed.',true)}})});
document.getElementById('reboot-btn').addEventListener('click',()=>{if(!confirm('Are you sure you want to reboot?'))return;fetch('/api/reboot',{method:'POST'}).then(()=>showStatus('Rebooting...'))});
function fetchPinSettings(){fetch('/api/pincode_settings').then(response=>response.json()).then(data=>{document.getElementById('pin-enabled').checked=data.enabled;document.getElementById('pin-length').value=data.length}).catch(err=>showStatus('Error fetching PIN settings.',true))}
document.getElementById('pincode-settings-form').addEventListener('submit',function(e){e.preventDefault();const newPin=document.getElementById('new-pin').value;const confirmPin=document.getElementById('confirm-pin').value;if(newPin!==confirmPin){showStatus('PINs do not match!',true);return}
const formData=new FormData();formData.append('enabled',document.getElementById('pin-enabled').checked);formData.append('length',document.getElementById('pin-length').value);if(newPin){formData.append('pin',newPin);formData.append('pin_confirm',confirmPin)}
fetch('/api/pincode_settings',{method:'POST',body:new URLSearchParams(formData)}).then(res=>res.text().then(text=>{if(res.ok){showStatus(text);document.getElementById('new-pin').value='';document.getElementById('confirm-pin').value=''}else{showStatus(text,true)}}))});
function fetchThemeSettings(){
    fetch('/api/theme')
        .then(response => response.json())
        .then(data => {
            if(data.theme === 'light'){
                document.getElementById('theme-light').checked = true;
            } else {
                document.getElementById('theme-dark').checked = true;
            }
        })
        .catch(err => showStatus('Error fetching theme settings.', true));
}

document.getElementById('theme-selection-form').addEventListener('submit', function(e){
    e.preventDefault();
    const selectedTheme = document.querySelector('input[name="theme"]:checked').value;
    const formData = new FormData();
    formData.append('theme', selectedTheme);
    fetch('/api/theme', {
        method: 'POST',
        body: new URLSearchParams(formData)
    })
    .then(res => res.text().then(text => {
        if(res.ok) {
            showStatus(text);
        } else {
            showStatus(text, true);
        }
    }))
    .catch(err => showStatus('Error applying theme: ' + err, true));
});

function openTab(evt,tabName){
    var i,tabcontent,tablinks;
    tabcontent=document.getElementsByClassName("tab-content");
    for(i=0;i<tabcontent.length;i++){
        tabcontent[i].style.display="none"
    }
    tablinks=document.getElementsByClassName("tab-link");
    for(i=0;i<tablinks.length;i++){
        tablinks[i].className=tablinks[i].className.replace(" active","")
    }
    document.getElementById(tabName).style.display="block";
    evt.currentTarget.className+=" active";
    if(tabName === 'Display'){
        fetchThemeSettings();
    }
}

document.addEventListener('DOMContentLoaded',function(){fetchKeys();fetchPinSettings();document.querySelector('.tab-link').click()});
</script></body></html>
)rawliteral";