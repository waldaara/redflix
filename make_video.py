with open("video.txt", "w") as file:
    for i in range(1, 10000 + 1):
        file.write(f"{i}\n")