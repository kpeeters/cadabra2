"""
Text-to-speech for scripted tutorials.

By default this talks to a local OpenAI-compatible TTS server (e.g. Kokoro)
at http://localhost:8880/v1. Environment variables:

  CADABRA_TTS=openai     use OpenAI's hosted TTS; needs OPENAI_API_KEY
  CADABRA_TTS=local      use the local server (default)
  CADABRA_TTS_MODEL=...  override the model (openai default: tts-1-hd)
  CADABRA_TTS_VOICE=...  override the voice (openai default: fable)
"""

import io
import os
import wave
import time
import tempfile
import cdb.remote.highlight
import openai
import subprocess

initialised=False
warping  = False
client   = None
tts_model = None
tts_voice = None
say_proc = None
say_num  = 0

def init(url=None, backend=None):
    global client, tts_model, tts_voice
    if backend is None:
        backend = os.environ.get("CADABRA_TTS", "local")
    backend = backend.lower()
    if backend == "openai":
        if not os.environ.get("OPENAI_API_KEY"):
            raise RuntimeError("CADABRA_TTS=openai requires OPENAI_API_KEY to be set.")
        client=openai.OpenAI(base_url=url) if url else openai.OpenAI()
        tts_model, tts_voice = "tts-1-hd", "fable"
    elif backend == "local":
        client=openai.OpenAI(base_url=url or "http://localhost:8880/v1", api_key="not-needed")
        tts_model, tts_voice = "kokoro", "bm_fable"
    else:
        raise ValueError(f"Unknown CADABRA_TTS backend '{backend}'; use 'local' or 'openai'.")
    tts_model = os.environ.get("CADABRA_TTS_MODEL", tts_model)
    tts_voice = os.environ.get("CADABRA_TTS_VOICE", tts_voice)

def warp(w=True):
    global warping
    warping=w

def tts(txt, fname):
    # print(f"calling api for {txt}...")
    with client.audio.speech.with_streaming_response.create(
            model=tts_model,
            speed=1.1,
            voice=tts_voice,
            input=txt
    ) as response:
        response.stream_to_file(fname)
        
def say_async(fname, delay):
    global say_proc
    # print(f"playing...")
    say_proc = subprocess.Popen(["bash", "-c", f"sleep {delay} && mpv {fname}"],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)

def say(text, subtitle=True, subtext="", delay=0, block=False, bottom=None):
    """
    Say a text. This will do the text-to-speech, then play the audio
    async. So it returns immediately after the playing starts.
    The async task will play silence for the indicated delay first.

    If another text is still being spoken, this function always
    blocks until that previous text is finished.
    """
    global warping, say_num, say_proc
    
    if client == None:
        init()

    if not warping:
        say_num += 1
        fname = f"/tmp/output_{say_num}.mp3"
        tts(text, fname)
        if say_proc != None:
            say_proc.wait()
        say_async(fname, delay)
       
    if subtitle:
        if subtext!="":
            cdb.remote.highlight.subtitle(subtext, bottom=bottom)
        else:
            cdb.remote.highlight.subtitle(text, bottom=bottom)

    if block:
        say_proc.wait()
        
#    if subtitle:
#        cdb.remote.highlight.subtitle()

if __name__ == "__main__":
    say("Hello!", subtitle=False)
    say("We are going to have some fun with this.", subtitle=False)
    say_proc.wait()
    
