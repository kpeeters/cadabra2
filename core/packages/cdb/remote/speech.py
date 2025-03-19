
import io
import wave
import time
import tempfile
import cdb.remote.highlight
import openai
import subprocess
import time

initialised=False
model    = None
warping  = False
client   = None
say_proc = None
say_num  = 0

def init(url="http://localhost:8880/v1"):
    global client
    client=openai.OpenAI(base_url=url, api_key="not-needed")

def warp(w=True):
    global warping
    warping=w

def tts(txt, fname):
    # print(f"calling api for {txt}...")
    with client.audio.speech.with_streaming_response.create(
            model="kokoro",
            speed=1.1,
            voice="bm_fable", #single or multiple voicepack combo
            input=txt
    ) as response:
        response.stream_to_file(fname)
        
def say_async(fname, delay):
    global say_proc
    # print(f"playing...")
    say_proc = subprocess.Popen(["bash", "-c", f"sleep {delay} && mpv {fname}"],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)

def say(text, subtitle=True, subtext="", delay=0):
    """
    Say a text. This will do the text-to-speech, then play the audio
    async. So it returns immediately after the playing starts.
    The async task will play silence for the indicated delay first.

    If another text is still being spoken, this function always
    blocks until that previous text is finished.
    """
    global model, warping, say_num, say_proc
    
    if client == None:
        init()

    if not warping:
        say_num += 1
        fname = f"output_{say_num}.mp3"
        tts(text, fname)
        if say_proc != None:
            say_proc.wait()
        say_async(fname, delay)
       
    if subtitle:
        if subtext!="":
            cdb.remote.highlight.subtitle(subtext)
        else:
            cdb.remote.highlight.subtitle(text)

#    if subtitle:
#        cdb.remote.highlight.subtitle()

if __name__ == "__main__":
    say("Hello!", subtitle=False)
    say("We are going to have some fun with this.", subtitle=False)
    say_proc.wait()
    
