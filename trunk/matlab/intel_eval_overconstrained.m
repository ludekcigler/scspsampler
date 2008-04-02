function Z = intel_eval_extravars_overconstrained(aData)
% Evaluate overconstrained data for the naive intel model

Z = intel_eval(aData);
% x_1 == 0
Z = Z .* (aData(:, 1) == 0);
% x_2 == 10
Z = Z .* (aData(:, 2) == 10);
