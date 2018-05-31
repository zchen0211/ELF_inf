import random

extended_board_size = 21

for i in range(extended_board_size ** 2):
    print("0x%x, " % random.getrandbits(64), end="")
    if i % 8 == 7:
        print("")
