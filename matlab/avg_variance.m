function v = avg_variance(aSamples)
% Compute average variance over all variables

v = mean(var(aSamples(:, 2:size(aSamples, 2))));
