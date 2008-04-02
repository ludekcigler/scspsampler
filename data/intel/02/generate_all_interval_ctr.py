NUM_INTERVALS = 11
for i in range(0, NUM_INTERVALS):
    for j in range(i + 1, NUM_INTERVALS):
        # abs(x_i - x_i+1) == x_{i+NUM_INTERVALS}; hard constraint
        print "INEQ %d %d %d %d 1" % (i, (i+1), j, j+1)

for i in range(0, NUM_INTERVALS + 1):
    for j in range(i + 1, NUM_INTERVALS + 1):
        print "NEQ %d %d 1" % (i, j)
