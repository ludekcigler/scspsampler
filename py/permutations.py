#!/usr/bin/env python

def xcombinations(items, n):
    if n==0: yield []
    else:
        for i in xrange(len(items)):
            for cc in xcombinations(items[:i]+items[i+1:],n-1):
                yield [items[i]]+cc

def xuniqueCombinations(items, n):
    if n==0: yield []
    else:
        for i in xrange(len(items)):
            for cc in xuniqueCombinations(items[i+1:],n-1):
                yield [items[i]]+cc
            
def xselections(items, n):
    if n==0: yield []
    else:
        for i in xrange(len(items)):
            for ss in xselections(items, n-1):
                yield [items[i]]+ss

def xpermutations(items):
    return xcombinations(items, len(items))

def permutation_distribution(permuted_list, prefix = None):
    """
    Generates all permutations of a permuted_list, and adds a prefix to it
    Then computes the number of different intervals we get
    """
    prefix = prefix or []
    results = {}
    for p in xpermutations(permuted_list):
        p = prefix + p
        interval_diff = [abs(x1 - x2) for x1, x2 in zip(p[0:-1], p[1:])]

        satisfied_constraints = \
            sum([interval_diff[i] != interval_diff[j] for i in xrange(0, len(p) - 2) \
                for j in xrange(i + 1, len(p) - 1)])

        num_intervals = len(set(interval_diff))
        if results.has_key((num_intervals, satisfied_constraints)):
            results[(num_intervals, satisfied_constraints)] = results[(num_intervals, satisfied_constraints)] + 1
        else:
            results[(num_intervals, satisfied_constraints)] = 1


    for num_intervals, satisfied_constraints in results:
        print "%d, %d, %d" % (num_intervals, satisfied_constraints, results[(num_intervals, satisfied_constraints)])

if __name__ == "__main__":
    print "Classical problem"
    permutation_distribution(range(0, 12))

    print "\nOverconstrained problem"
    permutation_distribution(range(1, 10) + [11], [0, 10])
