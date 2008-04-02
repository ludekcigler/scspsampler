#include <iostream>
#include "../src/domain_interval.h"

DomainIntervalMap create_interval1() {
        DomainIntervalMap result;
        result[DomainInterval(0, 10)] = 0.15;
        result[DomainInterval(15, 35)] = 0.20;
        result[DomainInterval(35, 80)] = 0.20;
        result[DomainInterval(85, 100)] = 0.10;
        result[DomainInterval(200, 201)] = 0.35;

        return result;
}

DomainIntervalMap create_interval2() {
        DomainIntervalMap result;
        result[DomainInterval(2, 5)] = 0.5;
        result[DomainInterval(15, 20)] = 0.1;
        result[DomainInterval(25, 35)] = 0.20;
        result[DomainInterval(85, 100)] = 0.1;
        result[DomainInterval(200, 202)] = 0.1;

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
        DomainIntervalMap il1 = create_interval1();
        DomainIntervalMap il2 = create_interval2();
        Domain d = create_domain();

        std::cout << "IL 1:\t\t" << interval_list_pprint(il1) << std::endl;
        std::cout << "IL 2:\t\t" << interval_list_pprint(il2) << std::endl;

        DomainIntervalMap merger = merge_intervals(il1, il2);

        std::cout << "Merged:\t\t" << interval_list_pprint(merger);

        merger = normalize_intervals(merger);
        std::cout << "Normalized:\t" << interval_list_pprint(merger);

        DomainIntervalMap joined = join_intervals(merger, 6);
        std::cout << "Joined:\t\t" << interval_list_pprint(joined);
        std::cout << "Joined size:\t" << joined.size() << std::endl;

        DomainIntervalMap adjusted = adjust_intervals_to_domain(joined, d);
        adjusted = normalize_intervals(adjusted);
        std::cout << "Adjusted:\t" << interval_list_pprint(adjusted);

}
