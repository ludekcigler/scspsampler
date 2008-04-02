% Load generated samples for intel extra vars data
% Compute evaluation of these data

Z = intel_eval(data);

% Compute Euclidean distance matrix between data
distance = pdist(data);

scaled = mds(squareform(distance), 2);
