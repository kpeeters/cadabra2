#import obspython as obs
from obswebsocket import obsws, requests
import subprocess
import json
import time
import re

def get_window_geometry(window_name):
    """Get window position and size using xdotool"""
    try:
        # Get window ID
        cmd = f"xdotool search --name '{window_name}'"
        window_id = subprocess.check_output(cmd, shell=True).decode().strip()
        # print(window_id)
        
        # Get window geometry
        cmd = f"xwininfo -id {window_id}"
        wininfo_output = subprocess.check_output(cmd, shell=True).decode()
        
        # Parse xwininfo output using regex
        x_match = re.search(r'Absolute upper-left X:\s+(\d+)', wininfo_output)
        y_match = re.search(r'Absolute upper-left Y:\s+(\d+)', wininfo_output)
        width_match = re.search(r'Width:\s+(\d+)', wininfo_output)
        height_match = re.search(r'Height:\s+(\d+)', wininfo_output)

        # Get screen geometry
        cmd = f"xdotool getdisplaygeometry"
        screen_geometry = subprocess.check_output(cmd, shell=True).decode().strip()
        # print(screen_geometry)
            
        sw, sh = screen_geometry.split(' ')
            
        return {
            'x': int(x_match.group(1)),
            'y': int(y_match.group(1)),
            'width': int(width_match.group(1)),
            'height': int(height_match.group(1)),
            'screen_width':  int(sw),
            'screen_height': int(sh)
        }
    except Exception as e:
        print(f"Error getting window geometry: {e}")
        return None

ws = None    
    
def setup_region_capture(window_name, password):
    global ws
    # Connect to OBS WebSocket
    ws = obsws("localhost", 4455, password)
    ws.connect()
    
    try:
        # Get window geometry to determine capture region
        geometry = get_window_geometry(window_name)
        if not geometry:
            raise Exception("Could not get window geometry")
        
        # Add some padding to ensure we capture overlapping windows
        # padding = 10  # adjust as needed
        # geometry['x'] = max(0, geometry['x'] - padding)
        # geometry['y'] = max(0, geometry['y'] - padding)
        # geometry['width'] += 2 * padding
        # geometry['height'] += 2 * padding
        
        # Create a scene if it doesn't exist
        scene_name = "Cadabra Recording"
        ws.call(requests.CreateScene(sceneName=scene_name))
        
        # Create display capture source with region
        source_settings = {
            "display": 0,  # Primary display
            "cursor": True
        }
        source_name = "Cadabra Window"
        
        ws.call(requests.CreateInput(
            sceneName=scene_name,
            inputName=source_name,
            inputKind="xshm_input",  # Use monitor capture instead of window capture
            settings=source_settings
        ))
        ws.call(requests.CreateInput(
            sceneName=scene_name,
            inputName="Audio Capture",
            inputKind="pulse_output_capture"
        ))
        time.sleep(1)
        
        crop_settings = {
            "cut_left":   geometry['x'],
            "cut_top":    geometry['y'],
            "cut_right":  geometry['screen_width'] - (geometry['x']+geometry['width']),
            "cut_bot":    geometry['screen_height']- (geometry['y']+geometry['height'])
        }
        
        ws.call(requests.SetInputSettings(
            inputName=source_name,
            inputSettings=crop_settings
        ))

        # Turn off microphone. FIXME: not working
        ws.call(requests.SetInputSettings(
            inputName="Mic/Aux",
            inputSettings={ "muted": True }
        ))
        
        # Set as current scene
        ws.call(requests.SetCurrentProgramScene(sceneName=scene_name))
        
        # print(f"Capture region: {json.dumps(geometry, indent=2)}")
        
    except Exception as e:
        print(f"Error: {e}")
#     finally:
#         ws.disconnect()

def start_record():
    global ws
    ws.call(requests.StartRecord())
        
def stop_record():
    global ws
    ws.call(requests.StopRecord())
        
        
if __name__ == "__main__":
    # Replace with your GTK window name
    gtk_window_name = "Your Window Name"
    setup_region_capture(gtk_window_name, sys.argv[1])
