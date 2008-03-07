#include <iostream>
#include "../src/domain_interval.h"

DomainIntervalSet create_interval1() {
        DomainIntervalSet result;
        result.insert(DomainInterval(0, 10, 0.15));
        result.insert(DomainInterval(15, 35, 0.20));
        result.insert(DomainInterval(35, 80, 0.20));
        result.insert(DomainInterval(85, 100, 0.10));
        result.insert(DomainInterval(200, 201, 0.35));

        return result;
}

DomainIntervalSet create_interval2() {
        DomainIntervalSet result;
        result.insert(DomainInterval(2, 5, 0.5));
        result.insert(DomainInterval(15, 20, 0.1));
        result.insert(DomainInterval(25, 35, 0.20));
        result.insert(DomainInterval(85, 100, 0.1));
        result.insert(DomainInterval(200, 202, 0.1));

        return result;
}

Domain create_domain() {
        Domain d;
        d.insert(2);
        /*
        d.insert(4);
        d.insert(5);
        d.insert(10);
        d.insert(11);
        d.insert(23);
        d.insert(28);
        d.insert(76);
        d.insert(87);
        d.insert(89);
        d.insert(90);
        d.insert(92);
        d.insert(93);
        d.insert(95);
        d.insert(96);*/
        d.insert(200);

        return d;
}

int main(int argc, char ** argv) {
        DomainIntervalSet il1 = create_interval1();
        DomainIntervalSet il2 = create_interval2();
        Domain d = create_domain();

        std::cout << "IL 1:\t\t";
        interval_list_pprint(il1);
        std::cout << "IL 2:\t\t";
        interval_list_pprint(il2);

        DomainIntervalSet merger = merge_intervals(il1, il2);

        std::cout << "Merged:\t\t";
        interval_list_pprint(merger);

        merger = normalize_intervals(merger);
        std::cout << "Normalized:\t";
        interval_list_pprint(merger);

        DomainIntervalSet joined = join_intervals(merger, 18);
        std::cout << "Joined:\t\t";
        interval_list_pprint(joined);

        DomainIntervalSet adjusted = adjust_intervals_to_domain(joined, d);
        adjusted = normalize_intervals(adjusted);
        std::cout << "Adjusted:\t";
        interval_list_pprint(adjusted);

        Domain::iterator domIt = d.begin();
        --domIt;
        std::cout << "DomIt " << (domIt == d.begin()) << std::endl;

        DomainIntervalSet ds;
        ds.insert(DomainInterval(0, 10, 0.3));
        ds.insert(DomainInterval(0, 10, 0.5));
        std::cout << "Find: " << (ds.find(DomainInterval(0, 10, 0.5)) != ds.end()) << std::endl;
        interval_list_pprint(ds);
}
