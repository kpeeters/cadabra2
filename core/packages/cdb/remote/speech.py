
from piper.voice import PiperVoice
from pydub import AudioSegment
from pydub.playback import play
import io
import wave
import time
import tempfile

initialised=False
model = None

def init(voice_file="en_GB-alba-medium.onnx"):
    global model, initialised
    model = PiperVoice.load(voice_file, voice_file+".json")
    initialised=True

def say(text):
    if not initialised:
        raise Exception("First call cdb.videos.init(onnx_filename) to initialise.")
    
    tmp = tempfile.NamedTemporaryFile()
    wave_file = wave.open(tmp.name, "w")
    model.synthesize(text, wave_file)
    wave_file.close()
    audio_data = AudioSegment.from_wav(tmp.name)
    play(audio_data)

if __name__ == "__main__":
    say("Hello!")
    say("We are going to have some fun with this.")
