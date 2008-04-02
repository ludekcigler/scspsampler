from math import ceil

def join_intervals(intervals, max_intervals):
    """
    Join intervals using simple heuristic so that there are max_intervals in the result

    Intervals are represented as a dictionary with (lower bound, upper bound) as a key
    and probability as a value
    """

    total_intervals = len(intervals)
    ideal_probability = 1.0 / max_intervals
    print "ideal %f" % ideal_probability

    splitted_intervals = {}
    # Split too big intervals
    intervals_keys = intervals.keys()
    intervals_keys.sort()
    for lower_bound, upper_bound in intervals_keys:
        interval = (lower_bound, upper_bound)
        probability = intervals[interval]
        if (probability == 0.0):
            continue

        interval_length = upper_bound - lower_bound
        sub_intervals = int(max(min(ceil(probability / (0.5*ideal_probability)), interval_length), 1))
        lb = lower_bound

        for i in xrange(0, sub_intervals):
            ub = lower_bound + int(ceil(interval_length * (i+1)/ float(sub_intervals)))
            splitted_intervals[(lb, ub)] = ((ub - lb) * probability) / interval_length
            lb = ub


    if (len(splitted_intervals) <= max_intervals):
        return splitted_intervals

    result = {}
    cumulated_probability = 0.0
    created_interval_probability = 0.0
    created_interval_lb = 0
    created_interval_ub = 0
    debt = 0.0
    interval_counter = 0
    print "Splitted"
    print "Sum %f" % sum(splitted_intervals.values())
    splitted_intervals_keys = splitted_intervals.keys()
    splitted_intervals_keys.sort()

    for lower_bound, upper_bound in splitted_intervals_keys:
        interval = (lower_bound, upper_bound)
        probability = splitted_intervals[interval]
        print "interval %s = %f" % (interval, probability)

        if probability > 0.0:
            if created_interval_probability == 0.0:
                if probability >= ideal_probability + debt:
                    result[interval] = probability
                    cumulated_probability += probability
                else:
                    created_interval_lb = lower_bound
                    created_interval_ub = upper_bound
                    created_interval_probability = probability
            else:
                if created_interval_probability + probability >= 2*ideal_probability + debt:
                    result[(created_interval_lb, created_interval_ub)] = created_interval_probability
                    cumulated_probability += created_interval_probability
                    created_interval_probability = 0.0
                    result[interval] = probability
                    cumulated_probability += probability
                elif created_interval_probability + probability >= ideal_probability + debt:
                    created_interval_ub = upper_bound
                    result[(created_interval_lb, created_interval_ub)] = created_interval_probability + probability
                    cumulated_probability += probability + created_interval_probability
                    created_interval_probability = 0.0
                else:
                    created_interval_ub = upper_bound
                    created_interval_probability += probability

        interval_counter += 1
        debt = len(result)*ideal_probability - cumulated_probability
    else:
        if created_interval_probability > 0.0:
            result[(created_interval_lb, created_interval_ub)] = created_interval_probability

    return result
    
if __name__ == "__main__":
    intervals1 = {(0, 1): 0.01, (1, 2): 0.49, (3, 6): 0.05, (9,20): 0.1, (22, 26): 0.05, (28, 31): 0.1, (35, 38): 0.1, (39, 40): 0.1}

    print "Sum %f" % sum(intervals1.values())

    joined = join_intervals(intervals1, 14)
    print joined

    print "Sum %f, size %d" % (sum(joined.values()), len(joined))
