function cost = avg_cost(aSamples, k)
% Compute average cost for a set of samples
% k is a parameter of the cost function (1/2)^(k * cost)

cost = mean(-log2(aSamples(:, 1)) ./ k);
