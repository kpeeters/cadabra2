#import obspython as obs
from obswebsocket import obsws, requests
import subprocess
import json
import time

def get_window_geometry(window_name):
    """Get window position and size using xdotool"""
    try:
        # Get window ID
        cmd = f"xdotool search --name '{window_name}'"
        window_id = subprocess.check_output(cmd, shell=True).decode().strip()
        print(window_id)
        
        # Get geometry
        cmd = f"xdotool getwindowgeometry --shell {window_id}"
        geometry = subprocess.check_output(cmd, shell=True).decode().strip()
        
        # Parse the geometry output
        geo_dict = {}
        for line in geometry.split('\n'):
            key, value = line.split('=')
            geo_dict[key] = int(value)
            
        return {
            'x': geo_dict['X'],
            'y': geo_dict['Y'],
            'width': geo_dict['WIDTH'],
            'height': geo_dict['HEIGHT']
        }
    except Exception as e:
        print(f"Error getting window geometry: {e}")
        return None

def setup_region_capture(window_name):
    # Connect to OBS WebSocket
    ws = obsws("localhost", 4455, "fikkie")
    ws.connect()
    
    try:
        # Get window geometry to determine capture region
        geometry = get_window_geometry(window_name)
        if not geometry:
            raise Exception("Could not get window geometry")
        
        # Add some padding to ensure we capture overlapping windows
        padding = 10  # adjust as needed
        geometry['x'] = max(0, geometry['x'] - padding)
        geometry['y'] = max(0, geometry['y'] - padding)
        geometry['width'] += 2 * padding
        geometry['height'] += 2 * padding
        
        # Create a scene if it doesn't exist
        scene_name = "Region_Recording"
        ws.call(requests.CreateScene(sceneName=scene_name))
        
        # Create display capture source with region
        source_settings = {
            "display": 0,  # Primary display
            "cursor": True
        }
        source_name = "Display Capture"
        
        ws.call(requests.CreateInput(
            sceneName=scene_name,
            inputName=source_name,
            inputKind="xshm_input",  # Use monitor capture instead of window capture
            settings=source_settings
        ))
        time.sleep(1)
        
        # Try different crop setting formats
        crop_settings = [
            {
                "cut_left": geometry['x'],
                "cut_top": geometry['y'],
                "cut_right": geometry['x']+geometry['width'],
                "cut_bottom": geometry['y']+geometry['height']
            }
        ]
        
        for settings in crop_settings:
            try:
                print(f"Trying crop settings: {json.dumps(settings, indent=2)}")
                ws.call(requests.SetInputSettings(
                    inputName=source_name,
                    inputSettings=settings
                ))
                print("Settings applied successfully")
            except Exception as e:
                print(f"Failed with settings: {str(e)}")
        
        
        # Set as current scene
        ws.call(requests.SetCurrentProgramScene(sceneName=scene_name))
        
        # Start recording
        ws.call(requests.StartRecord())
        
        print(f"Recording started for region around window: {window_name}")
        print(f"Capture region: {json.dumps(geometry, indent=2)}")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        ws.disconnect()

if __name__ == "__main__":
    # Replace with your GTK window name
    gtk_window_name = "Your Window Name"
    setup_region_capture(gtk_window_name)
