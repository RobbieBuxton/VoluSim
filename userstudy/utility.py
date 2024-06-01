from bson import ObjectId
import simpleaudio as sa
import os

# Helper function to handle MongoDB ObjectId when serializing to JSON
def json_encoder(o):
    if isinstance(o, ObjectId):
        return str(o)
    return o

def play_beep():
    # Generate a beep sound
    play_sound("countdown.wav")

def play_finished():
	# Generate a finish sound
	play_sound("finished.wav")

def play_sound(sound_path):
	# Generate a beep sound
    dir_path = os.path.dirname(os.path.realpath(__file__))
    wav_path = os.path.join(dir_path, f"data/{sound_path}")
    wave_obj = sa.WaveObject.from_wave_file(wav_path)
    play_obj = wave_obj.play()
    play_obj.wait_done()