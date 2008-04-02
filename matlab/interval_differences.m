function D = interval_differences(aData)
% Computes real interval differences for data

numIntervals = size(aData, 2);
N = size(aData, 1);

diff = abs([aData zeros(N, 1)] - [zeros(N, 1) aData]);
D = diff(:, 2:size(diff, 2)-1);
