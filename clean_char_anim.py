import os
import sys

def clean_directory(directory, anim_name, total_frames):
    frames = []
    for i in range(0, int(total_frames)+1, 10):
        frames.append(i) 

    allowed_files = []
    for frame in frames:
        allowed_files.append(f"{anim_name}{frame}.obj")

    for filename in os.listdir(directory):
        file_path = os.path.join(directory, filename)
        if os.path.isfile(file_path):
            if filename not in allowed_files and not filename.endswith('.blend') and not filename.endswith('.blend1'):
                if filename.startswith(anim_name):
                    print(f"Removing file: {filename}")
                    os.remove(file_path)
            else:
                print(f"Keeping file: {filename}")
                if filename.endswith('.obj'):
                    frame_number = int(filename[len(base_filename):-4])
                    new_name = f"{base_filename}{frame_number // 10}.obj"
                    new_path = os.path.join(directory, new_name)
                    print(f"Renaming {filename} to {new_name}")
                    os.rename(file_path, new_path)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python clean_char_anim.py <directory> <filename> <total_frames>")
        sys.exit(1)

    directory = 'data/chars/'+sys.argv[1]
    base_filename = sys.argv[2]
    total_frames = sys.argv[3]

    clean_directory(directory, base_filename, total_frames)
