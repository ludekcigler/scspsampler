NUM_INTERVALS = 11
for i in range(0, NUM_INTERVALS):
    for j in range(i + 1, NUM_INTERVALS):
        # abs(x_i - x_i+1) == x_{i+NUM_INTERVALS}; hard constraint
        print "%d %d %d %d 1" % (i, (i+1), j, j+1)
