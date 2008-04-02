function H = intel_create_interval_histogram(filename)
% Creates an interval length histogram for a data sample stored in filename

data = load(filename);

nintdiff = num_different_intervals(data(:, 1:12));

H = hist(nintdiff, 0:11);
H = H ./ sum(H);
