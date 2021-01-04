import multiprocessing
import subprocess
import time

from os import listdir


SONGS_PATH = './songs/'
IMAGES_PATH = './images/'
BITMAP_FILE_PATH = 'slavik, dai bitmap'

SONGS = listdir(SONGS_PATH)
IMAGES = listdir(IMAGES_PATH)


def next_song(current):
    return SONGS[(SONGS.index(current) + 1) % len(SONGS)]


def next_image(current):
    return IMAGES[(IMAGES.index(current) + 1) % len(IMAGES)]


def previous_song(current):
    return SONGS[SONGS.index(current) - 1]


def previous_image(current):
    return IMAGES[IMAGES.index(current) - 1]


def play(song, image):
    with open(image, 'r') as img:
        bitmap = img.readline()
    subprocess.Popen(['cvlc', song])
    with open(BITMAP_FILE_PATH, 'w') as bitmap_f:
        bitmap_f.write(bitmap)


if __name__ == '__main__':
    print(SONGS)
    last_mod = ''
    i = 0
    while True:
        time.sleep(.01)
        is_playing = 0
        with open('shared-file') as f:
            data = f.readlines()
            command = data[0].strip()
            mod = data[1]
            if mod == last_mod:
                continue
            last_mod = mod
            if command == 'next':
                i = (i + 1) % 2
                current_song = SONGS_PATH + SONGS[i] + '.mp3'
                current_image = IMAGES_PATH + IMAGES[i]
                p = multiprocessing.Process(target=play, args=(current_song, current_image))
                p.start()
            elif command == 'previous':
                i = (i - 1) % 2
                current_song = SONGS_PATH + SONGS[i] + '.mp3'
                current_image = IMAGES_PATH + IMAGES[i]
                p = multiprocessing.Process(target=play, args=(current_song, current_image))
                p.start()
            elif command == 'pp':
                if is_playing:
                    p.terminate()
                else:
                    current_song = SONGS_PATH + SONGS[i] + '.mp3'
                    current_image = IMAGES_PATH + IMAGES[i]
                    p = multiprocessing.Process(target=play, args=(current_song, current_image))
                    p.start()
            else:
                print('shared-file syntax error')
