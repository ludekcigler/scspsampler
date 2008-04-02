function Z = intel_eval_extravars(aData)
% Computes evaluation of data for extra-vars Intel model
% aData is Nx(2*NUM_INTERVALS - 1) array of samples

numIntervals = (size(aData, 2) + 1) / 2;
N = size(aData, 1);

Z = ones(N, 1);

% Evaluate equalities to interval variables and interval lengths
for i = 1:numIntervals-1
    intervalEqualities = (abs(aData(:, i) - aData(:, i+1)) == aData(:, i + numIntervals))
    Z = Z .* intervalEqualities;
end;

% Evaluate soft inequalities between interval
for i = numIntervals+1:2*numIntervals-1
    for j = i+1:2*numIntervals-1
        inequalities = (aData(:, i) ~= aData(:, j));
        Z = Z .* exp(log(2) * 10 * inequalities);
    end;
end;
