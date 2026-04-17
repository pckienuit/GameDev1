#!/usr/bin/env python3
"""
convert_sounds.py
Convert audio files in assets/sounds/ to PCM WAV compatible with XAudio2.

Supports:
  - MP3  -> PCM WAV  (via Windows MCI, no install needed)
  - WAVE_FORMAT_EXTENSIBLE -> plain PCM WAV (pure Python byte patching)
  - Already-valid PCM WAV  -> skipped

Usage:
  python convert_sounds.py
  python convert_sounds.py assets/sounds/die.wav assets/sounds/stomp.mp3
"""

import os
import sys
import struct
import shutil
import ctypes
import subprocess

SOUNDS_DIR = "assets/sounds"

# ---------------------------------------------------------------------------
# Format detection
# ---------------------------------------------------------------------------

def read_header(path, n=40):
    with open(path, "rb") as f:
        return f.read(n)


def is_mp3(path):
    h = read_header(path, 4)
    return h[:3] == b"ID3" or h[:2] in (b"\xFF\xFB", b"\xFF\xF3", b"\xFF\xF2", b"\xFF\xFA")


def is_ogg(path):
    return read_header(path, 4) == b"OggS"


def is_wav_extensible(path):
    h = read_header(path, 40)
    if len(h) < 24 or h[:4] != b"RIFF" or h[8:12] != b"WAVE":
        return False
    if h[12:16] == b"fmt ":
        fmt_tag = struct.unpack_from("<H", h, 20)[0]
        return fmt_tag == 0xFFFE
    return False


def is_valid_pcm_wav(path):
    h = read_header(path, 24)
    if len(h) < 24 or h[:4] != b"RIFF" or h[8:12] != b"WAVE":
        return False
    if h[12:16] == b"fmt ":
        fmt_tag = struct.unpack_from("<H", h, 20)[0]
        return fmt_tag in (1, 3)
    return False


# ---------------------------------------------------------------------------
# WAVE_FORMAT_EXTENSIBLE -> plain PCM (pure Python, no ffmpeg needed)
# ---------------------------------------------------------------------------

def convert_extensible_to_pcm(input_path, output_path):
    with open(input_path, "rb") as f:
        raw = bytearray(f.read())

    if raw[:4] != b"RIFF" or raw[8:12] != b"WAVE":
        return False

    pos = 12
    while pos + 8 <= len(raw):
        chunk_id   = raw[pos:pos+4]
        chunk_size = struct.unpack_from("<I", raw, pos+4)[0]
        aligned    = chunk_size + (chunk_size & 1)

        if chunk_id == b"fmt ":
            fmt_tag = struct.unpack_from("<H", raw, pos+8)[0]
            if fmt_tag != 0xFFFE or chunk_size < 28:
                break

            # SubFormat GUID first 2 bytes = real format tag
            sub_fmt         = struct.unpack_from("<H", raw, pos + 8 + 16)[0]
            nChannels       = struct.unpack_from("<H", raw, pos+8+2)[0]
            nSamplesPerSec  = struct.unpack_from("<I", raw, pos+8+4)[0]
            nAvgBytesPerSec = struct.unpack_from("<I", raw, pos+8+8)[0]
            nBlockAlign     = struct.unpack_from("<H", raw, pos+8+12)[0]
            wBitsPerSample  = struct.unpack_from("<H", raw, pos+8+14)[0]

            new_fmt = struct.pack("<HHIIHH",
                sub_fmt, nChannels, nSamplesPerSec,
                nAvgBytesPerSec, nBlockAlign, wBitsPerSample,
            ) + b"\x00\x00"  # cbSize = 0 -> 18 bytes total

            old_end   = pos + 8 + chunk_size + (chunk_size & 1)
            new_chunk = b"fmt " + struct.pack("<I", 18) + new_fmt
            raw = raw[:pos] + bytearray(new_chunk) + raw[old_end:]
            struct.pack_into("<I", raw, 4, len(raw) - 8)
            break

        pos += 8 + aligned

    with open(output_path, "wb") as f:
        f.write(raw)
    return True


# ---------------------------------------------------------------------------
# MP3 -> PCM WAV via Windows MCI (built-in, no install)
# ---------------------------------------------------------------------------

def convert_via_mci(input_path, output_path):
    """
    mciSendStringW: open MP3, save as WAV.
    Built into Windows via winmm.dll / the MP3 ACM codec.
    """
    try:
        winmm = ctypes.windll.winmm

        abs_in  = os.path.abspath(input_path).replace("/", "\\")
        abs_out = os.path.abspath(output_path).replace("/", "\\")

        def mci(cmd):
            return winmm.mciSendStringW(cmd, None, 0, None)

        alias = "cvt_snd"
        # Open as MPEGVideo (covers MP3 via DirectShow/MCI)
        err = mci('open "{}" type MPEGVideo alias {} wait'.format(abs_in, alias))
        if err != 0:
            err = mci('open "{}" alias {} wait'.format(abs_in, alias))
        if err != 0:
            return False

        mci("set {} time format milliseconds".format(alias))
        err = mci('save {} "{}"'.format(alias, abs_out))
        mci("close {}".format(alias))

        return os.path.isfile(abs_out) and os.path.getsize(abs_out) > 44
    except Exception as e:
        print("    MCI error: {}".format(e))
        return False


def find_ffmpeg():
    """Find ffmpeg: PATH first, then common WinGet install location."""
    if shutil.which("ffmpeg"):
        return "ffmpeg"

    import glob
    winget_base = os.path.expandvars(
        r"%LOCALAPPDATA%\Microsoft\WinGet\Packages"
    )
    pattern = os.path.join(winget_base, "Gyan.FFmpeg*", "**", "ffmpeg.exe")
    hits = glob.glob(pattern, recursive=True)
    if hits:
        return hits[0]

    return None


def convert_via_ffmpeg(input_path, output_path):
    ffmpeg = find_ffmpeg()
    if not ffmpeg:
        return False
    result = subprocess.run(
        [ffmpeg, "-y", "-i", input_path,
         "-ar", "44100", "-ac", "1", "-c:a", "pcm_s16le", output_path],
        capture_output=True
    )
    # ffmpeg returns 0 on success; ignore pipe exit codes
    return result.returncode == 0 or (
        os.path.isfile(output_path) and os.path.getsize(output_path) > 44
    )


# ---------------------------------------------------------------------------
# Per-file processing
# ---------------------------------------------------------------------------

def process_file(path):
    name = os.path.basename(path)
    ext  = os.path.splitext(name)[1].lower()

    if is_mp3(path) or is_ogg(path) or ext in (".mp3", ".ogg"):
        fmt = "MP3" if (is_mp3(path) or ext == ".mp3") else "OGG"
        print("  [{}] {} detected -> converting to PCM WAV ...".format(name, fmt))
        tmp = path + ".converting.wav"
        try:
            ok = convert_via_ffmpeg(path, tmp) or convert_via_mci(path, tmp)
            if ok:
                wav_path = os.path.splitext(path)[0] + ".wav"
                os.replace(tmp, wav_path)
                if wav_path != path:
                    try: os.remove(path)
                    except: pass
                print("  [{}] OK -> {}".format(name, os.path.basename(wav_path)))
                return True
            else:
                print("  [{}] FAILED - install ffmpeg for MP3 support".format(name))
                return False
        finally:
            if os.path.exists(tmp):
                try: os.remove(tmp)
                except: pass

    elif is_wav_extensible(path):
        print("  [{}] WAVE_FORMAT_EXTENSIBLE -> patching to plain PCM ...".format(name))
        tmp = path + ".converting.wav"
        if convert_extensible_to_pcm(path, tmp):
            os.replace(tmp, path)
            print("  [{}] OK Patched".format(name))
            return True
        else:
            if os.path.exists(tmp):
                try: os.remove(tmp)
                except: pass
            print("  [{}] FAILED Patch failed".format(name))
            return False

    elif is_valid_pcm_wav(path):
        print("  [{}] OK Already valid PCM WAV -- skipped".format(name))
        return True

    else:
        print("  [{}] WARN Unknown format -- skipped".format(name))
        return False


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    targets = sys.argv[1:]

    if targets:
        paths = [os.path.abspath(t) for t in targets]
    else:
        if not os.path.isdir(SOUNDS_DIR):
            print("ERROR: {0}/ not found. Run from project root.".format(SOUNDS_DIR))
            sys.exit(1)
        paths = [
            os.path.join(SOUNDS_DIR, f)
            for f in sorted(os.listdir(SOUNDS_DIR))
            if os.path.splitext(f)[1].lower() in (".wav", ".mp3", ".ogg")
        ]

    if not paths:
        print("No audio files found.")
        return

    print("[SOUND] Converter -- {} file(s)\n".format(len(paths)))
    ok = sum(1 for p in paths if process_file(p))
    status = "[OK]" if ok == len(paths) else "[WARN]"
    print("\n{} {}/{} files ready for XAudio2".format(status, ok, len(paths)))
    if ok < len(paths):
        print("   -> Install ffmpeg (https://ffmpeg.org) for MP3 support if MCI failed")


if __name__ == "__main__":
    main()
