function Z = intel_eval(aData)
% Computes evaluation of data for naive Intel model
% aData is NxNUM_INTERVALS array of samples

numIntervals = size(aData, 2);
N = size(aData, 1);

Z = ones(N, 1);

% Evaluate soft inequalities between interval lengths
for i = 1:numIntervals-1
    for j = i:numIntervals-1
        inequalities = (aData(:, i) ~= aData(:, j));
        Z = Z .* exp(log(2) * 1 * inequalities);
    end;
end;
