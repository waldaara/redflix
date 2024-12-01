with open("video.txt", "w") as file:
    for i in range(1, 100_000 + 1):
        file.write(f"{i}\n")