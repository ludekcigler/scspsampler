function D = num_different_values(aData)
% Computes number of different values in data

D = [];
for i = 1:size(aData, 1)
    D = [D; size(unique(aData(i, :)), 2)];
end;

