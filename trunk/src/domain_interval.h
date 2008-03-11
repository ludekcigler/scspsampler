#ifndef DOMAIN_INTERVAL_H_
#define DOMAIN_INTERVAL_H_

#include <list>
#include <map>

#include "types.h"

struct DomainInterval {
        DomainInterval():
                lowerBound(0), upperBound(0) {};

        DomainInterval(VarType aLowerBound, VarType aUpperBound):
                lowerBound(aLowerBound), upperBound(aUpperBound) {};

        /**
         * Compare upper bounds of the intervals.
         * The "real" ordering would have to be only partial, but since we are
         * only going to compare intervals belonging to a single variable
         * being in a single list, it shouldn't matter (since they should be disjunct)
         */
        bool operator<(const DomainInterval & aInterval) const {
                return upperBound < aInterval.upperBound;
        };

        bool operator<=(const DomainInterval & aInterval) const {
                return upperBound <= aInterval.upperBound;
        };

        bool operator==(const DomainInterval & aInterval) const {
                return (lowerBound == aInterval.lowerBound) && (upperBound == aInterval.upperBound);
        };

        std::string pprint() const;

        VarType lowerBound, upperBound;
};

typedef std::map<DomainInterval, double> DomainIntervalMap;

typedef std::map<VarIdType, DomainInterval> DomainIntervalAssignment;

typedef std::map<DomainInterval, double> IntervalProbabilityDistribution;

/**
 * Merges two intervals together, mixing their probabilities
 */
DomainIntervalMap merge_intervals(const DomainIntervalMap & aList1, const DomainIntervalMap & aList2);

DomainIntervalMap normalize_intervals(const DomainIntervalMap & aList);

DomainIntervalMap join_intervals(const DomainIntervalMap & aList, unsigned int aMaxDomainIntervals);

std::string interval_list_pprint(const DomainIntervalMap & aList);

DomainIntervalMap adjust_intervals_to_domain(const DomainIntervalMap & aList, const Domain & aDomain);

DomainIntervalMap uniform_intervals_for_domain(const Domain & aDomain, unsigned int aMaxDomainIntervals);

#endif // DOMAIN_INTERVAL_H_
