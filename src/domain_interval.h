#ifndef DOMAIN_INTERVAL_H_
#define DOMAIN_INTERVAL_H_

#include <list>
#include <map>

#include "types.h"

struct DomainInterval {
        DomainInterval():
                lowerBound(0), upperBound(0), probability(0.0) {};

        DomainInterval(VarType aLowerBound, VarType aUpperBound, double aProbability):
                lowerBound(aLowerBound), upperBound(aUpperBound), probability(aProbability) {};

        /**
         * Compare upper bounds of the intervals.
         * The "real" ordering would have to be only partial, but since we are
         * only going to compare intervals belonging to a single variable
         * being in a single list, it shouldn't matter (since they should be disjunct)
         */
        bool operator<(const DomainInterval & aInterval) const {
                return upperBound < aInterval.upperBound;
        };

        bool operator==(const DomainInterval & aInterval) const {
                return (lowerBound == aInterval.lowerBound) && (upperBound == aInterval.upperBound);
        };

        std::string pprint() const;

        VarType lowerBound, upperBound;
        double probability;
};

typedef std::set<DomainInterval> DomainIntervalSet;

typedef std::map<VarIdType, DomainInterval> DomainIntervalAssignment;

/**
 * Merges two intervals together, mixing their probabilities
 */
DomainIntervalSet merge_intervals(const DomainIntervalSet & aList1, const DomainIntervalSet & aList2);

DomainIntervalSet normalize_intervals(const DomainIntervalSet & aList);

DomainIntervalSet join_intervals(const DomainIntervalSet & aList, unsigned int aMaxDomainIntervals);

void interval_list_pprint(const DomainIntervalSet & aList);

DomainIntervalSet adjust_intervals_to_domain(const DomainIntervalSet & aList, const Domain & aDomain);

#endif // DOMAIN_INTERVAL_H_
