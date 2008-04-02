NUM_INTERVALS = 11
for i in range(0, NUM_INTERVALS):
    # abs(x_i - x_i+1) == x_{i+NUM_INTERVALS}; hard constraint
    print "EQ %d %d %d 0" % (i, (i+1), i + NUM_INTERVALS + 1)

for i in range(NUM_INTERVALS + 1, 2*NUM_INTERVALS + 1):
    for j in range(i+1, 2*NUM_INTERVALS + 1):
        # x_i != x_j; soft constraint
        print "NEQ %d %d 1" % (i, j)

for i in range(0, NUM_INTERVALS + 1):
    for j in range(i+1, NUM_INTERVALS + 1):
        # x_i != x_j; soft constraint
        print "NEQ %d %d 0" % (i, j)
