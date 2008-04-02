function D = num_different_intervals(aData)
% Computes number of different interval lengths in data

lengths = interval_differences(aData);

D = [];
for i = 1:size(aData, 1)
    D = [D; size(unique(lengths(i, :)), 2)];
end;
