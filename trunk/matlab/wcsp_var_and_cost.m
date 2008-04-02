% We assume the variable RESULTS_PATH set to the path where WCSP results are stored

result_files = dir([RESULTS_PATH '/*.csv']);

for i = 1:size(result_files, 1)
    [tok mat] = regexp(result_files(i).name, '^([^\.]*)\.', 'tokens');
    problem_id = char(tok{1});
    
    [tok mat] = regexp(result_files(i).name, '([^\.]*)\.csv$', 'tokens');
    alg_id = char(tok{1});

    if strcmp(alg_id, 'random') || strcmp(alg_id, 'domains')
        continue;
    end;

    alg_samples = load([RESULTS_PATH '/' result_files(i).name]);
    fprintf('ProblemID: %s\t AlgID: %s\tAvgVAR: %e\tAvgCost: %e\n', problem_id, alg_id, avg_variance(alg_samples), avg_cost(alg_samples, K));
end;

