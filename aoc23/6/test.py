time = 50

for i in range(time+1):
    hold_for = i
    move_for = time - i
    dist = hold_for * move_for
    print(f"move at {hold_for}mm/ms for {move_for}ms, travel {dist}")
