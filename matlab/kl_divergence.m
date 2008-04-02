function KL = kl_divergence(aSamples, aSolutions, aDomains)
% Compute Kullback-Leibler divergence between samples (aSamples)
% and exact solutions (aSolutions).
%
% aDomains is a cell array containing a domain for each variable
%
% The first column in both arguments denotes the evaluation of a sample. Each
% row denotes one sample.

EPSILON = 1e-20;

samples = aSamples(:, 2:size(aSamples, 2));

solutions = aSolutions(:, 2:size(aSolutions, 2));
solutionEvals = aSolutions(:, 1);

solutionDenom = sum(solutionEvals);

KL = [];

for varId = 1:length(aDomains)
    varSampleDist = [];
    varSolutionDist = [];
    % Walk through all variables and compute the divergence on each of them
    for val = aDomains{varId}
        % For each value compute the probability of encountering that value
        varSampleIndex = (samples(:, varId) == val);
        % For real samples, we don't need to multiply the distribution by sample probability
        varSampleDist = [varSampleDist sum(varSampleIndex)]; 

        varSolutionIndex = (solutions(:, varId) == val);
        varSolutionDist = [varSolutionDist sum(varSolutionIndex .* solutionEvals) / solutionDenom];
    end

    % Now adjust the zero values by a tiny margin, just to avoid those pesky NaNs
    varSampleDist = (varSampleDist + EPSILON) ./ sum(varSampleDist + EPSILON);
    varSolutionDist = (varSolutionDist + EPSILON) ./ sum(varSolutionDist + EPSILON);

    KL = [KL kldiv(aDomains{varId}, varSampleDist, varSolutionDist)];
end 

