% We assume the variable RESULTS_PATH set to the path where WCSP results are stored

random_result_files = dir([RESULTS_PATH '/*.random.csv']);

for i = 1:size(random_result_files, 1)
    [tok mat] = regexp(random_result_files(i).name, '^([^\.]*)\.', 'tokens');
    problem_id = char(tok{1});

    random_samples = load([RESULTS_PATH '/' random_result_files(i).name]);

    domains = {};
    domain_sizes = load([RESULTS_PATH '/' problem_id '.domains.csv']);
    for j = 1:size(domain_sizes, 2)
        domains{j} = 0:(domain_sizes(j)-1);
    end;

    other_result_files = dir([RESULTS_PATH '/' problem_id '*.csv']);

    for j = 1:size(other_result_files, 1)
            
        [tok mat] = regexp(other_result_files(j).name, '([^\.]*)\.csv$', 'tokens');
        alg_id = char(tok{1});
        if strcmp(alg_id, 'random') || strcmp(alg_id, 'domains')
            continue;
        end;

        alg_samples = load([RESULTS_PATH '/' other_result_files(j).name]);

        KL = kl_divergence(alg_samples, random_samples, domains);
        fprintf('ProblemID: %s\t AlgID: %s\t KLDiv: %e\tAvgVAR: %e\tAvgCost: %e\n', problem_id, alg_id, mean(KL), avg_variance(alg_samples), avg_cost(alg_samples, K));
    end;
end;
